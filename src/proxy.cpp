// Materialize 1.78 Chinese localization — runtime injection + icall hook.
//
// Injected by proxying WINHTTP.dll (statically imported by UnityPlayer.dll).
// Never touches any file of the application itself.
//
// Strategy: hook the *native* implementations of the IMGUI icalls that carry a
// GUIContent — the exact point every piece of IMGUI-visible text passes through.
// At that point all lookups/comparisons have already happened, so swapping the
// text can only ever affect what is drawn.
//
//   GUI/DoLabel.DoButton.DoToggle.DoWindow   -> native icalls of GUI.Label/Button/...
//   GUIStyle/Internal_Draw.Internal_Draw2    -> native icalls of GUIStyle.Draw, GUI.Box, ...
//
// The GUIContent text pointer is swapped for the duration of the call and
// restored afterwards: GUIContent.Temp() reuses a shared static instance, so
// mutating it permanently would leak the translation into unrelated draw calls.

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <shellapi.h>   // Shell_NotifyIcon / ExtractIconEx (not pulled in by WIN32_LEAN_AND_MEAN)
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <wchar.h>
#include <ctype.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

#include "detours.h"

// ---------------------------------------------------------------- mono api --
typedef void   MonoDomain;
typedef void   MonoAssembly;
typedef void   MonoImage;
typedef void   MonoClass;
typedef void   MonoMethod;
typedef void   MonoString;
typedef void   MonoClassField;
typedef void   MonoObject;

typedef MonoDomain*   (*p_mono_get_root_domain)();
typedef MonoDomain*   (*p_mono_domain_get)();
typedef MonoDomain*   (*p_mono_thread_attach)(MonoDomain*);
typedef void          (*p_mono_assembly_foreach)(void (*)(MonoAssembly*, void*), void*);
typedef MonoImage*    (*p_mono_assembly_get_image)(MonoAssembly*);
typedef const char*   (*p_mono_image_get_name)(MonoImage*);
typedef MonoClass*    (*p_mono_class_from_name)(MonoImage*, const char*, const char*);
typedef MonoMethod*   (*p_mono_class_get_method_from_name)(MonoClass*, const char*, int);
typedef void*         (*p_mono_lookup_internal_call)(MonoMethod*);
typedef MonoClassField* (*p_mono_class_get_field_from_name)(MonoClass*, const char*);
typedef int           (*p_mono_field_get_offset)(MonoClassField*);
typedef MonoString*   (*p_mono_string_new_utf16)(MonoDomain*, const uint16_t*, int32_t);
typedef MonoObject*   (*p_mono_object_new)(MonoDomain*, MonoClass*);
typedef MonoObject*   (*p_mono_runtime_invoke)(MonoMethod*, void*, void**, MonoObject**);
typedef MonoObject*   (*p_mono_field_get_value_object)(MonoDomain*, MonoClassField*, MonoObject*);
typedef uint32_t      (*p_mono_gchandle_new)(MonoObject*, int32_t);

static p_mono_get_root_domain              m_get_root_domain;
static p_mono_domain_get                   m_domain_get;
static p_mono_thread_attach                m_thread_attach;
static p_mono_assembly_foreach             m_assembly_foreach;
static p_mono_assembly_get_image           m_assembly_get_image;
static p_mono_image_get_name               m_image_get_name;
static p_mono_class_from_name              m_class_from_name;
static p_mono_class_get_method_from_name   m_class_get_method_from_name;
static p_mono_lookup_internal_call         m_lookup_internal_call;
static p_mono_class_get_field_from_name    m_class_get_field_from_name;
static p_mono_field_get_offset             m_field_get_offset;
static p_mono_string_new_utf16             m_string_new_utf16;
static p_mono_object_new                   m_object_new;
static p_mono_runtime_invoke               m_runtime_invoke;
static p_mono_field_get_value_object       m_field_get_value_object;
static p_mono_gchandle_new                 m_gchandle_new;

// ---------------------------------------------------------------- globals ---
static HMODULE      g_self;
static std::wstring g_root;          // application directory
static std::wstring g_logPath;
static std::wstring g_dictPath;
static std::wstring g_iniPath;

static bool         g_probeOnly = false;   // log only, never replace
static bool         g_logMiss   = true;
static std::wstring g_title;               // Chinese title ("... - 星空汉化版")
static std::wstring g_titleEn;             // title while switched to English

static int          g_textFieldOff = -1;   // GUIContent.m_Text offset (bytes)

static CRITICAL_SECTION g_logLock;
static FILE*            g_log;

typedef std::unordered_map<std::u16string, std::u16string> Dict;
static Dict*             g_dict = nullptr;
static SRWLOCK           g_dictLock = SRWLOCK_INIT;
static FILETIME          g_dictStamp = {0, 0};
static size_t            g_dictSize = 0;

static bool         g_logAll = false;
static std::unordered_set<std::string>* g_missed = nullptr;
static std::unordered_set<std::string>* g_drawn  = nullptr;
static std::mutex                       g_missLock;

// Chinese/English switch state (declared early: TranslateContent and ReadIni
// both reference it; the tray/hotkey implementation lives further down).
static volatile LONG g_langZh = 1;
static bool          g_trayOn = true;
static HWND          g_hwnd;
static HICON         g_icon;
static UINT          g_wmTaskbarCreated;
static UINT          g_hotkeyMods;
static UINT          g_hotkeyVk;
static std::wstring  g_hotkeyText;

static volatile LONG64 g_hookCalls = 0;
static volatile LONG64 g_hookHits  = 0;
static int          g_diagN = 0;

