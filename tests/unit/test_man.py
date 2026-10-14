"""Tests for the manual: tools/man.py, which formats the -man pages as plain text,
and cmd/man.c, which finds and shows them (run on the simulator).

    python3 tests/unit/test_man.py"""
import glob, os, re, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, f'{ROOT}/tools')
import man as fmt

SIM = [sys.executable, f'{ROOT}/tools/xsim.py']
count = fails = 0

def check(what, want, got):
    global count, fails
    count += 1
    if want != got:
        fails += 1
        print(f'FAIL {what}\n  want {want!r}\n  got  {got!r}')

def render(src):
    return fmt.Page().render(src).split('\n')[2:-1]     # without the heading line

# the formatter, on small pieces of -man
check('fill and wrap at 78', ['NAME', '     ' + 'word ' * 13 + 'word', '     word word word'],
      render('.TH X 1\n.SH NAME\n' + 'word ' * 17))
check('no blank line after a heading, one before the next',
      ['A', '     -x     an option', '', 'B', '     text'],
      render('.TH X 1\n.SH A\n.TP\n.B \\-x\nan option\n.SH B\ntext'))
check('.TP tag too wide goes on its own line',
      ['A', '     -verylong', '            text'],
      render('.TH X 1\n.SH A\n.TP\n\\-verylong\ntext'))
check('.TP with an indent', ['A', '     -x        text'], render('.TH X 1\n.SH A\n.TP 10\n\\-x\ntext'))
check('.IP', ['A', '     o      item'], render('.TH X 1\n.SH A\n.IP \\(bu\nitem'))
check('no-fill keeps lines, tab stops from the indent',
      ['A', '     a       b', '     abcdefghij      c', '     back'],
      render('.TH X 1\n.SH A\n.nf\na\tb\nabcdefghij\tc\n.fi\nback'))
check('.RS and .RE', ['A', '     one', '            two', '     three'],
      render('.TH X 1\n.SH A\none\n.RS\ntwo\n.RE\nthree'))
check('fonts and escapes', ['A', "     bold italic -- - \\ x y 'z'"],
      render(".TH X 1\n.SH A\n.B bold\n.I italic\n\\(em \\- \\e \\fBx\\fP\\ y \\(aqz\\(aq"))
check('.BR joins its words', ['A', '     cc(1), and'], render('.TH X 1\n.SH A\n.BR cc (1),\nand'))
check('comments ignored', ['A', '     text'], render('.TH X 1\n.\\" a comment\n.SH A\ntext'))
p = fmt.Page(); p.render('.TH FOO 3\n.SH NAME\nfoo, bar \\- do things\n.SH X\ny')
check('NAME line kept for whatis', 'foo, bar - do things', p.names)
check('heading', 'FOO(3)' + ' ' * 26 + 'xenix86-devsys' + ' ' * 26 + 'FOO(3)',
      fmt.Page().render('.TH foo 3\n').split('\n')[0])

# every page
tmp = tempfile.mkdtemp()
pages = sorted(glob.glob(f'{ROOT}/man/man*/*.[1-8]'))
subprocess.run([sys.executable, f'{ROOT}/tools/man.py', f'{tmp}/m'] + pages, check=True)
whatis = open(f'{tmp}/m/whatis').read().split('\n')[:-1]
check('a whatis line for each page', len(pages), len(whatis))
check('whatis lines fit man.c\'s buffer', [], [l for l in whatis if len(l) > 290])
check('whatis sorted', sorted(whatis, key=lambda l: (l.split('\t')[0].split('(')[1], l.split('(')[0])), whatis)
for path in pages:
    name, sec = os.path.basename(path).rsplit('.', 1)
    out = open(f'{tmp}/m/cat{sec}/{name}.{sec}').read()
    lines = out.split('\n')
    check(f'{name}.{sec}: lines fit in 78 columns', [], [l for l in lines if len(l) > 78])
    check(f'{name}.{sec}: no tabs or escapes left', [], [l for l in lines if '\t' in l or '\\f' in l or '\\-' in l or '\\(' in l])
    check(f'{name}.{sec}: heading', f'{name.upper()}({sec})', lines[0][:len(name) + 3])
    check(f'{name}.{sec}: NAME section', ['', 'NAME'], lines[1:3])
    src = open(path).read()
    check(f'{name}.{sec}: a HISTORY section', True, '.SH HISTORY' in src)
    # The manual is written as of 1986: no later year anywhere, but in the
    # pages about the clock itself, date(1) and ctime(3), which must name the
    # years it reaches and the rules for them; their HISTORY stays at 1986.
    year = lambda t: [y for y in re.findall(r'\b1[89]\d\d\b|\b20\d\d\b', t) if int(y) > 1986]
    late = year(src.split('.SH HISTORY')[1].split('\n.SH')[0]) if name in ('date', 'ctime') else year(src)
    check(f'{name}.{sec}: no year after 1986' + (' in its HISTORY' if name in ('date', 'ctime') else ''), [], late)
    entry = [l for l in whatis if l.startswith(f'{name}({sec})\t')]
    check(f'{name}.{sec}: in whatis with a description', True, len(entry) == 1 and ' - ' in entry[0])

