"""Check the XENIX-built BCC tools against the host's BCC, one stage at a time.

Every native tool runs on an emulated 8086 under tools/xsim.py. The reference
compiler pass is a host build of the same patched sources (ports/bcc/host.sh);
the host's preprocessor, assembler and linker come from the bcc/bin86 packages
(also Dev86 0.16.17). Inputs: this project's C
library and test programs, and BCC's own sources.
  cpp   bcc-cpp output must be identical
  cc1   bcc-cc1 output must be identical apart from blank lines (inside #asm
        blocks the separately built preprocessor leaves a few extra ones; the
        assembler ignores them). The XENIX bcc-cc1 has no atof: it reads the
        native bcc-cpp's output with floating constants in hexadecimal (-H),
        as cc runs it, while the host's reads the decimal ones
  as    as86 object files must be identical
  ld    ld86 executables must be identical
  cc    the native cc (all passes on the emulated 8086, with CCROOT pointing at
        build/xenix/root) must build every test program byte-identical to
        the cross-compiled build
  ar    the native ar86 must create, list, extract, delete and replace exactly
        as the host's ar86 does, on an archive larger than 64K
  self  the native toolchain must rebuild bcc-cc1, cc and aout2x byte for byte
Usage: ./dist.sh && python3 ports/bcc/test.py"""
import glob, os, subprocess, sys, tempfile
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = f'{ROOT}/build/ports/dev86-0.16.17'
BIN = f'{ROOT}/build/xenix/bin'
SIM = ['python3', f'{ROOT}/tools/xsim.py']
DEFS = ['-D__ELKS__', '-D__unix__', f'-I{ROOT}/include', '-D__BCC_VERSION__=0x001011L', '-0']

def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, **kw)

def same(a, b, blank_ok=False):
    x, y = open(a, 'rb').read(), open(b, 'rb').read()
    if blank_ok:
        x = b'\n'.join(l for l in x.split(b'\n') if l.strip())
        y = b'\n'.join(l for l in y.split(b'\n') if l.strip())
    return x == y

corpus = []   # (name, source, extra cpp flags)
for f in sorted(glob.glob(f'{ROOT}/lib/*/*.c') + glob.glob(f'{ROOT}/tests/*.c')):
    corpus.append((os.path.relpath(f, ROOT), f, []))
XFLAGS = ['-Dconst=', '-DM_XENIX']      # as ports/bcc/build.sh compiles them
for d, extra, files in (
        ('bcc', ['-DNO_ATOF'], 'bcc-cc1 codefrag declare express exptree floatop function gencode genloads '
                    'glogcode hardop input label loadexp longop output preproc preserve scan softop state '
                    'table type assign hashcmd'),
        ('cpp', [], 'main cpp hash token1 token2'),
        ('as', ['-DBIGBUFFER=0', '-DUSE_FIXED_HEAP'], 'as assemble errors express genbin gensym macro mops pops readsrc scan table alloc'),
        ('ld', ['-DREL_OUTPUT'], 'io ld readobj table linksyms writebin writex86'),
        ('ar', [], 'ar')):
    for f in files.split():
        corpus.append((f'dev86/{d}/{f}.c', f'{SRC}/{d}/{f}.c', XFLAGS + extra))

