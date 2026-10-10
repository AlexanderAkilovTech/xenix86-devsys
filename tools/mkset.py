"""mkset.py - make the Development System into a set that XENIX's custom installs.

    python3 tools/mkset.py [root] [outdir]

root (default build/xenix/root, made by dist.sh) is the tree as it installs.
Writes outdir/soft.perms and the disk images outdir/D01.img, D02.img, ... (360K).

custom's built-in "Development System" (custom -d) installs two lists: the
Operating System's /etc/dsmd.perms, whose SOFT package (adb and /usr/include/sys)
comes from SCO's own disk N05, and /etc/soft.perms, which these disks supply.
The format follows SCO's 2.1.3 sets: a label file ./tmp/_lbl/prd=xnxsv/ver=ALL86/
rel=2.1.3/vol=D01 first on each disk, the permissions list (with its second
name ./etc/perms/soft as a hard link) on D01, then the files, in a version 7 tar
with ./ names, owned by bin. Package sizes are 512-byte disk blocks."""
import os, sys, struct

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
root = sys.argv[1] if len(sys.argv) > 1 else f'{ROOT}/build/xenix/root'
out = sys.argv[2] if len(sys.argv) > 2 else f'{ROOT}/build/xenix/set'
DISK = 368640
MTIME = 504921600          # 1 January 1986: XENIX's clock cannot pass 1999
LABEL = './tmp/_lbl/prd=xnxsv/ver=ALL86/rel=2.1.3/vol={}'
BIN = 3                    # uid and gid of bin

# package, type, mode, path (relative to root). Directories have no volume.
def entries():
    e = [('SOFT', 'd', 0o755, './usr/include'), ('SOFT', 'd', 0o755, './usr/lib/cc')]
    for d, typ, mode in (('usr/bin', 'x', 0o711), ('usr/lib/cc', None, None), ('usr/include', 'f', 0o644)):
        for f in sorted(os.listdir(f'{root}/{d}')):
            p = f'{root}/{d}/{f}'
            if not os.path.isfile(p):
                continue
            t, m = (typ, mode) if typ else (('x', 0o711) if os.access(p, os.X_OK) else ('f', 0o644))
            e.append(('SOFT', t, m, f'./{d}/{f}'))
    return e

def blocks(size):
    """512-byte blocks a file takes on a XENIX file system, with indirect blocks"""
    n = (size + 511) // 512
    if n > 10:
        n += 1 + max(0, (n - 10 - 128 + 127) // 128)
    return n

# ---- version 7 tar ----
def octal(n, width):
    return (b'%0*o' % (width - 1, n)) + b'\0'

def header(name, mode, uid, gid, size, link=None):
    h = bytearray(512)
    h[0:len(name)] = name.encode()
    h[100:108] = octal(mode, 8)
    h[108:116] = octal(uid, 8)
    h[116:124] = octal(gid, 8)
    h[124:136] = octal(size, 12)
    h[136:148] = octal(MTIME, 12)
    h[148:156] = b' ' * 8
    if link:
        h[156] = ord('1')
        h[157:157 + len(link)] = link.encode()
    s = sum(h)
    h[148:156] = b'%06o\0 ' % s
    return bytes(h)

def member(name, data, mode, uid=BIN, gid=BIN):
    return header(name, mode, uid, gid, len(data)) + data + bytes(-len(data) % 512)

def tarsize(n):
    return 512 + (n + 511) // 512 * 512

# ---- build the set ----
os.makedirs(out, exist_ok=True)
files = entries()
sizes = {p: os.path.getsize(f'{root}/{p}') for pkg, t, m, p in files if t != 'd'}

# assign files to volumes; D01 also carries the label and the permissions list
PERMS_RESERVE = 8192
vols, cur, used = [], [], 512 * 2 + 1024 + PERMS_RESERVE
for pkg, t, m, p in files:
    if t == 'd':
        continue
    need = tarsize(sizes[p])
    if used + need > DISK - 1024:
        vols.append(cur)
        cur, used = [], 512 * 2 + 1024
    cur.append(p)
    used += need
vols.append(cur)
volof = {p: f'D{i + 1:02d}' for i, v in enumerate(vols) for p in v}

pkgblocks = {}
for pkg, t, m, p in files:
    if t != 'd':
        pkgblocks[pkg] = pkgblocks.get(pkg, 0) + blocks(sizes[p])

def perms_text(permsize):
    pk = {'PERM': blocks(permsize), 'SOFT': pkgblocks['SOFT']}
    L = ['# soft.perms - the Development System for XENIX 2.1.3 (8086): xenix86-devsys,',
         '# an open source recreation (GPL). Not SCO software.',
         '#', '#prd=xnxsv', '#ver=ALL86', '#rel=2.1.3', '#',
         '# User and group ids:', '#']
    for n, i in (('root', 0), ('bin', 3), ('uucp', 4), ('asg', 6), ('sysinfo', 10), ('network', 12), ('lp', 14)):
        L.append(f'uid\t{n}\t\t{i}')
    L.append('#')
    for n, i in (('root', 0), ('bin', 3), ('uucp', 4), ('asg', 6), ('sysinfo', 10), ('network', 12)):
        L.append(f'gid\t{n}\t\t{i}')
    L += ['#', '# Fields are: package [d,f,x]mode, user/group, links, path, volume', '#',
          f'#!ALL\t{sum(pk.values())}\tEntire Development System set',
          f'#!PERM\t{pk["PERM"]}\tDevelopment System contents and permissions lists',
          'PERM\tf644\tbin/bin\t\t2\t./etc/soft.perms\t\tD01',
          '\t\t\t\t\t./etc/perms/soft\t\tD01',
          f'#!SOFT\t{pk["SOFT"]}\tC compiler, assembler, linker, make, libraries, headers',
          '#\t(with adb and /usr/include/sys from the Operating System disk N05)']
    for pkg, t, m, p in files:
        tabs = '\t' * max(1, 4 - len(p) // 8)
        if t == 'd':
            L.append(f'{pkg}\t{t}{m:o}\tbin/bin\t\t1\t{p}')
        else:
            L.append(f'{pkg}\t{t}{m:o}\tbin/bin\t\t1\t{p}{tabs}{volof[p]}')
    return '\n'.join(L) + '\n'

text = perms_text(4096)
text = perms_text(len(text))           # its own size, in blocks, settles at once
open(f'{out}/soft.perms', 'w').write(text)

for i, v in enumerate(vols):
    vol = f'D{i + 1:02d}'
    img = bytearray(member(LABEL.format(vol), b'', 0o444, 0, 0))
    if i == 0:
        img += member('./etc/soft.perms', text.encode(), 0o644)
        img += header('./etc/perms/soft', 0o644, BIN, BIN, 0, link='./etc/soft.perms')
    for p in v:
        mode = next(m for pkg, t, m, q in files if q == p)
        img += member(p, open(f'{root}/{p}', 'rb').read(), mode)
    img += bytes(1024)
    if len(img) > DISK:
        raise SystemExit(f'{vol}: {len(img)} bytes, more than a 360K disk')
    img += bytes(DISK - len(img))
    open(f'{out}/{vol}.img', 'wb').write(img)
    print(f'{out}/{vol}.img: {len(v)} files')
print(f'{out}/soft.perms: {len(files)} entries; SOFT {pkgblocks["SOFT"]} blocks')