// ------------------------------------------------------------------ log -----
static void LogA(const char* fmt, ...)
{
    if (!g_log) return;
    EnterCriticalSection(&g_logLock);
    SYSTEMTIME st; GetLocalTime(&st);
    fprintf(g_log, "[%02d:%02d:%02d.%03d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    va_list ap; va_start(ap, fmt); vfprintf(g_log, fmt, ap); va_end(ap);
    fputc('\n', g_log);
    fflush(g_log);
    LeaveCriticalSection(&g_logLock);
}

// Wide (path/title) messages are converted to UTF-8 so the whole log stays a
// single, readable encoding - mixing fwprintf into a byte stream produces both
// mojibake and interleaving.
static std::string WideToUtf8(const std::wstring& w)
{
    if (w.empty()) return std::string();
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    if (n <= 0) return std::string();
    std::string o(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &o[0], n, nullptr, nullptr);
    return o;
}

// ------------------------------------------------------------ conversion ----
static std::string Utf16ToUtf8(const std::u16string& s)
{
    if (s.empty()) return std::string();
    int n = WideCharToMultiByte(CP_UTF8, 0, (const wchar_t*)s.c_str(), (int)s.size(),
                                nullptr, 0, nullptr, nullptr);
    if (n <= 0) return std::string();
    std::string out(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, (const wchar_t*)s.c_str(), (int)s.size(),
                        &out[0], n, nullptr, nullptr);
    return out;
}

static std::u16string Utf8ToUtf16(const std::string& s)
{
    if (s.empty()) return std::u16string();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (n <= 0) return std::u16string();
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return std::u16string((const char16_t*)w.data(), w.size());
}

static std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (n <= 0) return std::wstring();
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

// ----------------------------------------------------------- memory guards --
// Explicit readability checks instead of SEH: MSVC forbids __try in functions
// that need object unwinding, and IsBadReadPtr is deprecated.
static bool Readable(const void* p, size_t n)
{
    if (!p) return false;
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(p, &mbi, sizeof(mbi)) == 0) return false;
    if (mbi.State != MEM_COMMIT) return false;
    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;
    uintptr_t start = (uintptr_t)mbi.BaseAddress;
    uintptr_t end   = start + mbi.RegionSize;
    return ((uintptr_t)p + n) <= end;
}

// --------------------------------------------------------------- dict -------
static std::string Unescape(const std::string& s)
{
    std::string o; o.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char c = s[++i];
            if      (c == 'r')  o += '\r';
            else if (c == 'n')  o += '\n';
            else if (c == 't')  o += '\t';
            else if (c == '\\') o += '\\';
            else { o += '\\'; o += c; }
        } else o += s[i];
    }
    return o;
}

static void LoadDict(bool force)
{
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(g_dictPath.c_str(), GetFileExInfoStandard, &fad)) {
        if (force) LogA("[dict] not found: (root)\\_hanhua\\dict.tsv");
        return;
    }
    if (!force &&
        fad.ftLastWriteTime.dwLowDateTime  == g_dictStamp.dwLowDateTime &&
        fad.ftLastWriteTime.dwHighDateTime == g_dictStamp.dwHighDateTime)
        return;

    FILE* f = _wfopen(g_dictPath.c_str(), L"rb");
    if (!f) { LogA("[dict] open failed"); return; }
    std::string buf;
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz > 0) { buf.resize((size_t)sz); fread(&buf[0], 1, (size_t)sz, f); }
    fclose(f);

    size_t p = 0;
    if (buf.size() >= 3 && (unsigned char)buf[0] == 0xEF &&
        (unsigned char)buf[1] == 0xBB && (unsigned char)buf[2] == 0xBF) p = 3;

    Dict* nd = new Dict();
    nd->reserve(4096);
    size_t lines = 0, bad = 0;
    while (p < buf.size()) {
        size_t e = buf.find('\n', p);
        if (e == std::string::npos) e = buf.size();
        std::string ln = buf.substr(p, e - p);
        p = e + 1;
        lines++;
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        if (ln.empty() || ln[0] == '#') continue;
        size_t tab = ln.find('\t');
        if (tab == std::string::npos) { bad++; continue; }
        std::string k = Unescape(ln.substr(0, tab));
        std::string v = Unescape(ln.substr(tab + 1));
        if (k.empty()) continue;
        (*nd)[Utf8ToUtf16(k)] = Utf8ToUtf16(v);
    }

    AcquireSRWLockExclusive(&g_dictLock);
    Dict* old = g_dict;
    g_dict = nd;
    g_dictSize = nd->size();
    g_dictStamp = fad.ftLastWriteTime;
    ReleaseSRWLockExclusive(&g_dictLock);
    delete old;

    LogA("[dict] loaded %zu entries (%zu lines, %zu malformed)", nd->size(), lines, bad);
}

static void RecordMiss(const std::u16string& s)
{
    if (!g_logMiss || !g_missed) return;
    std::string u = Utf16ToUtf8(s);
    if (u.size() < 2) return;
    bool hasAlpha = false, hasCJK = false, hasSpace = false;
    for (size_t i = 0; i < u.size(); i++) {
        unsigned char c = (unsigned char)u[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) hasAlpha = true;
        if (c == ' ') hasSpace = true;
        if (c >= 0x80) hasCJK = true;
    }
    if (!hasAlpha || hasCJK) return;
    if (u.find(":\\") != std::string::npos || u.find("\\\\") != std::string::npos) return;
    (void)hasSpace;

    std::lock_guard<std::mutex> g(g_missLock);
    if (g_missed->insert(u).second) {
        std::wstring mp = g_root + L"\\_hanhua\\logs\\miss.log";
        FILE* f = _wfopen(mp.c_str(), L"ab");
        if (f) { fwrite(u.data(), 1, u.size(), f); fputc('\n', f); fclose(f); }
    }
}

// Cumulative log of every distinct string that reaches the render exit, hit or
// not. Unlike miss.log this also proves what IS covered, and it is the reliable
// way to tell whether a given dialog actually opened.
static void RecordDrawn(const std::u16string& s)
{
    if (!g_logAll || !g_drawn) return;
    std::string u = Utf16ToUtf8(s);
    if (u.empty()) return;
    std::lock_guard<std::mutex> g(g_missLock);
    if (g_drawn->insert(u).second) {
        std::wstring mp = g_root + L"\\_hanhua\\logs\\drawn.log";
        FILE* f = _wfopen(mp.c_str(), L"ab");
        if (f) { fwrite(u.data(), 1, u.size(), f); fputc('\n', f); fclose(f); }
    }
}

// --------------------------------------------------------- text swap core ---
static void DumpProbe(const char* tag, void* content, int probeOff)
{
    if (!Readable(content, 64)) { LogA("[diag] %s content=%p NOT READABLE", tag, content); return; }
    if (g_textFieldOff < 0) return;
    char* p = (char*)content + g_textFieldOff;
    if (!Readable(p, 8)) { LogA("[diag] %s field@%p not readable", tag, p); return; }
    void* v = *(void**)p;
    if (!v) { LogA("[diag] %s %p m_Text=NULL", tag, content); return; }
    if (!Readable(v, 24)) { LogA("[diag] %s %p m_Text=%p NOT READABLE", tag, content, v); return; }
    int32_t len = *(int32_t*)((char*)v + 16);
    if (len < 0 || len > 4096 || !Readable((char*)v + 20, (size_t)len * 2)) {
        LogA("[diag] %s %p m_Text=%p len=%d (bad)", tag, content, v, len); return;
    }
    std::u16string s((const char16_t*)((char*)v + 20), (size_t)len);
    LogA("[diag] %s content=%p m_Text=\"%s\" len=%d", tag, content, Utf16ToUtf8(s).c_str(), len);
    (void)probeOff;
}