tmp = tempfile.mkdtemp(prefix='bcctest')
fails = {'cpp': [], 'cc1': [], 'as': [], 'ld': [], 'cc': [], 'ar': [], 'self': []}
count = {'cpp': 0, 'cc1': 0, 'as': 0, 'ld': 0, 'cc': 0, 'ar': 0, 'self': 0}
objs = {}
for i, (name, src, extra) in enumerate(corpus):
    b = f'{tmp}/{i}'
    cwd = os.path.dirname(src)          # quoted includes resolve next to the source
    run(['/usr/lib/bcc/bcc-cpp', src, '-o', f'{b}.h.i'] + extra + DEFS, cwd=cwd)
    run(SIM + [f'{BIN}/bcc-cpp', src, '-o', f'{b}.x.i'] + extra + DEFS, cwd=cwd)
    run(SIM + [f'{BIN}/bcc-cpp', src, '-o', f'{b}.xh.i', '-H'] + extra + DEFS, cwd=cwd)
    count['cpp'] += 1
    if not same(f'{b}.h.i', f'{b}.x.i'):
        fails['cpp'].append(name)
    run([f'{ROOT}/build/ports/host/bcc-cc1', f'{b}.h.i', '-o', f'{b}.h.s', '-0'])
    run(SIM + [f'{BIN}/bcc-cc1', f'{b}.xh.i', '-o', f'{b}.x.s', '-0'])
    count['cc1'] += 1
    if not os.path.exists(f'{b}.x.s') or not same(f'{b}.h.s', f'{b}.x.s', blank_ok=True):
        fails['cc1'].append(name)
    mod = os.path.basename(src)[:-2] + '.s'
    run(['as86', f'{b}.h.s', '-o', f'{b}.h.o', '-u', '-w', '-0', '-n', mod])
    run(SIM + [f'{BIN}/as86', f'{b}.h.s', '-o', f'{b}.x.o', '-u', '-w', '-0', '-n', mod])
    count['as'] += 1
    if not os.path.exists(f'{b}.x.o') or not same(f'{b}.h.o', f'{b}.x.o'):
        fails['as'].append(name)
    objs[name] = f'{b}.h.o'
    print(f'\r{i + 1}/{len(corpus)} files', end='', file=sys.stderr, flush=True)
print(file=sys.stderr)

# link every test program (crt0.o + program + libm.a + libc.a) with both linkers,
# but those with an assembly half
for name in [n for n in objs if n.startswith('tests/') and n not in ('tests/regdump.c', 'tests/fptest.c',
                                                                    'tests/fpu87.c', 'tests/cgatest.c')]:
    b = f'{tmp}/link_{os.path.basename(name)[:-2]}'
    args = ['-0', '-i', '-o', None, f'{ROOT}/build/crt0.o', objs[name], f'{ROOT}/build/libcurses.a',
            f'{ROOT}/build/libtermcap.a',
            f'{ROOT}/build/libm.a', f'{ROOT}/build/libc.a']
    h = run(['ld86'] + [a if a else f'{b}.h' for a in args])
    x = run(SIM + [f'{BIN}/ld86'] + [a if a else f'{b}.x' for a in args])
    count['ld'] += 1
    if h.returncode or not os.path.exists(f'{b}.x') or not same(f'{b}.h', f'{b}.x'):
        fails['ld'].append(name)

# the native cc against the cross build
R = f'{ROOT}/build/xenix/testroot'   # the install tree, plus the sys/ stand-ins
cenv = dict(os.environ, XSIM_ENV=f'CCROOT={R}')
count['cc'], fails['cc'] = 0, []
HALVES = {'fptest': 'fptests.s', 'fpu87': 'fpu87s.s', 'cgatest': 'cgaio.s'}   # assembly halves
for src in sorted(glob.glob(f'{ROOT}/tests/*.c')):
    t = os.path.basename(src)[:-2]
    if t == 'regdump':                  # its two halves would both be regdump.o
        continue
    more = [f'{ROOT}/tests/{HALVES[t]}'] if t in HALVES else []
    run([f'{ROOT}/build.sh', src] + more)
    # build.sh links every program with libcurses.a, libtermcap.a and libm.a: so do
    # -lcurses -ltermcap -lm
    run(SIM + [f'{R}/usr/bin/cc', '-o', f'{tmp}/{t}.native', src] + more + ['-lcurses', '-ltermcap', '-lm'],
        env=cenv, cwd=tmp)
    count['cc'] += 1
    if not os.path.exists(f'{tmp}/{t}.native') or not same(f'{tmp}/{t}.native', f'{ROOT}/build/{t}'):
        fails['cc'].append(f'tests/{t}.c')
    print(f'\rcc: {count["cc"]} programs', end='', file=sys.stderr, flush=True)
print(file=sys.stderr)

