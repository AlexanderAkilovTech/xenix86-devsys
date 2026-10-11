"""Tests for the termcap library (lib/termcap/termcap.c): tests/tctest.c drives
tgetent, tgetnum, tgetflag, tgetstr, tgoto and tputs on the simulator, over the
made-up terminals in tests/termcap.test (and any termcap file named by
TC_EXTRA, every entry of it), and this file works out the answers itself.

    python3 tests/unit/test_termcap.py"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

def entries(path):
    """the termcap file's logical lines, as the library joins them"""
    out, cur, sol = [], '', True
    text = open(path, encoding='latin1').read().replace('\\\n', '')
    for line in text.split('\n'):
        if line.startswith('#') or line == '':
            continue
        out.append(line)
    return out

def find(lines, name):
    for e in lines:
        if name in e.split(':')[0].split('|'):
            return e
    return None

def tgetent(lines, name):
    e = find(lines, name)
    if e is None:
        return None
    for _ in range(16):
        i = field_pos(e, 'tc')
        if i is None or e[i:i + 1] != '=':
            break
        j = e.find(':', i)
        tcname = e[i + 1:j if j >= 0 else len(e)][:31]
        rest = e[j + 1:] if j >= 0 else ''
        other = find(lines, tcname)
        if other is None:
            e = e[:i - 2]
            break
        oc = other[other.find(':'):] if ':' in other else ''
        e = e[:i - 2] + rest
        if e.endswith(':') and oc.startswith(':'):
            oc = oc[1:]
        e = e + oc
    return e[:1023]

def field_pos(e, id):
    """where what follows capability id's name is, or None"""
    p = 0
    while True:
        p = e.find(':', p)
        if p < 0:
            return None
        p += 1
        if e[p:p + 2] == id and len(id) == 2:
            return p + 2

def getnum(e, id):
    i = field_pos(e, id)
    if i is None or e[i:i + 1] != '#':
        return -1
    m = re.match(r'\d*', e[i + 1:]).group()
    return int(m) if m else 0

def getflag(e, id):
    i = field_pos(e, id)
    return int(i is not None and e[i:i + 1] in (':', ''))

def getstr(e, id):
    i = field_pos(e, id)
    if i is None or e[i:i + 1] != '=':
        return None
    out, p = [], i + 1
    while p < len(e) and e[p] != ':':
        c = e[p]; p += 1
        if c == '^' and p < len(e):
            c = e[p]; p += 1
            out.append(0o177 if c == '?' else ord(c) & 0o37)
        elif c == '\\' and p < len(e):
            c = e[p]; p += 1
            esc = {'E': 0o33, 'e': 0o33, 'n': 10, 'r': 13, 't': 9, 'b': 8, 'f': 12}
            if c in esc:
                out.append(esc[c])
            elif '0' <= c <= '7':
                v, k = int(c), 1
                while k < 3 and p < len(e) and '0' <= e[p] <= '7':
                    v = 8 * v + int(e[p]); p += 1; k += 1
                out.append(v & 0o377 or 0o200)
            else:
                out.append(ord(c))
        else:
            out.append(ord(c))
    return bytes(out)

def tgoto(cm, col, line, up, bc):
    if cm is None:
        return b'OOPS'
    out, added, oncol, which, p = bytearray(), bytearray(), 0, line, 0
    while p < len(cm):
        c = cm[p]; p += 1
        if c != ord('%'):
            out.append(c); continue
        c = chr(cm[p]) if p < len(cm) else '\0'; p += 1
        if c in 'd23':
            out += (b'%d' % which if c == 'd' else b'%02d' % (which % 100) if c == '2' else b'%03d' % (which % 1000))
        elif c in '+.':
            if c == '+':
                which += cm[p]; p += 1
            which &= 0o377
            if which in (0, 4, 10) and (oncol or up):
                while True:
                    added += (bc if bc else b'\b') if oncol else up
                    which += 1
                    if which != 10: break
            out.append(which & 0o377)
        elif c == '>':
            x = cm[p]; p += 1
            if which > x: which += cm[p] if p < len(cm) else 0
            if p < len(cm): p += 1
            continue
        elif c == 'r':
            oncol, which = 1, col; continue
        elif c == 'i':
            line += 1; col += 1; which += 1; continue
        elif c == 'n':
            line ^= 0o140; col ^= 0o140; which ^= 0o140; continue
        elif c == 'B':
            which = which // 10 * 16 + which % 10; continue
        elif c == 'D':
            which -= 2 * (which % 16); continue
        elif c == '%':
            out.append(ord('%')); continue
        else:
            return b'OOPS'
        oncol = 1 - oncol
        which = col if oncol else line
    return bytes(out + added)