// Returns the replacement MonoString for `content`, or nullptr if nothing to do.
// *origOut receives the pre-existing pointer that the caller must restore.
// (Boehm GC is non-moving and scans the native stack conservatively, so the
//  freshly created MonoString stays reachable until it is stored.)
static MonoString* TranslateContent(void* content, void** origOut)
{
    *origOut = nullptr;
    if (!content || g_textFieldOff < 0) return nullptr;
    if (!Readable(content, (size_t)g_textFieldOff + sizeof(void*))) return nullptr;

    MonoString* cur = *(MonoString**)((char*)content + g_textFieldOff);
    if (!cur) return nullptr;
    if (!Readable(cur, 20)) return nullptr;

    int32_t len = *(int32_t*)((char*)cur + 16);
    if (len <= 0 || len > 8192) return nullptr;
    if (!Readable((char*)cur + 20, (size_t)len * 2)) return nullptr;

    std::u16string s((const char16_t*)((char*)cur + 20), (size_t)len);

    RecordDrawn(s);
    if (!g_langZh) return nullptr;          // switched to English via tray/hotkey

    std::u16string repl;
    AcquireSRWLockShared(&g_dictLock);
    Dict* d = g_dict;
    if (d) {
        Dict::const_iterator it = d->find(s);
        if (it != d->end()) repl = it->second;
    }
    RecordDrawn(s);
    bool hit = !repl.empty();
    ReleaseSRWLockShared(&g_dictLock);

    if (!hit) { RecordMiss(s); return nullptr; }

    MonoString* ns = m_string_new_utf16(m_domain_get(),
                                        (const uint16_t*)repl.c_str(), (int32_t)repl.size());
    if (!ns) return nullptr;
    *origOut = cur;
    return ns;
}

// ------------------------------------------------ icall prototypes (shared) --
// Declared here because the in-app language button below calls the original
// GUI.DoButton directly, before the hook table itself is set up.
// NOTE: bool, not int - Unity's native DoButton/DoToggle return a bool in AL and
// leave the upper 24 bits of EAX undefined. Reading it as int yields garbage and
// makes the button look clicked on every event.
typedef bool (*t_DoButton)(void*, void*, void*);
static t_DoButton o_DoButton;

// ------------------------------------------------------- tiny button style --
// The four 20x20 map-panel buttons (P/C/O/S -> 粘贴/复制/打开/保存) are too small
// for a two-character label: the row is 100px wide (5/10/5 px gaps) and the
// stock button style clips text to the rect. Fix: give those four buttons a
// wider rect (the row has unused slack) and a private copy of GUI.skin.button
// with clipping = Overflow so the full label is drawn.
//
// Built lazily from inside the hook, NOT at startup: GUI.skin calls
// GUIUtility.CheckOnGUI() and throws on any thread that is not running OnGUI.
static MonoImage* FindImage(const char* name);   // defined in the mono-boot section below

static volatile LONG g_tinyTried = 0;
static void*         g_tinyStylePtr = nullptr;   // GUIStyle.m_Ptr of our copy
static MonoObject*   g_tinyStyleObj = nullptr;
static void*         g_skinButtonPtr = nullptr;  // GUIStyle.m_Ptr of GUI.skin.button
static MonoObject*   g_ownContent = nullptr;     // our own GUIContent (language button)

// Filled in by the setup below; used to draw a real IMGUI button of our own.
static void UpdateTray(HWND h);                  // defined with the tray code

static void EnsureTinyButtonStyle()
{
    if (InterlockedCompareExchange(&g_tinyTried, 1, 0) != 0) return;
    MonoImage* img = FindImage("UnityEngine.IMGUIModule.dll");
    if (!img) img = FindImage("UnityEngine.IMGUIModule");
    if (!img) { LogA("[tiny] IMGUI module not found"); return; }

    MonoClass* guiK   = m_class_from_name(img, "UnityEngine", "GUI");
    MonoClass* skinK  = m_class_from_name(img, "UnityEngine", "GUISkin");
    MonoClass* styleK = m_class_from_name(img, "UnityEngine", "GUIStyle");
    if (!guiK || !skinK || !styleK) { LogA("[tiny] classes missing"); return; }
    MonoDomain* dom = m_domain_get();

    MonoMethod* getSkin = m_class_get_method_from_name(guiK, "get_skin", 0);
    if (!getSkin) { LogA("[tiny] GUI.get_skin not found"); return; }
    MonoObject* ex = nullptr;
    MonoObject* skin = m_runtime_invoke(getSkin, nullptr, nullptr, &ex);
    if (ex || !skin) { LogA("[tiny] GUI.skin unavailable"); return; }

    MonoClassField* fBtn = m_class_get_field_from_name(skinK, "m_button");
    if (!fBtn) { LogA("[tiny] GUISkin.m_button not found"); return; }
    MonoObject* btn = m_field_get_value_object(dom, fBtn, skin);
    if (!btn) { LogA("[tiny] skin.button is null"); return; }

    MonoMethod* ctor = m_class_get_method_from_name(styleK, ".ctor", 1);
    if (!ctor) { LogA("[tiny] GUIStyle copy-ctor not found"); return; }
    MonoObject* mine = m_object_new(dom, styleK);
    if (!mine) { LogA("[tiny] mono_object_new(GUIStyle) failed"); return; }
    void* cargs[1] = { btn };
    ex = nullptr;
    m_runtime_invoke(ctor, mine, cargs, &ex);
    if (ex) { LogA("[tiny] copy-ctor threw"); return; }

    MonoMethod* setClip = m_class_get_method_from_name(styleK, "set_clipping", 1);
    if (!setClip) { LogA("[tiny] set_clipping not found"); return; }
    int32_t overflow = 0;                      // TextClipping.Overflow
    void* cargs2[1] = { &overflow };
    ex = nullptr;
    m_runtime_invoke(setClip, mine, cargs2, &ex);
    if (ex) { LogA("[tiny] set_clipping threw"); return; }

    MonoClassField* fPtr = m_class_get_field_from_name(styleK, "m_Ptr");
    if (!fPtr) { LogA("[tiny] GUIStyle.m_Ptr not found"); return; }
    int off = m_field_get_offset(fPtr);
    if (off < 0 || !Readable((char*)mine + off, sizeof(void*))) {
        LogA("[tiny] m_Ptr offset %d unreadable", off); return;
    }

    g_tinyStyleObj = mine;
    g_tinyStylePtr = *(void**)((char*)mine + off);
    m_gchandle_new(mine, 0);                   // root it for the process lifetime

    // The untouched skin button style, for the language button we draw ourselves.
    g_skinButtonPtr = *(void**)((char*)btn + off);

    // Our own GUIContent, reused every frame (the text field is rewritten when
    // the language changes, so the object itself stays put).
    MonoClass* contentK = m_class_from_name(img, "UnityEngine", "GUIContent");
    MonoMethod* ctor1 = contentK ? m_class_get_method_from_name(contentK, ".ctor", 1) : nullptr;
    if (contentK && ctor1) {
        MonoObject* c = m_object_new(dom, contentK);
        if (c) {
            static const uint16_t kEn[] = { 'E','n','g','l','i','s','h' };
            MonoString* s = m_string_new_utf16(dom, kEn, 7);
            void* a1[1] = { s };
            ex = nullptr;
            m_runtime_invoke(ctor1, c, a1, &ex);
            if (!ex) { g_ownContent = c; m_gchandle_new(c, 0); }
            else LogA("[lang-btn] GUIContent ctor threw");
        }
    } else {
        LogA("[lang-btn] GUIContent ctor not found");
    }

    LogA("[tiny] private button style ready (m_Ptr=%p off=%d, skin=%p, content=%p)",
         g_tinyStylePtr, off, g_skinButtonPtr, g_ownContent);
}

