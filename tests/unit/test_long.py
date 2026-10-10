"""Unit tests for lib/crt/long.s: runs the helpers on an emulated 8086 (Unicorn, 16-bit
mode) and compares every result against Python's exact 32-bit arithmetic."""
import os, random, re, struct, subprocess, sys, tempfile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16
from unicorn.x86_const import *

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
tmp = tempfile.mkdtemp()
NAMES = re.findall(r'export\s+(\w+)', open(f'{ROOT}/lib/crt/long.s').read())
START = ''.join(f'\t.globl\t{n}\n' for n in NAMES) + '\t.globl\t_main\n\t.text\n\tnop\n\texport\tstart\nstart:\n\tnop\n\texport\t_main\n_main:\n\tret\n'
open(f'{tmp}/start.s', 'w').write(START)
subprocess.run(['as86', '-0', '-o', f'{tmp}/start.o', f'{tmp}/start.s'], check=True)
subprocess.run(['as86', '-0', '-o', f'{tmp}/long.o', f'{ROOT}/lib/crt/long.s'], check=True)
mp = subprocess.run(['ld86', '-0', '-d', '-M', '-o', f'{tmp}/long.bin', f'{tmp}/start.o', f'{tmp}/long.o'],
                    check=True, capture_output=True, text=True).stdout
SYM = {m.group(1): int(m.group(2), 16) for m in re.finditer(r'long\s+(\w+)\s+\d\s+([0-9a-f]{8})', mp)}
assert 'laddl' in SYM, mp[:500]
CODE = open(f'{tmp}/long.bin', 'rb').read()

M32 = 0xffffffff
s32 = lambda v: v - (1 << 32) if v & 0x80000000 else v
OPND, HALT, STK = 0x8000, 0x7000, 0xF000
FL_C, FL_Z, FL_S, FL_O = 1, 0x40, 0x80, 0x800

def run(name, a, b=0, shift=None, mem=None):
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map(0, 0x10000)
    uc.mem_write(0, CODE)
    uc.mem_write(HALT, b'\xf4')
    uc.mem_write(OPND, struct.pack('<I', b & M32))
    sp = STK - 2
    uc.mem_write(sp, struct.pack('<H', HALT))
    uc.reg_write(UC_X86_REG_SP, sp)
    uc.reg_write(UC_X86_REG_AX, a & 0xffff)
    uc.reg_write(UC_X86_REG_BX, (a >> 16) & 0xffff)
    uc.reg_write(UC_X86_REG_DI, OPND if shift is None else shift)
    keep = {r: random.randrange(0x10000) for r in (UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_BP)}
    for r, v in keep.items(): uc.reg_write(r, v)
    if mem is not None:
        uc.mem_write(OPND, struct.pack('<I', mem & M32))
        uc.reg_write(UC_X86_REG_BX, OPND)
    uc.emu_start(SYM[name], HALT, count=20000)
    for r, v in keep.items():
        assert uc.reg_read(r) == v, f'{name} clobbered a preserved register'
    if mem is not None:
        return struct.unpack('<I', uc.mem_read(OPND, 4))[0]
    res = uc.reg_read(UC_X86_REG_AX) | (uc.reg_read(UC_X86_REG_BX) << 16)
    return res, uc.reg_read(UC_X86_REG_EFLAGS)

def rnd():
    k = random.choice([4, 8, 16, 17, 31, 32])
    v = random.getrandbits(k)
    return random.choice([v, -v & M32, 0, 1, M32, 0x80000000, 0x7fffffff, 0xffff, 0x10000])

fails = 0
def check(cond, msg):
    global fails
    if not cond:
        fails += 1
        if fails < 15: print('FAIL', msg)

random.seed(1986)
for _ in range(4000):
    a, b, n = rnd(), rnd(), random.randrange(0, 40)
    sa, sb = s32(a), s32(b)
    for nm, exp in [('laddl', a + b), ('lsubl', a - b), ('landl', a & b), ('lorl', a | b),
                    ('leorl', a ^ b), ('lmull', a * b), ('lnegl', -a), ('lcoml', ~a)]:
        r, _ = run(nm, a, b); check(r == exp & M32, f'{nm} {a:#x} {b:#x} -> {r:#x}')
    k = n & 31
    r, _ = run('lsll', a, shift=n); check(r == (a << k) & M32, f'lsll {a:#x} {n}')
    r, _ = run('lsrul', a, shift=n); check(r == a >> k, f'lsrul {a:#x} {n}')
    r, _ = run('lsrl', a, shift=n); check(r == (sa >> k) & M32, f'lsrl {a:#x} {n}')
    if b:
        r, _ = run('ldivul', a, b); check(r == a // b, f'ldivul {a:#x}/{b:#x} -> {r:#x}')
        r, _ = run('lmodul', a, b); check(r == a % b, f'lmodul {a:#x}%{b:#x} -> {r:#x}')
        q = int(sa / sb) if abs(sa) < 2**53 else 0
        q = abs(sa) // abs(sb) * (1 if (sa < 0) == (sb < 0) else -1)
        rm = sa - q * sb
        if not (sa == -2**31 and sb == -1):
            r, _ = run('ldivl', a, b); check(r == q & M32, f'ldivl {sa}/{sb} -> {s32(r)} want {q}')
            r, _ = run('lmodl', a, b); check(r == rm & M32, f'lmodl {sa}%{sb} -> {s32(r)} want {rm}')
    # compares: emulate the jumps BCC emits
    _, f = run('lcmpl', a, b)
    zf, sf, of, cf = bool(f & FL_Z), bool(f & FL_S), bool(f & FL_O), bool(f & FL_C)
    check(zf == (sa == sb), f'lcmpl eq {sa} {sb}')
    check((sf != of) == (sa < sb), f'lcmpl lt {sa} {sb}')
    check((zf or (sf != of)) == (sa <= sb), f'lcmpl le {sa} {sb}')
    _, f = run('lcmpul', a, b)
    zf, cf = bool(f & FL_Z), bool(f & FL_C)
    check(zf == (a == b) and cf == (a < b), f'lcmpul {a:#x} {b:#x}')
    _, f = run('ltstl', a)
    zf, sf, of = bool(f & FL_Z), bool(f & FL_S), bool(f & FL_O)
    check(zf == (sa == 0) and (sf != of) == (sa < 0), f'ltstl {sa}')
    check(run('lincl', 0, mem=a) == (a + 1) & M32, f'lincl {a:#x}')
    check(run('ldecl', 0, mem=a) == (a - 1) & M32, f'ldecl {a:#x}')

print(f'{4000} rounds, {len(SYM)} helpers: {"ALL PASS" if fails == 0 else f"{fails} FAILURES"}')
sys.exit(1 if fails else 0)
