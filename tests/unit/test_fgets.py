"""fgets against a model of it in Python: files of short and long lines,
with and without a newline at the end, read with buffer sizes from 2 to
over the stdio buffer's 512, so that lines cross the buffer's refills.

    python3 tests/unit/test_fgets.py"""
import os, random, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
random.seed(42)
subprocess.run(['./build.sh', 'tests/fgetst.c'], cwd=ROOT, check=True, stdout=subprocess.DEVNULL)
count = fails = 0
tmp = tempfile.mkdtemp()
for case in range(40):
    lines = []
    for _ in range(random.randint(0, 60)):
        n = random.choice([0, 1, 5, 40, 79, 300, 511, 512, 513, 700])
        lines.append(''.join(random.choice('abcxyz \t:#|') for _ in range(n)))
    data = '\n'.join(lines) + random.choice(['', '\n'])
    open(f'{tmp}/f', 'w').write(data)
    sizes = [random.choice([2, 3, 10, 80, 256, 513, 600]) for _ in range(random.randint(1, 4))]
    out = subprocess.run([sys.executable, f'{ROOT}/tools/xsim.py', f'{ROOT}/build/fgetst', f'{tmp}/f'] +
                         [str(s) for s in sizes], stdout=subprocess.PIPE).stdout.decode('latin1')
    # the model: at most size - 1 characters, through the newline
    exp, pos, i = [], 0, 0
    while pos < len(data):
        n = sizes[i % len(sizes)] - 1
        i += 1
        nl = data.find('\n', pos, pos + n)
        end = nl + 1 if nl >= 0 else min(pos + n, len(data))
        exp.append(f'{end - pos}:{data[pos:end]}|Z\n')
        pos = end
    exp.append('EOF\n')
    count += 1
    if out != ''.join(exp):
        fails += 1
        print(f'FAIL case {case} sizes {sizes}: {out[:200]!r} vs {"".join(exp)[:200]!r}')
print(f'{count} files, {fails} failures: ' + ('ALL PASS' if not fails else 'FAILED'))
sys.exit(1 if fails else 0)