// ------------------------------------------------------- in-app lang button --
// Draws a real IMGUI button into the application's own OnGUI pass, right below
// "Hide Gui". Calling the original DoButton re-entrantly from inside our hook is
// safe because it happens at the same point in the call sequence every event, so
// control IDs stay stable.
static void SetLangFromButton()
{
    LONG v = InterlockedCompareExchange(&g_langZh, 0, 1);
    if (v == 1) InterlockedExchange(&g_langZh, 0);
    else        InterlockedExchange(&g_langZh, 1);
    LogA("[lang] switched to %s (in-app button)", g_langZh ? "zh" : "en");
    if (g_hwnd) UpdateTray(g_hwnd);
}

static void DrawLangButton(const float* hideRect)
{
    if (!g_ownContent || !g_skinButtonPtr) return;

    // Label shows the language you would switch TO.
    static const uint16_t kZh[] = { 0x4E2D, 0x6587 };            // 中文
    static const uint16_t kEn[] = { 'E','n','g','l','i','s','h' };
    const uint16_t* txt = g_langZh ? kEn : kZh;
    int len = g_langZh ? 7 : 2;
    MonoString* s = m_string_new_utf16(m_domain_get(), txt, len);
    if (s) *(MonoString**)((char*)g_ownContent + g_textFieldOff) = s;

    float r[4] = { hideRect[0], hideRect[1] + hideRect[3] + 4.0f, hideRect[2], hideRect[3] };
    if (o_DoButton(r, g_ownContent, g_skinButtonPtr)) SetLangFromButton();
}

// ------------------------------------------------------------ rect patches ---
static bool ContentText(void* content, std::string* out)
{
    out->clear();
    if (!content || g_textFieldOff < 0) return false;
    if (!Readable(content, (size_t)g_textFieldOff + sizeof(void*))) return false;
    MonoString* s = *(MonoString**)((char*)content + g_textFieldOff);
    if (!s || !Readable(s, 20)) return true;          // empty text is fine
    int32_t len = *(int32_t*)((char*)s + 16);
    if (len <= 0 || len > 256) return true;
    if (!Readable((char*)s + 20, (size_t)len * 2)) return false;
    *out = Utf16ToUtf8(std::u16string((const char16_t*)((char*)s + 20), (size_t)len));
    return true;
}

// The four 20x20 map-panel buttons (P/C/O/S -> 粘贴/复制/打开/保存).
// All seven panels share the same geometry: panel = 110x250 at y=20,
// thumbnail 5,25,100,100, this row at y=130 (abs 150), x offsets 5/30/60/85.
// Two-character labels do not fit that row (100px of usable width for four
// buttons), so they are re-laid out 2x2 into 50px-wide buttons, the three rows
// underneath move down, and the panel grows just enough to hold them.
struct TinyBtn { const char* zh; const char* en; float origX; int col; int row; };
static const TinyBtn kTiny[] = {
    { "粘贴", "P", 5.0f,  0, 0 },
    { "复制", "C", 30.0f, 1, 0 },
    { "打开", "O", 60.0f, 0, 1 },
    { "保存", "S", 85.0f, 1, 1 },
};

static void PatchGuiRect(const std::string& t, float* r, void** stylePtr)
{
    if (!r) return;

    for (int i = 0; i < 4; i++) {
        if (r[2] != 20.0f || r[3] != 20.0f || r[1] != 150.0f) break;
        if (t != kTiny[i].zh && t != kTiny[i].en) continue;
        float x0 = r[0] - kTiny[i].origX;
        r[0] = x0 + 5.0f + kTiny[i].col * 52.0f;
        r[1] = 150.0f + kTiny[i].row * 22.0f;
        r[2] = 50.0f;
        if (stylePtr && g_tinyStylePtr) *stylePtr = g_tinyStylePtr;
        return;
    }

    // rows below the button block, moved down to make room for the second row
    if (r[2] == 80.0f && r[3] == 20.0f && r[1] == 180.0f &&
        (t == "快速保存" || t == "Quick Save")) { r[1] = 197.0f; return; }
    if (r[2] == 80.0f && r[3] == 20.0f && r[1] == 210.0f &&
        (t == "预览"     || t == "Preview"))    { r[1] = 222.0f; return; }
    if (r[3] == 20.0f && r[1] == 240.0f && r[2] >= 44.0f && r[2] <= 51.0f &&
        (t == "创建" || t == "Create" || t == "清除" || t == "Clear")) { r[1] = 247.0f; return; }

    // the map panel box itself: 250 -> 256, enough for the extra button row
    if (r[1] == 20.0f && r[2] == 110.0f && r[3] == 250.0f) { r[3] = 256.0f; return; }
}

// ----------------------------------------------------------------- hooks ----
typedef void (*t_DoLabel)(void*, void*, void*);
typedef bool (*t_DoToggle)(void*, int, int, void*, void*);
typedef void (*t_DoWindow)(int, int, void*, void*, void*, void*, void*, int, void*);
typedef void (*t_InternalDraw)(void*, void*);
typedef void (*t_InternalDraw2)(void*, void*, void*, int, int);

