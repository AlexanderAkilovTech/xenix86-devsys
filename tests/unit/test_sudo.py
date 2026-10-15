"""Check sudo and shutdown (cmd/sudo.c, cmd/shutdown.c) under xsim: who may use sudo
(/etc/default/sudo), root's or the account's own password (crypt, M69's hash of
test1234), three tries, no password asked when the field is empty, the log in
/usr/adm/sudolog, the command run; shutdown's arguments and root only."""
import os, subprocess, tempfile
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SIM = ['python3', f'{ROOT}/tools/xsim.py']
tmp = tempfile.mkdtemp()
H = 'j9jR7NRJjWyAI'                    # crypt("test1234", "j9"), from XENIX (M69)
fails = n = 0
def check(what, ok, info=''):
    global fails, n
    n += 1
    if not ok:
        fails += 1; print('FAIL', what, info)
def setup(rootpw, alexpw, users, pwmode='root'):
    open(f'{tmp}/passwd', 'w').write(f'root:{rootpw}:0:0:Superuser:/:\n'
                                     f'bin:NONE:3:3::/bin:\nalex:{alexpw}:201:50:Alex:/usr/alex:/bin/csh\n')
    open(f'{tmp}/sudo', 'w').write(f'# who may use sudo\nUSERS={users}\nPASSWORD={pwmode}\n')
    if os.path.exists(f'{tmp}/sudolog'): os.remove(f'{tmp}/sudolog')
def sudo(uid, args, typed=''):
    env = dict(os.environ, XSIM_UID=str(uid), XSIM_MAP=f'/etc/passwd={tmp}/passwd,/etc/default/sudo={tmp}/sudo,'
               f'/usr/adm/sudolog={tmp}/sudolog,/dev/tty={tmp}/notty/tty,/bin/echo={ROOT}/build/echo',
               XSIM_LIMIT='2000000000', XSIM_SETUID='1')
    r = subprocess.run(SIM + [f'{ROOT}/build/sudo'] + args, input=typed.encode(), capture_output=True, env=env, cwd=tmp)
    log = open(f'{tmp}/sudolog').read() if os.path.exists(f'{tmp}/sudolog') else ''
    return r.stdout.decode('latin1'), r.stderr.decode('latin1'), r.returncode, log

setup(H, 'ab' + 'x' * 11, 'bob, alex')
o, e, rc, log = sudo(201, ['/bin/echo', 'hello'], 'test1234\n')
check('alex, root\'s password: the command runs', o == 'hello\n' and rc == 0, (o, e, rc))
check('asked for root\'s', 'Password for root: ' in e, repr(e))
check('logged', ' alex ran: /bin/echo hello' in log, repr(log))
o, e, rc, log = sudo(201, ['/bin/echo', 'hi'], 'nope\nnope\nnope\n')
check('three wrong passwords', o == '' and rc == 1 and e.count('Sorry, try again.') == 2 and '3 wrong passwords' in e, (o, e, rc))
check('logged wrong', 'alex WRONG PASSWORD: /bin/echo hi' in log, repr(log))
o, e, rc, log = sudo(201, ['/bin/echo', 'x'], 'nope\ntest1234\n')
check('right on the second try', o == 'x\n' and e.count('Sorry, try again.') == 1, (o, e))
setup(H, H, 'bob')
o, e, rc, log = sudo(201, ['/bin/echo', 'x'], 'test1234\n')
check('not in USERS', o == '' and rc == 1 and 'alex may not use sudo' in e and 'NOT ALLOWED' in log, (o, e, log))
o, e, rc, log = sudo(0, ['/bin/echo', 'root'])
check('root needs no password', o == 'root\n' and 'Password' not in e, (o, e))
setup('', 'ab' + 'x' * 11, 'alex')
o, e, rc, log = sudo(201, ['/bin/echo', 'free'])
check('root without a password: none asked', o == 'free\n' and 'Password' not in e, (o, e))
setup('ab' + 'x' * 11, H, 'alex', 'self')
o, e, rc, log = sudo(201, ['/bin/echo', 'own'], 'test1234\n')
check('PASSWORD=self: alex\'s own', o == 'own\n' and 'Password for alex: ' in e, (o, e))
o, e, rc, log = sudo(201, [])
check('usage', rc == 2 and 'usage: sudo' in e, (e, rc))
o, e, rc, log = sudo(201, ['/no/such'], 'test1234\n')
check('no such command', rc == 1 and 'sudo: /no/such: not found' in e, (e, rc))
# shutdown: its arguments, and root only (it is not let go on: it would halt)
def shut(uid, args):
    r = subprocess.run(SIM + [f'{ROOT}/build/shutdown'] + args, capture_output=True,
                       env=dict(os.environ, XSIM_UID=str(uid)), cwd=tmp)
    return r.stderr.decode('latin1'), r.returncode
for a in ([], ['-h'], ['soon'], ['+'], ['+5x'], ['-r', 'now']):
    e, rc = shut(0, a)
    check(f'shutdown {a}: usage', rc == 2 and 'usage: shutdown' in e, (a, e, rc))
e, rc = shut(201, ['-h', 'now'])
check('shutdown, not root', rc == 1 and 'only root may halt' in e, (e, rc))
print(f'{n} checks, {fails} failures' + (': ALL PASS' if not fails else ''))
