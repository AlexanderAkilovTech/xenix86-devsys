"""Test cmd/make.c on the emulated 8086: makefiles, macros, suffix rules,
timestamps and options, with the native cc building real programs.
Needs ./dist.sh (for the native cc) and ./build.sh for cmd/make.c and tests/bin."""
import os, subprocess, sys, tempfile, time, shutil
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
R = f'{ROOT}/build/xenix/testroot'   # the install tree, plus the sys/ stand-ins
tmp = tempfile.mkdtemp(prefix='maketest')
bindir = f'{tmp}/bin'
os.makedirs(bindir)
for p in ('make', 'echo', 'true', 'false'):
    shutil.copy(f'{ROOT}/build/{p}', f'{bindir}/{p}')
ENV = dict(os.environ, XSIM_ENV=f'PATH={bindir}:{R}/usr/bin CCROOT={R} GREETING=hi')
fails = 0
passes = 0

def run(d, *args, env=ENV):
    r = subprocess.run(['python3', f'{ROOT}/tools/xsim.py', f'{bindir}/make'] + list(args),
                       cwd=d, capture_output=True, text=True, env=env)
    return r.returncode, r.stdout, r.stderr

def check(name, cond, out=''):
    global fails, passes
    if cond:
        passes += 1
    else:
        fails += 1
        print(f'FAIL {name}\n{out}')

def setup(name, files):
    d = f'{tmp}/{name}'
    os.makedirs(d)
    for f, text in files.items():
        open(f'{d}/{f}', 'w').write(text)
        os.utime(f'{d}/{f}', (1e9, 1e9))      # sources: long ago
    return d

def newer(d, f):
    t = time.time() + 1000
    os.utime(f'{d}/{f}', (t, t))

# 1. a two-file program: header dependencies, the .c.o rule, up-to-date checks
d = setup('prog', {
    'makefile': 'OBJS = main.o util.o\n\nprog: $(OBJS)\n\tcc -o prog $(OBJS)\n\nmain.o util.o: util.h\n',
    'main.c': '#include <stdio.h>\n#include "util.h"\nmain()\n{\n\tprintf("%d\\n", twice(21));\n\treturn 0;\n}\n',
    'util.c': '#include "util.h"\ntwice(n)\nint n;\n{\n\treturn n * TWO;\n}\n',
    'util.h': '#define TWO 2\n'})
rc, out, err = run(d)
check('build', rc == 0 and out.split('\n')[:3] == ['cc -O -c main.c', 'cc -O -c util.c', 'cc -o prog main.o util.o'], out + err)
r = subprocess.run(['python3', f'{ROOT}/tools/xsim.py', f'{d}/prog'], capture_output=True, text=True)
check('the program works', r.stdout == '42\n', r.stdout)
rc, out, err = run(d)
check('up to date', rc == 0 and out == "`prog' is up to date.\n", out + err)
newer(d, 'util.c')
os.utime(f'{d}/prog', (1e9 + 10, 1e9 + 10))   # timestamps have 1-second resolution: age the last link
rc, out, err = run(d)
check('one source changed', out.split('\n')[:2] == ['cc -O -c util.c', 'cc -o prog main.o util.o'], out + err)
newer(d, 'util.h')
os.utime(f'{d}/prog', (1e9 + 10, 1e9 + 10))   # timestamps have 1-second resolution: age the last link
rc, out, err = run(d)
check('header changed', out.split('\n')[:3] == ['cc -O -c main.c', 'cc -O -c util.c', 'cc -o prog main.o util.o'], out + err)

# 2. no makefile at all: the single-suffix rule .c:
d = setup('nomake', {'hello.c': 'main()\n{\n\twrite(1, "hello\\n", 6);\n\treturn 0;\n}\n'})
rc, out, err = run(d, 'hello')
check('.c: rule without a makefile', rc == 0 and out == 'cc -O  -o hello hello.c\n' and os.path.exists(f'{d}/hello'), out + err)

# 3. macros
d = setup('macros', {'makefile':
    'SRCS = a.c b.c\nOBJS = $(SRCS:.c=.o)\nX = $(Y)\nY = late\nV = makefile\n'
    'show:\n\techo objs $(OBJS) x $(X) v $(V) env $(GREETING) at $@ braces ${Y} one $Xy\n'
    'dirs: sub/f.o\nsub/f.o:\n\techo $(@D) $(@F) $*\n'})
rc, out, err = run(d)
check('macros', out == 'echo objs a.o b.o x late v makefile env hi at show braces late one latey\n'
      'objs a.o b.o x late v makefile env hi at show braces late one latey\n', out + err)
rc, out, err = run(d, 'V=cmdline')
check('command line wins', 'v cmdline' in out, out + err)
rc, out, err = run(d, '-n', 'dirs')
check('$(@D) $(@F) $* and -n', out == 'echo sub f.o sub/f\n', out + err)

# 4. options and command prefixes
d = setup('opts', {'makefile':
    'all: one two\none:\n\t@echo quiet\n\t-false\n\techo after\n'
    'two:\n\tfalse\n\techo never\nthree: two\n\techo three\n'})
rc, out, err = run(d, 'one')
check('@ and -', out == 'quiet\nfalse\n*** Error code 1 (ignored)\necho after\nafter\n', out + err)
rc, out, err = run(d, 'two')
check('error stops make', rc == 1 and 'never' not in out and '*** Error code 1' in out and 'Stop.' in err, out + err)
rc, out, err = run(d, '-i', 'two')
check('-i', rc == 0 and out.endswith('never\n'), out + err)
rc, out, err = run(d, '-k', 'two', 'one')
check('-k keeps going', rc == 1 and 'after' in out, out + err)
rc, out, err = run(d, '-s', 'one')
check('-s', out == 'quiet\n*** Error code 1 (ignored)\nafter\n', out + err)

# 5. -t, -q, ::, .DEFAULT, unknown targets, .SUFFIXES and include
d = setup('more', {
    'makefile': 'include extra.mk\nout: in\n\techo making $@\nlog::\n\techo first\nlog::\n\techo second\n'
                '.SUFFIXES: .x .y\n.x.y:\n\techo convert $< to $@ stem $*\n',
    'extra.mk': 'INC = included\nshowinc:\n\techo $(INC)\n',
    'in': '', 'data.x': ''})
rc, out, err = run(d, '-q', 'out')
check('-q says out of date', rc == 1 and out == '', out + err)
rc, out, err = run(d, '-t', 'out')
check('-t touches', out == 'touch(out)\n' and os.path.exists(f'{d}/out'), out + err)
rc, out, err = run(d, '-q', 'out')
check('-q says up to date', rc == 0, out + err)
rc, out, err = run(d, 'log')
check('double-colon rules', out == 'echo first\nfirst\necho second\nsecond\n', out + err)
rc, out, err = run(d, 'data.y')
check('suffix rule with $< and $*', out == 'echo convert data.x to data.y stem data\nconvert data.x to data.y stem data\n', out + err)
rc, out, err = run(d, 'showinc')
check('include', out.endswith('included\n'), out + err)
rc, out, err = run(d, 'nosuch')
check("don't know how", rc == 1 and "Don't know how to make nosuch" in err, out + err)

print(f'{passes} make checks passed, {fails} failed')
shutil.rmtree(tmp)
sys.exit(1 if fails else 0)