static t_DoLabel       o_DoLabel;
static t_DoToggle      o_DoToggle;
static t_DoWindow      o_DoWindow;
static t_DoWindow      o_DoModalWindow;
static t_InternalDraw  o_InternalDraw;
static t_InternalDraw2 o_InternalDraw2;

// Every hook gets its own small budget of diagnostic dumps so a wrong argument
// layout is immediately visible in the log instead of failing silently.
static volatile LONG _hc_DoLabel = 8, _hc_DoButton = 8, _hc_DoToggle = 8,
                     _hc_DoWindow = 8, _hc_DoModal = 8, _hc_IDraw = 8, _hc_IDraw2 = 8;

#define HOOK_PROLOGUE(cptr, budget, tag)                                     \
    InterlockedIncrement64(&g_hookCalls);                                    \
    if (InterlockedDecrement(budget) >= 0) DumpProbe(tag, (void*)(cptr), 0); \
    void* _orig = nullptr;                                                   \
    if (!g_probeOnly) {                                                      \
        MonoString* _ns = TranslateContent((cptr), &_orig);                  \
        if (_ns) {                                                           \
            InterlockedIncrement64(&g_hookHits);                             \
            *(MonoString**)((char*)(cptr) + g_textFieldOff) = _ns;           \
        }                                                                    \
    }

#define HOOK_EPILOGUE(cptr)                                                  \
    if (_orig) *(MonoString**)((char*)(cptr) + g_textFieldOff) = (MonoString*)_orig;

static void WINAPI H_DoLabel(void* rect, void* content, void* style)
{
    HOOK_PROLOGUE(content, &_hc_DoLabel, "DoLabel")
    o_DoLabel(rect, content, style);
    HOOK_EPILOGUE(content)
}

static bool WINAPI H_DoButton(void* rect, void* content, void* style)
{
    EnsureTinyButtonStyle();
    HOOK_PROLOGUE(content, &_hc_DoButton, "DoButton")
    std::string t;
    ContentText(content, &t);
    PatchGuiRect(t, (float*)rect, &style);
    bool r = o_DoButton(rect, content, style);
    // draw our own language switch right under the app's "Hide Gui" button
    if (t == "隐藏界面" || t == "Hide Gui") DrawLangButton((const float*)rect);
    HOOK_EPILOGUE(content)
    return r;
}

static bool WINAPI H_DoToggle(void* rect, int id, int value, void* content, void* style)
{
    HOOK_PROLOGUE(content, &_hc_DoToggle, "DoToggle")
    bool r = o_DoToggle(rect, id, value, content, style);
    HOOK_EPILOGUE(content)
    return r;
}

static void WINAPI H_DoWindow(int id, int iid, void* clientRect, void* func, void* title,
                              void* style, void* skin, int force, void* outRect)
{
    HOOK_PROLOGUE(title, &_hc_DoWindow, "DoWindow")
    o_DoWindow(id, iid, clientRect, func, title, style, skin, force, outRect);
    HOOK_EPILOGUE(title)
}

static void WINAPI H_DoModalWindow(int id, int iid, void* clientRect, void* func, void* title,
                                   void* style, void* skin, int force, void* outRect)
{
    HOOK_PROLOGUE(title, &_hc_DoModal, "DoModalWindow")
    o_DoModalWindow(id, iid, clientRect, func, title, style, skin, force, outRect);
    HOOK_EPILOGUE(title)
}

static void WINAPI H_InternalDraw(void* content, void* args)
{
    HOOK_PROLOGUE(content, &_hc_IDraw, "Internal_Draw")
    o_InternalDraw(content, args);
    HOOK_EPILOGUE(content)
}

static void WINAPI H_InternalDraw2(void* style, void* rect, void* content, int controlID, int on)
{
    HOOK_PROLOGUE(content, &_hc_IDraw2, "Internal_Draw2")
    // this is where GUI.Box draws, so the map panel's Box is resized here
    std::string t;
    ContentText(content, &t);
    PatchGuiRect(t, (float*)rect, nullptr);
    o_InternalDraw2(style, rect, content, controlID, on);
    HOOK_EPILOGUE(content)
}

// ------------------------------------------------------------- mono boot ----
template <typename T>
static T Resolve(HMODULE h, const char* name)
{
    T p = (T)GetProcAddress(h, name);
    if (!p) LogA("[mono] MISSING export: %s", name);
    return p;
}

static bool BindMono(HMODULE h)
{
    m_get_root_domain            = Resolve<p_mono_get_root_domain>(h, "mono_get_root_domain");
    m_domain_get                 = Resolve<p_mono_domain_get>(h, "mono_domain_get");
    m_thread_attach              = Resolve<p_mono_thread_attach>(h, "mono_thread_attach");
    m_assembly_foreach           = Resolve<p_mono_assembly_foreach>(h, "mono_assembly_foreach");
    m_assembly_get_image         = Resolve<p_mono_assembly_get_image>(h, "mono_assembly_get_image");
    m_image_get_name             = Resolve<p_mono_image_get_name>(h, "mono_image_get_name");
    m_class_from_name            = Resolve<p_mono_class_from_name>(h, "mono_class_from_name");
    m_class_get_method_from_name = Resolve<p_mono_class_get_method_from_name>(h, "mono_class_get_method_from_name");
    m_lookup_internal_call       = Resolve<p_mono_lookup_internal_call>(h, "mono_lookup_internal_call");
    m_class_get_field_from_name  = Resolve<p_mono_class_get_field_from_name>(h, "mono_class_get_field_from_name");
    m_field_get_offset           = Resolve<p_mono_field_get_offset>(h, "mono_field_get_offset");
    m_string_new_utf16           = Resolve<p_mono_string_new_utf16>(h, "mono_string_new_utf16");
    m_object_new                 = Resolve<p_mono_object_new>(h, "mono_object_new");
    m_runtime_invoke             = Resolve<p_mono_runtime_invoke>(h, "mono_runtime_invoke");
    m_field_get_value_object     = Resolve<p_mono_field_get_value_object>(h, "mono_field_get_value_object");
    m_gchandle_new               = Resolve<p_mono_gchandle_new>(h, "mono_gchandle_new");
    return m_get_root_domain && m_domain_get && m_thread_attach && m_assembly_foreach &&
           m_assembly_get_image && m_image_get_name && m_class_from_name &&
           m_class_get_method_from_name && m_lookup_internal_call &&
           m_class_get_field_from_name && m_field_get_offset && m_string_new_utf16;
}