# the man command
def man(*a, mandir=f'{tmp}/m'):
    env = dict(os.environ, XSIM_ENV=f'MANPATH={mandir}')
    r = subprocess.run(SIM + [f'{ROOT}/build/man'] + list(a), capture_output=True, env=env)
    return r.stdout.decode('latin1'), r.stderr.decode('latin1'), r.returncode

page = lambda p: open(f'{tmp}/m/{p}').read()
check('man cc', [page('cat1/cc.1'), '', 0], list(man('cc')))
check('man 1 nm', [page('cat1/nm.1'), '', 0], list(man('1', 'nm')))
check('man two pages', [page('cat1/size.1') + page('cat1/strip.1'), 0], [man('size', 'strip')[0], man('size', 'strip')[2]])
if os.path.exists(f'{tmp}/m/cat3'):
    for name, pg in (('strcpy', 'string'), ('sqrt', 'exp'), ('tputs', 'termcap'), ('isdigit', 'ctype'), ('fflush', 'fclose')):
        check(f'man {name} finds {pg}(3)', page(f'cat3/{pg}.3'), man(name)[0])
    check('man intro: section 1 first', page('cat1/intro.1'), man('intro')[0])
    check('man 3 intro', page('cat3/intro.3'), man('3', 'intro')[0])
check('man, no such page', ['', 'man: no manual entry for nothing\n', 1], list(man('nothing')))
check('man, not in that section', ['', 'man: no entry for nm in section 3\n', 1], list(man('3', 'nm')))
check('man -k, any case', [[l + '\n' for l in whatis if 'librar' in l.lower()], 0],
      [man('-k', 'LIBRAR')[0].splitlines(True), 0])
check('man -k, nothing', ['', 'man: nothing about zzz\n', 1], list(man('-k', 'zzz')))
check('man, no names', 2, man()[2])
check('man -k, no keyword', 2, man('-k')[2])
check('man with MANPATH missing', 1, man('cc', mandir='/nonexistent')[2])

# a page for several names, the same name in two sections, a page not in the index
d = f'{tmp}/x'
for s in '13':
    os.makedirs(f'{d}/cat{s}')
open(f'{d}/whatis', 'w').write('printf(1)\tprintf - print\nstring(3)\tstrcat, strncat, strcmp - strings\n'
                               'printf(3)\tprintf, fprintf, sprintf - print formatted\n')
for f, t in (('cat1/printf.1', 'one\n'), ('cat3/string.3', 'strings\n'), ('cat3/printf.3', 'three\n'),
             ('cat3/loose.3', 'loose\n')):
    open(f'{d}/{f}', 'w').write(t)
check('a name in a list', ['strings\n', 0], [man('strncat', mandir=d)[0], man('strncat', mandir=d)[2]])
check('the last name in a list', 'strings\n', man('strcmp', mandir=d)[0])
check('the page name itself', 'strings\n', man('string', mandir=d)[0])
check('part of a name is not a match', 1, man('strn', mandir=d)[2])
check('the first section', 'one\n', man('printf', mandir=d)[0])
check('the section asked for', 'three\n', man('3', 'printf', mandir=d)[0])
check('a name later in a list, section 3', 'three\n', man('sprintf', mandir=d)[0])
check('a page not in whatis', 'loose\n', man('loose', mandir=d)[0])
os.remove(f'{d}/whatis')
check('no whatis at all', ['three\n', 0], [man('3', 'printf', mandir=d)[0], man('3', 'printf', mandir=d)[2]])
check('no whatis, man -k', 1, man('-k', 'x', mandir=d)[2])

shutil.rmtree(tmp)
print(f'{count} checks, {fails} failures: ' + ('ALL PASS' if not fails else 'FAILED'))
sys.exit(1 if fails else 0)
