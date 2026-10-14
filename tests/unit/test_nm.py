"""Tests for nm, size and strip (cmd/nm.c, size.c, strip.c, objfile.c): run them on
the simulator over the C and math libraries, their object files and some
executables, and compare with this file's own reading of the formats.

    python3 tests/unit/test_nm.py"""
import glob, os, shutil, struct, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SIM = [sys.executable, f'{ROOT}/tools/xsim.py']

def xout_syms(d):
    """(value, letter, name) for an x.out's symbol table, segment names left out"""
    segpos, segsize = struct.unpack_from('<ll', d, 52)
    out = []
    for i in range(0, segsize, 32):
        t, attr, seg, al, cres, pos, size = struct.unpack_from('<HHHBBll', d, segpos + i)
        if t != 3:
            continue
        s, j = d[pos:pos + size], 0
        while j + 8 < len(s):
            st, sg, v = struct.unpack_from('<HHL', s, j)   # values are unsigned, as nm sorts them
            k = s.index(b'\0', j + 8)
            name, j = s[j + 8:k], k + 1
            if st & 0x1F == 8:
                continue
            c = {0: 'u', 1: 'a', 2: 't', 3: 'd', 4: 'b', 5: 'c'}.get(st & 0x1F, '?')
            out.append((v, c.upper() if st & 0x20 else c, name))
    return out