struct AsmSearch { const char* want; MonoImage* img; };
static void AsmCb(MonoAssembly* ass, void* ud)
{
    AsmSearch* s = (AsmSearch*)ud;
    if (s->img) return;
    MonoImage* img = m_assembly_get_image(ass);
    if (!img) return;
    const char* nm = m_image_get_name(img);
    if (!nm) return;
    if (_stricmp(nm, s->want) == 0) s->img = img;
}

static MonoImage* FindImage(const char* name)
{
    AsmSearch s = { name, nullptr };
    m_assembly_foreach(AsmCb, &s);
    return s.img;
}

struct DumpCtx { int n; };
static void DumpCb(MonoAssembly* ass, void* ud)
{
    DumpCtx* c = (DumpCtx*)ud;
    MonoImage* img = m_assembly_get_image(ass);
    if (img) { LogA("[asm] %s", m_image_get_name(img)); c->n++; }
}
static void DumpAssemblies()
{
    DumpCtx c = { 0 };
    m_assembly_foreach(DumpCb, &c);
    LogA("[asm] %d assemblies", c.n);
}

// -------------------------------------------------------------- title -------
struct FindWnd { DWORD pid; HWND hwnd; };
static BOOL CALLBACK EnumWnd(HWND h, LPARAM lp)
{
    FindWnd* f = (FindWnd*)lp;
    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    if (pid == f->pid && IsWindowVisible(h) && GetWindow(h, GW_OWNER) == nullptr) {
        wchar_t cls[64] = {0};
        GetClassNameW(h, cls, 63);
        if (wcscmp(cls, L"UnityWndClass") == 0) { f->hwnd = h; return FALSE; }
    }
    return TRUE;
}

static DWORD WINAPI TitleThread(LPVOID)
{
    for (int i = 0; i < 900; i++) {
        // The title follows the Chinese/English switch, so the patch identity
        // shows while Chinese is active and the original name comes back in English.
        const std::wstring& want = (g_langZh || g_titleEn.empty()) ? g_title : g_titleEn;
        if (!want.empty()) {
            FindWnd f = { GetCurrentProcessId(), nullptr };
            EnumWindows(EnumWnd, (LPARAM)&f);
            if (f.hwnd) {
                wchar_t cur[512] = {0};
                GetWindowTextW(f.hwnd, cur, 511);
                if (wcscmp(cur, want.c_str()) != 0) {
                    SetWindowTextW(f.hwnd, want.c_str());
                    LogA("[title] set to %s", WideToUtf8(want).c_str());
                }
                Sleep(1000);
                continue;
            }
        }
        Sleep(200);
    }
    return 0;
}

// -------------------------------------------------------------- ini ---------
static void ReadIni()
{
    FILE* f = _wfopen(g_iniPath.c_str(), L"rb");
    if (!f) { LogA("[ini] not found, using defaults"); return; }
    std::string buf;
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz > 0) { buf.resize((size_t)sz); fread(&buf[0], 1, (size_t)sz, f); }
    fclose(f);
    size_t p = 0;
    while (p < buf.size()) {
        size_t e = buf.find('\n', p);
        if (e == std::string::npos) e = buf.size();
        std::string ln = buf.substr(p, e - p); p = e + 1;
        while (!ln.empty() && (ln.back() == '\r' || ln.back() == ' ')) ln.pop_back();
        if (ln.empty() || ln[0] == '#' || ln[0] == ';' || ln[0] == '[') continue;
        size_t eq = ln.find('=');
        if (eq == std::string::npos) continue;
        std::string k = ln.substr(0, eq), v = ln.substr(eq + 1);
        while (!k.empty() && k.back() == ' ') k.pop_back();
        while (!v.empty() && v[0] == ' ') v.erase(0, 1);
        // allow trailing "  ; comment" / "  # comment" so hook.ini stays self-documenting
        size_t c = v.find_first_of("#;");
        if (c != std::string::npos) v = v.substr(0, c);
        while (!v.empty() && (v.back() == ' ' || v.back() == '\t')) v.pop_back();
        if      (k == "mode")      g_probeOnly = (v == "probe");
        else if (k == "log_miss")  g_logMiss   = (v != "0");
        else if (k == "log_all")   g_logAll    = (v != "0");
        else if (k == "tray")      g_trayOn    = (v != "0");
        else if (k == "lang")      InterlockedExchange(&g_langZh, (v == "en") ? 0 : 1);
        else if (k == "hotkey")    g_hotkeyText = Utf8ToWide(v);
        else if (k == "title")     g_title     = Utf8ToWide(v);
        else if (k == "title_en")  g_titleEn   = Utf8ToWide(v);
    }
    LogA("[ini] mode=%s log_miss=%d", g_probeOnly ? "probe" : "translate", (int)g_logMiss);
    LogA("[ini] title=%s", WideToUtf8(g_title).c_str());
}

// ------------------------------------------------------------ tray/hotkey ---
// A Chinese/English switch. The application's own IMGUI layout cannot be
// extended without patching its assembly, so the switch lives outside it: a
// notification-area icon (left click toggles, right click opens a menu) plus an
// optional global hotkey. Both work in fullscreen, unlike an overlay window.
#define WM_TRAYICON  (WM_APP + 1)
#define IDM_ZH       40001
#define IDM_EN       40002
#define IDM_RELOAD   40003
#define IDM_SITE     40004
#define HOTKEY_ID    0xB001

static const wchar_t* kSiteUrl = L"https://space.bilibili.com/177308205";
static const wchar_t* kSiteName = L"星空汉化 · B站主页";
static const wchar_t* kTipText = L"Materialize 星空汉化  (左键切换中/英，右键菜单)";

static void ToggleLang(HWND h);

static void UpdateTray(HWND h)
{
    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(nid);
    nid.hWnd = h;
    nid.uID = 1;
    nid.uFlags = NIF_TIP | NIF_ICON;
    nid.hIcon = g_icon;
    wcscpy_s(nid.szTip, kTipText);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

static void ToggleLang(HWND h)
{
    LONG v = InterlockedCompareExchange(&g_langZh, 0, 1);
    if (v == 1) InterlockedExchange(&g_langZh, 0);
    else        InterlockedExchange(&g_langZh, 1);
    LogA("[lang] switched to %s", g_langZh ? "zh" : "en");

    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(nid);
    nid.hWnd = h;
    nid.uID = 1;
    nid.uFlags = NIF_INFO;
    wcscpy_s(nid.szInfoTitle, g_langZh ? L"已切换为中文  ·  星空汉化" : L"Switched to English");
    wcscpy_s(nid.szInfo, g_langZh ? L"界面已恢复中文显示" : L"Interface restored to English");
    nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &nid);
    UpdateTray(h);
}

