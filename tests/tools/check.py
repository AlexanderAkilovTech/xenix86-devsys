"""check.py - make the list of the set's installed files for crc (tests/tools/crc.c)
to read on XENIX, and check what it found there against the set as built (M64 on).
    python3 tests/tools/check.py list DIR     -> DIR/files.list, for a test disk
    python3 tests/tools/check.py report FILE  -> each file: the same, or how not"""
import os, sys, zlib
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
R = f'{ROOT}/build/xenix/root'
IDS = {'root': 0, 'bin': 3, 'sysinfo': 10}
# files that change once installed: lastlogin's records, xtop's kept symbols
CHANGES = {'./usr/adm/lastlog', './usr/lib/xtop.syms'}

def entries():
    """(path, mode, links, uid, gid) of each name in soft.perms, links included"""
    out, cur = [], None
    for l in open(f'{ROOT}/build/xenix/set/soft.perms'):
        if l.startswith('#') or not l.strip() or l.startswith('uid') or l.startswith('gid'):
            continue
        f = l.rstrip('\n').split('\t')
        if f[0]:
            fields = [x for x in f if x]
            pkg, tm, own, nl, path = fields[0], fields[1], fields[2], int(fields[3]), fields[4]
            typ = {'x': 0o100000, 'f': 0o100000, 'd': 0o040000}[tm[0]]
            cur = (typ | int(tm[1:], 8), nl, IDS[own.split('/')[0]], IDS[own.split('/')[1]])
            if pkg == 'PERM':           # the list itself: checked against the set's copy
                cur = None
            else:
                out.append((path,) + cur)
        else:
            path = [x for x in f if x][0]
            if cur:
                out.append((path,) + cur)
    return out

if sys.argv[1] == 'list':
    with open(f'{sys.argv[2]}/files.list', 'w') as o:
        o.write('/etc/soft.perms\n/etc/perms/soft\n')
        for e in entries():
            o.write(e[0][1:] + '\n')
    print(len(entries()) + 2, 'names')
else:
    got = {}
    crc = True                          # only crc's section: others have paths too
    for l in open(sys.argv[2], encoding='latin1'):
        if l.startswith('+++ '):
            crc = 'installed files' in l
            continue
        if not crc:
            continue
        w = l.split()
        if len(w) >= 2 and w[0].startswith('/'):
            got[w[0]] = w[1:]
    bad = 0
    perms = open(f'{ROOT}/build/xenix/set/soft.perms', 'rb').read()
    want = [('/etc/soft.perms', 0o100644, 2, None, None, perms), ('/etc/perms/soft', 0o100644, 2, None, None, perms)]
    for path, mode, nl, uid, gid in entries():
        data = open(R + path[1:], 'rb').read() if mode & 0o100000 else None
        want.append((path[1:], mode, nl, uid, gid, data))
    for path, mode, nl, uid, gid, data in want:
        g = got.get(path)
        if g is None:
            print('NOT CHECKED', path); bad += 1; continue
        if g[0] == 'missing':
            print('MISSING', path); bad += 1; continue
        gm, gn, gu, gg, gs = int(g[0], 8), int(g[1]), int(g[2]), int(g[3]), int(g[4])
        why = []
        if path.startswith('/etc/'):     # custom's copy: its mode and owner are custom's
            pass
        else:
            if gm != mode: why.append(f'mode {gm:o}, not {mode:o}')
            if gn != nl and mode & 0o100000: why.append(f'{gn} links, not {nl}')   # a directory's are its own
            if (gu, gg) != (uid, gid): why.append(f'owner {gu}/{gg}, not {uid}/{gid}')
        if data is not None and '.' + path not in CHANGES:
            if gs != len(data): why.append(f'{gs} bytes, not {len(data)}')
            elif len(g) < 6 or g[5] != '%08x' % zlib.crc32(data): why.append('different bytes')
        if why:
            print('DIFFERENT', path + ':', ', '.join(why)); bad += 1
    print(f'{len(want)} names, {bad} not as built' + (': ALL THE SAME' if not bad else ''))
