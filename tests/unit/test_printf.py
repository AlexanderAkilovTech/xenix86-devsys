"""printf and sprintf against the host C library's, for the integer and string
conversions: every flag, widths, precisions (0 for a value of 0 included),
16- and 32-bit values. The same program is compiled for XENIX, run on the
simulator twice (once with standard output a file, once passing for a
terminal, so line buffered), and compiled for the host; the three outputs
must be the same. (The floating point conversions have their own test.)

    python3 tests/unit/test_printf.py"""
import os, random, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
random.seed(1986)
FLAGS = ['', '-', '0', '+', ' ', '#', '-+', '0+', '- ', '#0', '-#', '+ ', '0 #']
ints, longs, strs = [], [], []
for i in range(900):
    conv = random.choice('diuxXo')
    fmt = '%' + random.choice(FLAGS) + random.choice(['', '1', '5', '12', '20']) + \
          random.choice(['', '', '.0', '.1', '.3', '.8'])
    if random.random() < 0.5:
        v = random.choice([0, 1, -1, 65535, 65536, -65536, 2147483647, -2147483648,
                           123456789, 4095, random.randint(-2**31, 2**31 - 1)])
        if conv in 'uxXo':
            v &= 0xffffffff
            longs.append((fmt + 'l' + conv, '(long)%uL' % v))
        else:
            longs.append((fmt + 'l' + conv, '%dL' % v if v != -2**31 else '(-2147483647L-1)'))
    else:
        v = random.choice([0, 1, -1, 32767, -32768, 255, 1000, 8, random.randint(-32768, 32767)])
        if conv in 'uxXo':
            ints.append((fmt + conv, '%uU' % (v & 0xffff)))
        else:
            ints.append((fmt + conv, '%d' % v if v != -32768 else '(-32767-1)'))
for i in range(200):
    fmt = '%' + random.choice(['', '-']) + random.choice(['', '3', '10', '25', '*']) + \
          random.choice(['', '.2', '.0', '.15', '.*'])
    s = random.choice(['', 'a', 'hello', 'long string here!', 'two\\nlines', '%d not a conversion'])
    strs.append((fmt + 's', s))

src = ['#include <stdio.h>']
def table(name, typ, cases):
    src.append('char *%sf[] = {%s};' % (name, ','.join('"[%s]|text %%%% c=%%c\\n"' % f for f, _ in cases)))
    src.append('char *%sg[] = {%s};' % (name, ','.join('"<%s>"' % f for f, _ in cases)))
    src.append('%s %sv[] = {%s};' % (typ, name, ','.join(a for _, a in cases)))
table('i', 'unsigned', ints)
table('l', 'long', longs)
src.append('char *sf[] = {%s};' % ','.join('"[%s]|%%5c|\\n"' % f for f, _ in strs))
src.append('char *sv[] = {%s};' % ','.join('"%s"' % s for _, s in strs))
src.append('int sw[] = {%s};' % ','.join(str(random.choice([-7, 0, 4, 9])) for _ in strs))
src.append('int sp[] = {%s};' % ','.join(str(random.choice([-1, 0, 3, 30])) for _ in strs))
src.append('int sn[] = {%s};' % ','.join(str(f.count('*')) for f, _ in strs))
src.append('''main()
{
	char b[300];
	int k;

	for (k = 0; k < %d; k++) {
		printf(if[k], iv[k], 'A' + k %% 26);
		sprintf(b, ig[k], iv[k]);
		fputs(b, stdout);
		putchar('\\n');
	}
	for (k = 0; k < %d; k++) {
		printf(lf[k], lv[k], 'a' + k %% 26);
		sprintf(b, lg[k], lv[k]);
		fputs(b, stdout);
		putchar('\\n');
	}
	for (k = 0; k < %d; k++) {
		if (sn[k] == 0)			/* the stars: width, precision */
			printf(sf[k], sv[k], 'x');
		else if (sn[k] == 1)
			printf(sf[k], sw[k], sv[k], 'y');
		else
			printf(sf[k], sw[k], sp[k], sv[k], 'z');
	}
	printf("%%s|%%c|%%%%|%%5%%|done\\n", "end", 'Z');
	exit(0);
}''' % (len(ints), len(longs), len(strs)))
text = '\n'.join(src).replace('char *if[]', 'char *i_f[]').replace('printf(if[k]', 'printf(i_f[k]') + '\n'

os.makedirs(f'{ROOT}/build', exist_ok=True)
open(f'{ROOT}/build/printft.c', 'w').write(text)
r = subprocess.run(['./build.sh', 'build/printft.c'], cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
if r.returncode:
    print(r.stdout.decode()); sys.exit(1)
open(f'{ROOT}/build/printft_host.c', 'w').write('#include <stdlib.h>\n' + text)
subprocess.run(['gcc', '-w', '-std=gnu89', '-o', f'{ROOT}/build/printft_host', f'{ROOT}/build/printft_host.c'], check=True)
host = subprocess.run([f'{ROOT}/build/printft_host'], stdout=subprocess.PIPE).stdout
count = fails = 0
for what, env in (('to a file', {}), ('to a terminal, line buffered', {'XSIM_TTY': '1'})):
    out = subprocess.run([sys.executable, f'{ROOT}/tools/xsim.py', f'{ROOT}/build/printft'],
                         env=dict(os.environ, **env), stdout=subprocess.PIPE).stdout
    a, b = out.split(b'\n'), host.split(b'\n')
    count += len(b)
    bad = [i for i in range(max(len(a), len(b))) if i >= len(a) or i >= len(b) or a[i] != b[i]]
    fails += len(bad)
    for i in bad[:10]:
        print(f'FAIL {what}, line {i + 1}: ours {a[i] if i < len(a) else None!r}, host {b[i] if i < len(b) else None!r}')
print(f'{count} lines, {fails} differences: ' + ('ALL PASS' if not fails else 'FAILED'))
sys.exit(1 if fails else 0)