static void ShowMenu(HWND h)
{
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, 0, L"Materialize 星空汉化");
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m, MF_STRING | (g_langZh ? MF_CHECKED : 0), IDM_ZH, L"中文");
    AppendMenuW(m, MF_STRING | (!g_langZh ? MF_CHECKED : 0), IDM_EN, L"English");
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m, MF_STRING, IDM_RELOAD, L"重新载入词典");
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m, MF_STRING, IDM_SITE, kSiteName);
    POINT p; GetCursorPos(&p);
    SetForegroundWindow(h);
    TrackPopupMenu(m, TPM_RIGHTBUTTON, p.x, p.y, 0, h, nullptr);
    PostMessage(h, WM_NULL, 0, 0);
    DestroyMenu(m);
}

static LRESULT CALLBACK TrayProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == g_wmTaskbarCreated) {
        NOTIFYICONDATAW nid = {0};
        nid.cbSize = sizeof(nid); nid.hWnd = h; nid.uID = 1;
        nid.uFlags = NIF_TIP | NIF_ICON | NIF_MESSAGE;
        nid.uCallbackMessage = WM_TRAYICON;
        nid.hIcon = g_icon;
        wcscpy_s(nid.szTip, kTipText);
        Shell_NotifyIconW(NIM_ADD, &nid);
        return 0;
    }
    switch (msg) {
    case WM_TRAYICON:
        if (lp == WM_LBUTTONUP) ToggleLang(h);
        else if (lp == WM_RBUTTONUP) ShowMenu(h);
        return 0;
    case WM_HOTKEY:
        if (wp == HOTKEY_ID) ToggleLang(h);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDM_ZH: InterlockedExchange(&g_langZh, 1); UpdateTray(h); break;
        case IDM_EN: InterlockedExchange(&g_langZh, 0); UpdateTray(h); break;
        case IDM_RELOAD: LoadDict(true); break;
        case IDM_SITE:
            ShellExecuteW(nullptr, L"open", kSiteUrl, nullptr, nullptr, SW_SHOWNORMAL);
            break;
        }
        return 0;
    case WM_DESTROY:
        Shell_NotifyIconW(NIM_DELETE, nullptr);
        return 0;
    }
    return DefWindowProcW(h, msg, wp, lp);
}

// "Ctrl+Alt+Z" / "F12" / "Ctrl+Shift+L" -> modifiers + virtual key
static bool ParseHotkey(const std::wstring& spec)
{
    if (spec.empty()) return false;
    UINT mods = 0, vk = 0;
    std::wstring s = spec, part;
    size_t pos;
    while ((pos = s.find(L'+')) != std::wstring::npos) {
        part = s.substr(0, pos);
        s = s.substr(pos + 1);
        for (auto& c : part) c = towupper(c);
        if      (part == L"CTRL")  mods |= MOD_CONTROL;
        else if (part == L"ALT")   mods |= MOD_ALT;
        else if (part == L"SHIFT") mods |= MOD_SHIFT;
        else if (part == L"WIN")   mods |= MOD_WIN;
        else return false;
    }
    if (s.empty()) return false;
    for (int i = 1; i <= 24; i++) {
        wchar_t buf[8]; swprintf_s(buf, L"F%d", i);
        if (_wcsicmp(s.c_str(), buf) == 0) { vk = VK_F1 + i - 1; break; }
    }
    if (!vk && s.size() == 1 && iswalnum(s[0])) vk = towupper(s[0]);
    if (!vk) return false;
    g_hotkeyMods = mods; g_hotkeyVk = vk; g_hotkeyText = spec;
    return true;
}

