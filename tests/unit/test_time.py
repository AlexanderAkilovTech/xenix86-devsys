"""Check gmtime/localtime/ctime (lib/gen/ctime.c, run as tests/timet.c under xsim) against
Python's calendar and the America/Los_Angeles zone with TZ=PST8PDT, from 1970 to the end of
unsigned 32-bit time in February 2106, including every daylight time changeover in those
years: the rules of 1967-73, 1974 and 1975, 1976-86, 1987-2006, and 2007 on. A time is
taken as unsigned, so 2**31 and above are 2038 to 2106, not 1901 to 1969."""
import random, subprocess, time, datetime, os, sys
from zoneinfo import ZoneInfo
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
random.seed(7)
ts = [0, 1, 86399, 86400, 951782400, 951868800, 2147483647, 2147483648, 907933262, 496800000,
      4294967295, 4294944000, 1791918000]
ts += [random.randrange(0, 2**32) for _ in range(400)]
la = ZoneInfo('America/Los_Angeles')
utc = datetime.timezone.utc
def local(t):
    return datetime.datetime.fromtimestamp(t, utc).astimezone(la)
for y in range(1970, 2106):
    for m in (1, 2, 3, 4, 10, 11):
        d0 = int(datetime.datetime(y, m, 1, tzinfo=utc).timestamp())
        for h in range(0, 31 * 24):
            t = d0 + h * 3600
            if local(t).dst() != local(t + 3600).dst():
                ts += [t - 1, t, t + 1, t + 3599, t + 3600, t + 3601]
ts = [t for t in ts if 0 <= t < 2**32]
inp = ''.join(f'{t}\n' for t in ts)
out = subprocess.run(['python3', f'{ROOT}/tools/xsim.py', f'{ROOT}/build/timet'], input=inp.encode(),
                     capture_output=True, env=dict(os.environ, XSIM_TZ='PST8PDT')).stdout.decode()
bad = 0
for line in out.splitlines():
    left, ct = line.split(' | ')
    t, *f = map(int, left.split())
    t &= 0xffffffff                         # printed signed: from 2038 on, negative
    g = datetime.datetime.fromtimestamp(t, utc).timetuple()
    want = [g.tm_year - 1900, g.tm_mon - 1, g.tm_mday, g.tm_hour, g.tm_min, g.tm_sec, (g.tm_wday + 1) % 7, g.tm_yday - 1]
    l = local(t)
    wct = l.strftime('%a %b ') + f'{l.day:2d}' + l.strftime(' %H:%M:%S %Y')
    if f != want or ct != wct:
        bad += 1
        if bad < 6: print('MISMATCH', t, f, want, repr(ct), repr(wct))
n = len(out.splitlines())
print(f'{n} times, {bad} mismatches' + (': ALL PASS' if bad == 0 and n == len(ts) else ''))
sys.exit(0 if bad == 0 and n == len(ts) else 1)
