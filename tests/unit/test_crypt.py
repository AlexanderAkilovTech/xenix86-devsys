"""Check crypt (lib/gen/crypt.c, run as tests/cryptt.c under xsim) against the host's
libcrypt on random passwords and salts, and on XENIX's own answer from M69:
crypt("test1234", "j9") is "j9jR7NRJjWyAI"."""
import ctypes, ctypes.util, os, random, subprocess
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
lib = ctypes.CDLL(ctypes.util.find_library('crypt') or 'libcrypt.so.1')
lib.crypt.restype = ctypes.c_char_p
AL = './0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'
random.seed(69)
cases = [('j9', 'test1234'), ('ab', ''), ('..', 'x'), ('zz', 'abcdefghij')]
for _ in range(300):
    pw = ''.join(chr(random.randrange(33, 127)) for _ in range(random.randrange(0, 11)))
    cases.append((random.choice(AL) + random.choice(AL), pw))
inp = ''.join(s + p + '\n' for s, p in cases)
out = subprocess.run(['python3', f'{ROOT}/tools/xsim.py', f'{ROOT}/build/cryptt'], input=inp.encode(),
                     capture_output=True, env=dict(os.environ, XSIM_LIMIT='2000000000')).stdout.decode().splitlines()
bad = 0
for (s, p), got in zip(cases, out):
    want = lib.crypt(p.encode(), s.encode()).decode()
    if got != want:
        bad += 1
        if bad < 5: print('MISMATCH', repr(p), s, got, want)
if out[:1] != ['j9jR7NRJjWyAI']:
    bad += 1; print('XENIX', out[:1])
if len(out) != len(cases):
    bad += 1; print('lines', len(out), len(cases))
print(f'{len(cases)} passwords, {bad} mismatches' + (': ALL PASS' if not bad else ''))
