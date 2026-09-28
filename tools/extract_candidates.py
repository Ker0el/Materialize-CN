# -*- coding: utf-8 -*-
"""Extract translatable UI strings from the decompiled Assembly-CSharp.

Two outputs:
  candidates.txt  - complete string literals passed to GUI.*/GUILayout.*
  fragments.txt   - literal pieces of concatenated/dynamic expressions, for review
  nonui.txt       - literals seen in GUI calls that look like data, not labels
"""
import os, re, sys, collections

HERE = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.join(HERE, '..', 'src', 'dnspy_acs', 'Assembly-CSharp')
OUT = os.path.join(HERE, '..', 'dict')
QUOTE = '"'
BS = chr(92)

# which argument index holds the display text, per API.
# GuiHelper/ShowBrowser matter as much as GUI.* -- Materialize passes most
# parameter-panel captions straight into GuiHelper.Slider(rect, "Title", ...),
# so an extractor that only looks at GUI.* misses them entirely.
TEXT_ARG = {
    'GUI.Label':          [1],
    'GUI.Button':         [1],
    'GUI.Box':            [1],
    'GUI.Window':         [3],
    'GUI.Toggle':         [2],
    'GUI.SelectionGrid':  [2],
    'GUILayout.Label':    [0],
    'GUILayout.Button':   [0],
    'GUILayout.Box':      [0],
    'GUILayout.Toggle':   [1],
    'GUILayout.BeginArea':[1],
    'GUILayout.Window':   [3],
    'GuiHelper.Slider':   [1],   # (rect, title, ...)
    'GuiHelper.Toggle':   [3],   # (rect, value, out, Text, ...)
    'ShowBrowser':        [0],   # file-dialog caption
}

# things that are data, not UI copy
DATA_RE = re.compile(r'^[\s\d.,:%+\-/*()\[\]]*$')
EXT_RE = re.compile(r'^\.?[A-Za-z0-9]{2,5}$')


def split_args(s):
    out, d, cur, ins, esc = [], 0, '', False, False
    for ch in s:
        if ins:
            cur += ch
            if esc: esc = False
            elif ch == BS: esc = True
            elif ch == QUOTE: ins = False
            continue
        if ch == QUOTE:
            ins = True; cur += ch; continue
        if ch in '([{': d += 1
        elif ch in ')]}': d -= 1
        if ch == ',' and d == 0:
            out.append(cur.strip()); cur = ''; continue
        cur += ch
    if cur.strip(): out.append(cur.strip())
    return out


def literals_in(expr):
    """Return (full_literal, [fragments]) - full_literal is set only when the whole
    expression is one plain string literal."""
    frags, d = [], 0
    i = 0
    simple = True
    while i < len(expr):
        ch = expr[i]
        if ch == QUOTE:
            j = i + 1
            buf = []
            while j < len(expr):
                if expr[j] == BS:
                    buf.append(expr[j:j + 2]); j += 2; continue
                if expr[j] == QUOTE:
                    break
                buf.append(expr[j]); j += 1
            frags.append(''.join(buf))
            i = j + 1
            continue
        if ch in '([{':
            d += 1
        elif ch in ')]}':
            d -= 1
        elif d == 0 and expr[i:i + 2] == '+=' :
            simple = False
        elif d == 0 and ch == '+':
            simple = False
        i += 1
    full = expr.strip()
    if simple and full.startswith(QUOTE) and full.endswith(QUOTE):
        return frags[0] if frags else None, frags
    return None, frags


def unescape(s):
    out, i = [], 0
    while i < len(s):
        if s[i] == BS and i + 1 < len(s):
            c = s[i + 1]
            out.append({'n': '\n', 'r': '\r', 't': '\t', BS: BS, '"': '"', "'": "'",
                        '0': ''}[c] if c in 'nrt' + BS + '"' + "'" + '0' else c)
            i += 2
        else:
            out.append(s[i]); i += 1
    return ''.join(out)


def main():
    os.makedirs(OUT, exist_ok=True)
    cand = collections.Counter()
    frag = collections.Counter()
    where = {}
    apicalls = re.compile(r'\b((?:\w+\.)?\w+)\s*\(')

    for root, _, fs in os.walk(BASE):
        for fn in fs:
            if not fn.endswith('.cs'):
                continue
            path = os.path.join(root, fn)
            t = open(path, encoding='utf-8-sig', errors='replace').read()
            for m in apicalls.finditer(t):
                api = m.group(1)
                if api not in TEXT_ARG:
                    continue
                i, d, ins, esc = m.end(), 1, False, False
                while i < len(t) and d > 0:
                    ch = t[i]
                    if ins:
                        if esc: esc = False
                        elif ch == BS: esc = True
                        elif ch == QUOTE: ins = False
                    else:
                        if ch == QUOTE: ins = True
                        elif ch in '([{': d += 1
                        elif ch in ')]}': d -= 1
                    i += 1
                args = split_args(t[m.end():i - 1])
                for idx in TEXT_ARG[api]:
                    if idx >= len(args):
                        continue
                    full, frags = literals_in(args[idx])
                    if full is not None:
                        u = unescape(full)
                        if u.strip():
                            cand[u] += 1
                            where.setdefault(u, '%s:%s' % (fn, api))
                    else:
                        for f in frags:
                            u = unescape(f)
                            if u.strip():
                                frag[u] += 1
                                where.setdefault(u, '%s:%s(concat)' % (fn, api))

    def esc(k):
        # TSV is one entry per line: newlines/tabs/backslashes must be escaped,
        # otherwise a multi-line label silently splits and its halves never match.
        return (k.replace(chr(92), chr(92) * 2)
                 .replace('\r', chr(92) + 'r')
                 .replace('\n', chr(92) + 'n')
                 .replace('\t', chr(92) + 't'))

    def dump(name, counter):
        p = os.path.join(OUT, name)
        with open(p, 'w', encoding='utf-8') as f:
            for k in sorted(counter, key=lambda x: x.lower()):
                f.write('%s\t%s\n' % (esc(k), where.get(k, '')))
        print('%-16s %d  -> %s' % (name, len(counter), os.path.normpath(p)))

    dump('candidates.txt', cand)
    dump('fragments.txt', frag)

    # simple report on what looks like data rather than copy
    print()
    print('--- single tokens / likely data (review before translating) ---')
    for k in sorted(cand, key=lambda x: x.lower()):
        if len(k) <= 3 or DATA_RE.match(k):
            print('   %-24r x%d  %s' % (k, cand[k], where.get(k, '')))


if __name__ == '__main__':
    main()