static DWORD WINAPI TrayThread(LPVOID)
{
    g_wmTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = TrayProc;
    wc.hInstance = g_self;
    wc.lpszClassName = L"MaterializeCN_Tray";
    RegisterClassExW(&wc);
    g_hwnd = CreateWindowExW(0, wc.lpszClassName, L"MaterializeCN", 0, 0, 0, 0, 0,
                             HWND_MESSAGE, nullptr, g_self, nullptr);
    if (!g_hwnd) { LogA("[tray] CreateWindow failed %lu", GetLastError()); return 1; }

    wchar_t exe[MAX_PATH] = {0};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    ExtractIconExW(exe, 0, &g_icon, nullptr, 1);
    if (!g_icon) g_icon = LoadIconW(nullptr, IDI_APPLICATION);

    if (g_trayOn) {
        NOTIFYICONDATAW nid = {0};
        nid.cbSize = sizeof(nid);
        nid.hWnd = g_hwnd;
        nid.uID = 1;
        nid.uFlags = NIF_TIP | NIF_ICON | NIF_MESSAGE;
        nid.uCallbackMessage = WM_TRAYICON;
        nid.hIcon = g_icon;
        wcscpy_s(nid.szTip, kTipText);
        BOOL ok = Shell_NotifyIconW(NIM_ADD, &nid);
        LogA("[tray] add icon: %s", ok ? "ok" : "FAILED");
    }

    if (g_hotkeyVk) {
        BOOL ok = RegisterHotKey(g_hwnd, HOTKEY_ID, g_hotkeyMods | MOD_NOREPEAT, g_hotkeyVk);
        LogA("[hotkey] %s: %s", ok ? "registered" : "FAILED", WideToUtf8(g_hotkeyText).c_str());
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}

// ------------------------------------------------------------ bootstrap -----
struct HookSpec {
    const char* cls; const char* name; int argc;
    void* hook; void** orig; const char* label; bool installed;
};

static DWORD WINAPI Bootstrap(LPVOID)
{
    g_log = _wfopen(g_logPath.c_str(), L"wb");
    LogA("=== MaterializeCN ===");
    LogA("[env] root=%s", WideToUtf8(g_root).c_str());

    ReadIni();

    HMODULE mono = nullptr;
    for (int i = 0; i < 6000 && !mono; i++) {
        mono = GetModuleHandleW(L"mono.dll");
        if (!mono) mono = GetModuleHandleW(L"mono-2.0-bdwgc.dll");
        if (!mono) Sleep(10);
    }
    if (!mono) { LogA("[mono] mono.dll never loaded"); return 1; }
    LogA("[mono] module=%p", mono);
    if (!BindMono(mono)) { LogA("[mono] binding incomplete, abort"); return 1; }

    MonoDomain* dom = nullptr;
    for (int i = 0; i < 12000 && !dom; i++) { dom = m_get_root_domain(); if (!dom) Sleep(10); }
    if (!dom) { LogA("[mono] root domain never appeared"); return 1; }
    LogA("[mono] root domain=%p", dom);

    m_thread_attach(dom);
    LogA("[mono] thread attached");

    LoadDict(true);

    MonoClass* gui = nullptr; MonoClass* style = nullptr; MonoClass* content = nullptr;
    MonoImage* img = nullptr;
    for (int i = 0; i < 6000; i++) {
        if (!img) img = FindImage("UnityEngine.IMGUIModule.dll");
        if (!img) img = FindImage("UnityEngine.IMGUIModule");
        if (img) {
            if (!gui)     gui     = m_class_from_name(img, "UnityEngine", "GUI");
            if (!style)   style   = m_class_from_name(img, "UnityEngine", "GUIStyle");
            if (!content) content = m_class_from_name(img, "UnityEngine", "GUIContent");
            if (content && g_textFieldOff < 0) {
                MonoClassField* fld = m_class_get_field_from_name(content, "m_Text");
                if (fld) {
                    g_textFieldOff = m_field_get_offset(fld);
                    LogA("[hook] GUIContent.m_Text field offset = %d", g_textFieldOff);
                }
            }
            if (gui && style && content && g_textFieldOff >= 0) break;
        }
        if (i == 500 && !img) DumpAssemblies();
        Sleep(10);
    }
    if (!gui || !style || !content || g_textFieldOff < 0) {
        LogA("[hook] resolution FAILED gui=%p style=%p content=%p off=%d", gui, style, content, g_textFieldOff);
        return 1;
    }
    LogA("[hook] classes resolved (gui=%p style=%p content=%p)", gui, style, content);

    HookSpec specs[] = {
      { "GUI",     "INTERNAL_CALL_DoLabel",                3, (void*)H_DoLabel,       (void**)&o_DoLabel,       "GUI.DoLabel",            false },
      { "GUI",     "INTERNAL_CALL_DoButton",               3, (void*)H_DoButton,      (void**)&o_DoButton,      "GUI.DoButton",           false },
      { "GUI",     "INTERNAL_CALL_DoToggle",               5, (void*)H_DoToggle,      (void**)&o_DoToggle,      "GUI.DoToggle",           false },
      { "GUI",     "INTERNAL_CALL_Internal_DoWindow",      9, (void*)H_DoWindow,      (void**)&o_DoWindow,      "GUI.DoWindow",           false },
      { "GUI",     "INTERNAL_CALL_Internal_DoModalWindow", 8, (void*)H_DoModalWindow, (void**)&o_DoModalWindow, "GUI.DoModalWindow",      false },
      { "GUIStyle","Internal_Draw",                        2, (void*)H_InternalDraw,  (void**)&o_InternalDraw,  "GUIStyle.Internal_Draw", false },
      { "GUIStyle","INTERNAL_CALL_Internal_Draw2",         5, (void*)H_InternalDraw2, (void**)&o_InternalDraw2, "GUIStyle.Internal_Draw2",false },
    };
    const int N = (int)(sizeof(specs) / sizeof(specs[0]));

    int resolved = 0;
    // Bounded wait: attach whatever Unity has already registered rather than
    // blocking the app forever on an icall that may never be used.
    for (int pass = 0; pass < 80 && resolved < N; pass++) {
        resolved = 0;
        for (int i = 0; i < N; i++) {
            if (specs[i].installed) { resolved++; continue; }
            MonoClass* k = (strcmp(specs[i].cls, "GUI") == 0) ? gui : style;
            MonoMethod* m = m_class_get_method_from_name(k, specs[i].name, specs[i].argc);
            if (!m) continue;
            void* fn = m_lookup_internal_call(m);
            if (!fn) continue;
            LogA("[hook] %-26s argc=%d method=%p native=%p", specs[i].label, specs[i].argc, m, fn);
            *specs[i].orig = fn;
            specs[i].installed = true;
            resolved++;
        }
        if (resolved < N && pass % 25 == 0)
            LogA("[hook] %d/%d resolved, waiting...", resolved, N);
        if (resolved < N) Sleep(100);
    }

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    int attached = 0;
    for (int i = 0; i < N; i++) {
        if (!specs[i].installed) { LogA("[hook] NOT INSTALLED: %s", specs[i].label); continue; }
        void* orig = *specs[i].orig;
        if (DetourAttach(specs[i].orig, specs[i].hook) == NO_ERROR) attached++;
        else LogA("[hook] DetourAttach FAILED: %s", specs[i].label);
        (void)orig;
    }
    LONG err = DetourTransactionCommit();
    LogA("[hook] attached %d/%d icalls, commit=%ld", attached, N, err);

    if (!g_title.empty() || !g_titleEn.empty())
        CreateThread(nullptr, 0, TitleThread, nullptr, 0, nullptr);

    if (!g_hotkeyText.empty() && !ParseHotkey(g_hotkeyText))
        LogA("[hotkey] could not parse \"%s\"", WideToUtf8(g_hotkeyText).c_str());
    CreateThread(nullptr, 0, TrayThread, nullptr, 0, nullptr);
    LogA("[lang] initial=%s tray=%d hotkey=%s", g_langZh ? "zh" : "en", (int)g_trayOn,
         WideToUtf8(g_hotkeyText).c_str());

    for (int t = 0;; t++) {
        Sleep(2000);
        LoadDict(false);
        if (t % 15 == 14)
            LogA("[stat] calls=%lld hits=%lld dict=%zu",
                 (long long)g_hookCalls, (long long)g_hookHits, g_dictSize);
    }
    return 0;
}

// -------------------------------------------------------------- DllMain -----
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = hinst;
        DisableThreadLibraryCalls(hinst);
        InitializeCriticalSection(&g_logLock);

        wchar_t p[MAX_PATH] = {0};
        GetModuleFileNameW(hinst, p, MAX_PATH);
        std::wstring s(p);
        size_t b = s.find_last_of(L"\\/");
        g_root     = (b == std::wstring::npos) ? L"." : s.substr(0, b);
        g_logPath  = g_root + L"\\_hanhua\\logs\\hook.log";
        g_dictPath = g_root + L"\\_hanhua\\dict.tsv";
        g_iniPath  = g_root + L"\\_hanhua\\hook.ini";

        CreateDirectoryW((g_root + L"\\_hanhua").c_str(), nullptr);
        CreateDirectoryW((g_root + L"\\_hanhua\\logs").c_str(), nullptr);

        g_missed = new std::unordered_set<std::string>();
        g_drawn  = new std::unordered_set<std::string>();
        HANDLE h = CreateThread(nullptr, 0, Bootstrap, nullptr, 0, nullptr);
        if (h) CloseHandle(h);
    }
    return TRUE;
}