# ar: the same operations with both archivers
count['ar'], fails['ar'] = 0, []
ad = f'{tmp}/ar'
os.makedirs(f'{ad}/x')
members = sorted(glob.glob(f'{ROOT}/build/ports/obj/bcc/*.o'))
NAR = SIM + [f'{BIN}/ar86']
for what, host, native in (
        ('create', ['ar86', 'rc', 'h.a'] + members, NAR + ['rc', 'n.a'] + members),
        ('list', ['ar86', 't', 'h.a'], NAR + ['t', 'n.a']),
        ('delete', ['ar86', 'd', 'h.a', 'scan.o', 'table.o'], NAR + ['d', 'n.a', 'scan.o', 'table.o']),
        ('replace', ['ar86', 'r', 'h.a', members[0]], NAR + ['r', 'n.a', members[0]])):
    h = run(host, cwd=ad); x = run(native, cwd=ad)
    count['ar'] += 1
    if h.stdout != x.stdout or open(f'{ad}/h.a', 'rb').read() != open(f'{ad}/n.a', 'rb').read():
        fails['ar'].append(what)
run(NAR + ['x', '../n.a'], cwd=f'{ad}/x')
count['ar'] += 1
for m in members:
    b = os.path.basename(m)
    if b in ('scan.o', 'table.o'):
        continue
    if not os.path.exists(f'{ad}/x/{b}') or not same(m, f'{ad}/x/{b}'):
        fails['ar'].append('extract'); break

# self-hosting: the native toolchain rebuilds its own parts
count['self'], fails['self'] = 0, []
sd = f'{tmp}/selfhost'
run(['cp', '-r', f'{SRC}/bcc', sd])
cc1src = ('bcc-cc1 codefrag dbnode declare express exptree floatop function gencode genloads glogcode hardop '
          'input label loadexp longop output preproc preserve scan softop state table type assign hashcmd').split()
for name, args, ref in (
        # as ports/bcc/build.sh builds it: callersaves, copt (with rules.mc), as86 -O, no atof
        ('bcc-cc1', ['-O', '-Mc', '-s', '-DNO_ATOF', '-Dconst=', '-F', '2000'] + [f + '.c' for f in cc1src],
         f'{BIN}/bcc-cc1'),
        ('cc', [f'{ROOT}/cmd/cc.c'], f'{ROOT}/build/cc'),
        ('aout2x', [f'{ROOT}/cmd/aout2x.c'], f'{ROOT}/build/aout2x')):
    run(SIM + [f'{R}/usr/bin/cc', '-o', f'{sd}/{name}.native'] + args, env=cenv, cwd=sd)
    count['self'] += 1
    if not os.path.exists(f'{sd}/{name}.native') or not same(f'{sd}/{name}.native', ref):
        fails['self'].append(name)

# Known differences where the host tool is the one in error, each checked to be
# exactly the expected difference.
KNOWN = {('cc1', 'dev86/as/genbin.c'): ('mov\tbx,#$FFFF', 'mov\tbx,#$7FFF',
         'the host build of bcc-cc1 is 64-bit and folds (unsigned long)-1 >> 1 to 0xFFFFFFFF; '
         'the right value, 0x7FFFFFFF, is what the 16-bit build gives')}
for (stage, name), (host_line, xen_line, why) in KNOWN.items():
    if name in fails[stage]:
        i = [n for n, *_ in corpus].index(name)
        h = [l for l in open(f'{tmp}/{i}.h.s').read().split('\n') if l.strip()]
        x = [l for l in open(f'{tmp}/{i}.x.s').read().split('\n') if l.strip()]
        d = [(a, b) for a, b in zip(h, x) if a != b]
        if len(h) == len(x) and d == [(host_line, xen_line)]:
            fails[stage].remove(name)
            print(f'note: {name}: {why}')

ok = True
for stage in ('cpp', 'cc1', 'as', 'ld', 'cc', 'ar', 'self'):
    n, f = count[stage], fails[stage]
    print(f'{stage:4} {n - len(f)}/{n} identical' + (f'; different: {", ".join(f)}' if f else ''))
    ok = ok and not f
sys.exit(0 if ok else 1)
