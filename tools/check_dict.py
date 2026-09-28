# -*- coding: utf-8 -*-
"""Cross-check the hand-written dictionary against every UI string the extractor
found, so nothing silently falls through to English at runtime."""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
D = os.path.join(HERE, '..', 'dict')
BS = chr(92)

# single letters / channel markers / file-format tokens intentionally kept as-is
KEEP = set(['BMP', 'JPG', 'PNG', 'TGA', 'TIFF', 'P', 'C', 'O', 'S',
            'R', 'G', 'B', 'R:', 'G:', 'B:', '-', '', ' '])

# real filesystem names the file browser renders as-is - these are paths, not copy
def is_noise(k):
    if k in KEEP:
        return True
    if re.match(r'^[A-Za-z]:$', k):          # bare drive letter
        return True
    if os.sep in k or '/' in k:
        return True
    if re.match(r'^[\w .\-一-鿿]+\.\w{2,4}$', k):   # filename.ext
        return True
    return False


def unesc(s):
    out, i = [], 0
    while i < len(s):
        if s[i] == BS and i + 1 < len(s):
            c = s[i + 1]
            out.append({'r': '\r', 'n': '\n', 't': '\t', BS: BS}.get(c, c))
            i += 2
        else:
            out.append(s[i]); i += 1
    return ''.join(out)


def load(path, has_where):
    keys = {}
    if not os.path.exists(path):
        return keys
    for ln in open(path, encoding='utf-8'):
        ln = ln.rstrip('\n')
        if not ln or ln.startswith('#'):
            continue
        parts = ln.split('\t')
        keys[unesc(parts[0])] = (parts[1] if has_where and len(parts) > 1 else '')
    return keys


def main():
    cands = load(os.path.join(D, 'candidates.txt'), True)
    frags = load(os.path.join(D, 'fragments.txt'), True)
    dictp = load(os.path.join(D, 'dict.tsv'), False)
    # runtime gaps collected by the hook (may not exist yet)
    miss = load(os.path.join(D, '..', 'logs', 'miss.log'), False)

    def report(title, items):
        items = [k for k in items if k not in dictp and not is_noise(k)]
        print('\n=== %s: %d not in dictionary ===' % (title, len(items)))
        for k in sorted(items, key=lambda x: x.lower()):
            extra = (cands.get(k) or frags.get(k) or '')
            print('   %-62r %s' % (k, extra))
        return items

    report('static candidates', list(cands))
    report('runtime misses (miss.log)', list(miss))
    print('\ndictionary entries: %d' % len(dictp))


if __name__ == '__main__':
    main()