def obj_syms(d):
    """(value, letter, name) for an as86 object file, and its text, data, bss"""
    assert d[:2] == b'\xa3\x86' and (sum(d[:4]) & 0xFF) == d[4]
    nmod, pos = struct.unpack_from('<H', d, 2)[0], 5
    out, text, data, bss = [], 0, 0, 0
    width = [0, 1, 2, 4]
    def num(n):
        nonlocal pos
        v = int.from_bytes(d[pos:pos + n], 'little'); pos += n; return v
    for _ in range(nmod):
        strsize = struct.unpack_from('<H', d, pos + 8)[0]
        desc = d[pos + 16:pos + 20]
        pos += 20
        for seg in range(16):
            sz = num(width[3 & (desc[(15 - seg) // 4] >> (2 * (seg % 4)))])
            if 1 <= seg <= 4: data += sz
            else: text += sz
        nsym = num(2)
        syms = []
        for _ in range(nsym):
            off, flags = num(2), num(2)
            v = num(width[flags >> 14])
            if flags & 0x40:
                c = 'C' if v else 'U'
            else:
                c = 'a' if flags & 0x10 else ('b' if flags & 0x2000 else 'd') if 1 <= flags & 0xF <= 4 else 't'
                if flags & 0x80: c = c.upper()
            if c in 'Cb': bss += v
            syms.append((v, c, off))
        strtab = d[pos:pos + strsize]; pos += strsize
        for v, c, off in syms:
            out.append((v, c, strtab[off:strtab.index(b'\0', off)]))
    return out, text, data, bss

def members(d):
    """the object file members of an archive"""
    pos, out = 8, []
    while pos + 60 <= len(d):
        h = d[pos:pos + 60]
        size = int(h[48:58])
        name = h[:16].decode().rstrip(' /')
        body = d[pos + 60:pos + 60 + size]
        if name and body[:2] == b'\xa3\x86':
            out.append((name, body))
        pos += 60 + size + (size & 1)
    return out

def nm_lines(syms, opts, where=None):
    syms = list(syms)
    if 'p' not in opts:
        key = (lambda s: (s[0], s[2])) if 'n' in opts else (lambda s: (s[2], s[0]))
        syms.sort(key=key, reverse='r' in opts)
    out = []
    for v, c, name in syms:
        if 'g' in opts and not c.isupper(): continue
        if 'u' in opts and c not in 'Uu': continue
        pre = f'{where}: ' if 'o' in opts else ''
        n = name.decode('latin1')
        out.append(pre + (f'         {c} {n}' if c in 'Uu' else f'{v & 0xFFFFFFFF:08x} {c} {n}'))
    return out

def nm_ref(files, opts):
    lines = []
    for f in files:
        d = open(f'{tmp}/{f}', 'rb').read()
        if d[:8] == b'!<arch>\n':
            for name, body in members(d):
                syms = obj_syms(body)[0]
                where = f'{f}({name})'
                if not syms: continue
                if 'o' not in opts: lines += ['', (name if len(files) == 1 else where) + ':']
                lines += nm_lines(syms, opts, where)
        else:
            syms = xout_syms(d) if d[:2] == b'\x06\x02' else obj_syms(d)[0]
            if not syms: continue
            if len(files) > 1 and 'o' not in opts: lines += ['', f + ':']
            lines += nm_lines(syms, opts, f)
    return lines

def size_ref(files, fmt=''):
    def show(t, d, b):
        if fmt == 'o': return f'0{t:o} + 0{d:o} + 0{b:o} = 0{t + d + b:o}'
        if fmt == 'x': return f'0x{t:x} + 0x{d:x} + 0x{b:x} = 0x{t + d + b:x}'
        return f'{t} + {d} + {b} = {t + d + b}'
    lines = []
    for f in files:
        d = open(f'{tmp}/{f}', 'rb').read()
        if d[:8] == b'!<arch>\n':
            for name, body in members(d):
                _, t, da, b = obj_syms(body)
                lines.append(f'{f}({name}): ' + show(t, da, b))
        else:
            if d[:2] == b'\x06\x02': t, da, b = struct.unpack_from('<lll', d, 4)
            else: _, t, da, b = obj_syms(d)
            lines.append((f'{f}: ' if len(files) > 1 else '') + show(t, da, b))
    return lines

def run(prog, args, cwd):
    r = subprocess.run(SIM + [f'{ROOT}/build/{prog}'] + args, capture_output=True, text=True, cwd=cwd)
    return r.stdout.split('\n')[:-1], r.stderr, r.returncode

for p in ('nm', 'size', 'strip'):
    subprocess.run([f'{ROOT}/build.sh', f'cmd/{p}.c', 'cmd/objfile.c'], cwd=ROOT, check=True, stdout=subprocess.DEVNULL)
for p in ('symt', 'fmtest', 'm12', 'xwc'):
    subprocess.run([f'{ROOT}/build.sh', f'tests/{p}.c'], cwd=ROOT, check=True, stdout=subprocess.DEVNULL)

tmp = tempfile.mkdtemp(prefix='nmtest')
files = ['libc.a', 'libm.a', 'symt', 'fmtest', 'm12', 'xwc', 'nm', 'size', 'strip'] + \
        [f'o/{os.path.basename(f)}' for f in sorted(glob.glob(f'{ROOT}/build/lib/*.o'))[::3]] + ['o/malloc.o']
if 'o/strtod.o' not in files:   # every third module, whichever they are; strtod is named below
    files.append('o/strtod.o')
os.makedirs(f'{tmp}/o')
for f in files:
    src = f'{ROOT}/build/lib/{f[2:]}' if f.startswith('o/') else f'{ROOT}/build/{f}'
    shutil.copy(src, f'{tmp}/{f}')

fails, count = 0, 0
def check(what, want, got):
    global fails, count
    count += 1
    if want != got:
        fails += 1
        if fails <= 8:
            print(f'FAIL {what}')
            for i, (a, b) in enumerate(zip(want, got)):
                if a != b: print(f'  line {i}: want {a!r}\n           got  {b!r}'); break
            else: print(f'  want {len(want)} lines, got {len(got)}')

for opts in ('', '-p', '-n', '-r', '-nr', '-g', '-u', '-o', '-gn', '-po'):
    for group in (['libc.a'], ['libm.a'], ['symt'], ['fmtest', 'symt'], ['o/malloc.o'],
                  files[9:20], ['libm.a', 'symt', 'o/strtod.o']):
        got, err, rc = run('nm', ([opts] if opts else []) + group, tmp)
        check(f'nm {opts} {" ".join(group)}', nm_ref(group, opts.strip('-')), got)
for opts in ('', '-o', '-x'):
    for group in (['symt'], ['libc.a'], files[2:9], files[9:]):
        got, err, rc = run('size', ([opts] if opts else []) + group, tmp)
        check(f'size {opts} {" ".join(group[:3])}', size_ref(group, opts.strip('-')), got)

# size86 (Dev86's) agrees on object files
for f in files[9:]:
    r = subprocess.run(['size86', f], capture_output=True, text=True, cwd=tmp).stdout.split('\n')[1].split('\t')
    _, t, d, b = obj_syms(open(f'{tmp}/{f}', 'rb').read())
    check(f'size86 {f}', r[:3], [str(t), str(d), str(b)])

# strip gives what cc -s gives, and the stripped program still runs
for p in ('symt', 'fmtest', 'm12', 'xwc', 'nm'):
    subprocess.run([sys.executable, f'{ROOT}/tools/aout2xout.py', '--strip', f'{ROOT}/build/{p}.aout',
                    f'{tmp}/{p}.s'], check=True, stdout=subprocess.DEVNULL)
    _, err, rc = run('strip', [p], tmp)
    check(f'strip {p}', [open(f'{tmp}/{p}.s', 'rb').read(), 0], [open(f'{tmp}/{p}', 'rb').read(), rc])
got, err, rc = run('nm', ['symt'], tmp)
check('nm on a stripped program', ['nm: symt: no symbols\n', 1], [err, rc])
got, err, rc = run('strip', ['symt'], tmp)
check('strip a stripped program', [b'', 0], [open(f'{tmp}/symt', 'rb').read() != open(f'{tmp}/symt.s', 'rb').read() and b'changed' or b'', rc])
got = subprocess.run(SIM + [f'{tmp}/symt'], capture_output=True, text=True).stdout
check('stripped symt runs', '86\n', got)
got, err, rc = run('strip', ['o/malloc.o'], tmp)
check('strip refuses an object file', ['strip: o/malloc.o: not an executable\n', 1], [err, rc])
shutil.rmtree(tmp)
print(f'{count} checks, {fails} failures: ' + ('ALL PASS' if not fails else 'FAILED'))
sys.exit(1 if fails else 0)