TMSPC10 = [0, 2000, 1333, 909, 743, 666, 500, 333, 166, 83, 55, 41, 20, 10, 5]

def tputs(s, affcnt, ospeed):
    if s is None:
        return b''
    m = re.match(rb'(\d*)(\.(\d?)\d*)?(\*?)', s)
    delay = int(m.group(1) or 0) * 10 + (int(m.group(3)) if m.group(3) else 0)
    if m.group(4):
        delay *= affcnt
    out = s[m.end():]
    if delay and 0 < ospeed < len(TMSPC10):
        out += b'@' * ((delay + TMSPC10[ospeed] // 2) // TMSPC10[ospeed])
    return out

def show(b):
    if b is None:
        return '(null)'
    return ''.join(chr(c) if 32 <= c < 0o177 else '\\%03o' % c for c in b.split(b'\0')[0])

def cases(path, names):
    lines = entries(path)
    cmds, want = [], []
    for name in names:
        e = tgetent(lines, name)
        cmds.append(f'e {name}'); want.append('1' if e is not None else '0')
        if e is None:
            continue
        ids = sorted(set(re.findall(r':([^:\s]{2})', ':' + e)) | {'co', 'li', 'am', 'xn', 'zz', 'tc'})
        for id in ids:
            if not re.fullmatch(r'[\x21-\x7e]{2}', id) or id[0] in '%':
                continue
            cmds += [f'n {id}', f'f {id}', f's {id}']
            want += [str(getnum(e, id)), str(getflag(e, id)), show(getstr(e, id))]
        up, bc = getstr(e, 'up'), getstr(e, 'bc')
        cm = getstr(e, 'cm')
        if cm is not None:
            for col, line in ((0, 0), (5, 10), (9, 3), (79, 23), (3, 0), (0, 4), (10, 9), (99, 100), (255, 3), (31, 31)):
                cmds.append(f'g cm {col} {line}'); want.append(show(tgoto(cm, col, line, up, bc)))
        for id in ids:
            s = getstr(e, id)
            if s is not None and re.match(rb'\d', s):
                for aff, sp in ((1, 0), (1, 13), (3, 9), (24, 1), (2, 15)):
                    cmds.append(f'p {id} {aff} {sp}'); want.append(show(tputs(s, aff, sp)))
    return cmds, want

prog = f'{ROOT}/build/tctest'
subprocess.run([f'{ROOT}/build.sh', 'tests/tctest.c'], cwd=ROOT, check=True, stdout=subprocess.DEVNULL)
fails = count = 0
runs = [(f'{ROOT}/tests/termcap.test', ['dumb', 'un', 'vt100', 'vt', 'v2', 'v3', 'adm', 'd1', 'd2', 'd3', 'bc',
                                         'dd', 'nn', 'gt', 'ri', 'es', 'mt', 'lp', 'nosuch'])]
if os.environ.get('TC_EXTRA'):
    t = os.environ['TC_EXTRA']
    runs.append((t, sorted({n for e in entries(t) for n in e.split(':')[0].split('|') if n and not re.search(r'\s', n)})))
for path, names in runs:
    cmds, want = cases(path, names)
    env = dict(os.environ, XSIM_ENV=f'TERMCAP={path}', XSIM_LIMIT='4000000000')
    got = subprocess.run([sys.executable, f'{ROOT}/tools/xsim.py', prog], input='\n'.join(cmds) + '\n',
                         capture_output=True, text=True, env=env, encoding='latin1').stdout.split('\n')
    for i, (c, w) in enumerate(zip(cmds, want)):
        count += 1
        g = got[i] if i < len(got) else '(no output)'
        if g != w:
            fails += 1
            if fails <= 12:
                print(f'FAIL {os.path.basename(path)}: {c}\n  want {w}\n  got  {g}')
# an entry in the environment's TERMCAP
env = dict(os.environ, XSIM_ENV='TERMCAP=xx|myterm:cl=\\E[H:li#5:')
got = subprocess.run([sys.executable, f'{ROOT}/tools/xsim.py', prog], input='e myterm\ns cl\nn li\n',
                     capture_output=True, text=True, env=env).stdout.split('\n')[:3]
count += 1
if got != ['1', '\\033[H', '5']:
    fails += 1; print(f'FAIL TERMCAP entry: got {got}')
print(f'{count} checks, {fails} failures: ' + ('ALL PASS' if not fails else 'FAILED'))
sys.exit(1 if fails else 0)
