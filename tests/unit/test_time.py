"""Check gmtime/localtime/ctime (lib/gen/ctime.c, run as tests/timet.c under xsim) against
Python's calendar and the America/Los_Angeles zone with TZ=PST8PDT, 1977-2006, including
every daylight-saving changeover in those years."""
import random, subprocess, time, datetime, os, sys
from zoneinfo import ZoneInfo
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
random.seed(7)
ts = [0, 1, 86399, 86400, 951782400, 951868800, 2147483647, 907933262, 496800000]
ts += [random.randrange(0, 2**31) for _ in range(300)]
la = ZoneInfo('America/Los_Angeles')
for y in range(1977, 2007):
    for m in (4, 10):
        d0 = int(datetime.datetime(y, m, 1, tzinfo=datetime.timezone.utc).timestamp())
        for h in range(0, 32 * 24):
            t = d0 + h * 3600
            if datetime.datetime.fromtimestamp(t, la).dst() != datetime.datetime.fromtimestamp(t + 3600, la).dst():
                ts += [t - 1, t, t + 1, t + 3599, t + 3600, t + 3601]
inp = ''.join(f'{t}\n' for t in ts)
out = subprocess.run(['python3', f'{ROOT}/tools/xsim.py', f'{ROOT}/build/timet'], input=inp.encode(),
                     capture_output=True, env=dict(os.environ, XSIM_TZ='PST8PDT')).stdout.decode()
bad = 0
for line in out.splitlines():
    left, ct = line.split(' | ')
    t, *f = map(int, left.split())
    g = time.gmtime(t)
    want = [g.tm_year - 1900, g.tm_mon - 1, g.tm_mday, g.tm_hour, g.tm_min, g.tm_sec, (g.tm_wday + 1) % 7, g.tm_yday - 1]
    l = datetime.datetime.fromtimestamp(t, la)
    wct = l.strftime('%a %b ') + f'{l.day:2d}' + l.strftime(' %H:%M:%S %Y')
    if f != want or (1977 <= l.year <= 2006 and ct != wct):
        bad += 1
        if bad < 6: print('MISMATCH', t, f, want, repr(ct), repr(wct))
n = len(out.splitlines())
print(f'{n} times, {bad} mismatches' + (': ALL PASS' if bad == 0 and n == len(ts) else ''))
sys.exit(0 if bad == 0 and n == len(ts) else 1)
