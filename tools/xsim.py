"""xsim - run XENIX 2.1.3 (8086) small-model x.out programs on an emulated 8086.

A development aid only: it loads an x.out the way the kernel does (text at CS:0,
data/bss at DS:0, fixed stack above bss, argv/envp above the stack, entry at
CS:0080), then services `int 5` system calls on the host. It implements just the
calls the project's libc uses; real XENIX remains the final test.
Usage: python3 xsim.py prog [args...]"""
import os, struct, sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INTR, UcError
from unicorn.x86_const import *

TSEG, DSEG = 0x1000, 0x2000        # paragraphs: text at 64K, data at 128K
EINVAL, EBADF, ENOENT = 22, 9, 2

class Exit(Exception):
    def __init__(self, code): self.code = code

def load(path):
    d = open(path, 'rb').read()
    magic, ext, tsz, dsz, bss, syms, rel, entry, cpu, relsym, renv = struct.unpack_from('<HHlllllLBBH', d)
    assert magic == 0x206 and cpu & 0x3f == 4, 'not an 8086 x.out'
    e = struct.unpack_from('<9lBBBBHH', d, 32)
    stksize, segpos, segsize = e[4], e[5], e[6]
    segs = [struct.unpack_from('<HHHBBllllHHl', d, segpos + i) for i in range(0, segsize, 32)]
    text = data = b''; vdata = 0
    for s in segs:
        if s[0] == 1: text = d[s[5]:s[5] + s[6]]
        if s[0] == 2: data, vdata = d[s[5]:s[5] + s[6]], s[7]
    return text, data, vdata, stksize

class Sim:
    def __init__(self, path, argv, env, stdin=sys.stdin.buffer, stdout=sys.stdout.buffer):
        text, data, vdata, stk = load(path)
        self.uc = uc = Uc(UC_ARCH_X86, UC_MODE_16)
        uc.mem_map(0, 0x100000)
        uc.mem_write(TSEG * 16, text)
        uc.mem_write(DSEG * 16, data)
        # argv/envp block above the fixed stack
        top = vdata + stk
        strs, ptrs = bytearray(), []
        base = top + 2 * (1 + len(argv) + 1 + len(env) + 1)
        for s in list(argv) + list(env):
            ptrs.append(base + len(strs)); strs += s.encode() + b'\0'
        vec = [len(argv)] + ptrs[:len(argv)] + [0] + ptrs[len(argv):] + [0]
        uc.mem_write(DSEG * 16 + top, struct.pack(f'<{len(vec)}H', *vec) + bytes(strs))
        self.argend = base + len(strs)
        self.brk = (self.argend + 15) & ~15
        for r, v in [(UC_X86_REG_CS, TSEG), (UC_X86_REG_DS, DSEG), (UC_X86_REG_ES, DSEG),
                     (UC_X86_REG_SS, DSEG), (UC_X86_REG_SP, top), (UC_X86_REG_IP, 0x80)]:
            uc.reg_write(r, v)
        self.files = {0: stdin, 1: stdout, 2: sys.stderr.buffer}
        uc.hook_add(UC_HOOK_INTR, self.intr)
        self.code = None
        self.sig = [0] * 20
        self.alarm = 0

    def mem(self, off, n): return bytes(self.uc.mem_read(DSEG * 16 + off, n))
    def cstr(self, off):
        b = bytearray()
        while True:
            c = self.mem(off + len(b), 1)
            if c == b'\0': return b.decode('latin1')
            b += c

    def intr(self, uc, intno, _):
        if intno != 5:
            uc.emu_stop(); self.code = f'unexpected interrupt {intno:#x}'; return
        ax, bx, cx, si, di = (uc.reg_read(r) for r in (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_SI, UC_X86_REG_DI))
        err, r1, r2 = 0, 0, 0
        try:
            if ax == 1: raise Exit(bx & 0xff)
            elif ax == 3:
                f = self.files.get(bx)
                if f is None: err = EBADF
                else:
                    b = f.read1(si) if hasattr(f, 'read1') else f.read(si)
                    uc.mem_write(DSEG * 16 + cx, b); r1 = len(b)
            elif ax == 4:
                f = self.files.get(bx)
                if f is None: err = EBADF
                else: f.write(self.mem(cx, si)); f.flush(); r1 = si
            elif ax in (5, 8):
                name = self.cstr(bx)
                mode = {0: 'rb', 1: 'r+b', 2: 'r+b'}[cx & 3] if ax == 5 else 'w+b'
                try:
                    f = open(name, mode)
                    fd = min(set(range(20)) - set(self.files)); self.files[fd] = f; r1 = fd
                except FileNotFoundError: err = ENOENT
            elif ax == 6:
                f = self.files.pop(bx, None)
                if f is None: err = EBADF
                elif bx > 2: f.close()
            elif ax == 19:
                f = self.files[bx]; off = cx | (si << 16)
                if off & 0x80000000: off -= 1 << 32
                p = f.seek(off, di); r1, r2 = p & 0xffff, p >> 16
            elif ax == 17:
                if bx < self.argend: err = EINVAL
                else: self.brk = bx
            elif ax == 48:                  # signal: remember the trampoline address
                if not 1 <= bx < 20: err = EINVAL
                else: r1, self.sig[bx] = self.sig[bx], cx
            elif ax == 27: r1, self.alarm = self.alarm, bx
            elif ax == 29:                  # pause: deliver a pending alarm, else stop
                if self.alarm and self.sig[14] > 1:
                    self.alarm = 0
                    self.deliver(14)
                    return
                err = 4
            elif ax == 13: import time; t = int(time.time()); r1, r2 = t & 0xffff, t >> 16
            elif ax == 20: r1, r2 = 1234, 1     # getpid: pid, ppid (V7 style)
            elif ax in (24, 47): r1, r2 = 0, 0  # getuid/getgid: real, effective
            elif ax == 10:
                try: os.unlink(self.cstr(bx))
                except FileNotFoundError: err = ENOENT
            elif ax == 54: err = 25        # ioctl: ENOTTY
            else: err = EINVAL
        except Exit as e:
            self.code = e.code; uc.emu_stop(); return
        fl = uc.reg_read(UC_X86_REG_EFLAGS)
        if err:
            uc.reg_write(UC_X86_REG_AX, err); uc.reg_write(UC_X86_REG_EFLAGS, fl | 1)
        else:
            uc.reg_write(UC_X86_REG_AX, r1 & 0xffff); uc.reg_write(UC_X86_REG_BX, r2 & 0xffff)
            uc.reg_write(UC_X86_REG_DX, r2 & 0xffff)   # XENIX copies the second word to DX too
            uc.reg_write(UC_X86_REG_EFLAGS, fl & ~1)

    def deliver(self, sig):
        """Interrupt the program the way XENIX does: the interrupted call fails with
        EINTR, FLAGS/CS/IP are pushed on the user stack, and control goes to the
        trampoline registered for the signal (reset to SIG_DFL, V7 style)."""
        uc = self.uc
        fl = uc.reg_read(UC_X86_REG_EFLAGS) | 1
        uc.reg_write(UC_X86_REG_AX, 4)
        sp = uc.reg_read(UC_X86_REG_SP) - 6
        uc.mem_write(DSEG * 16 + sp, struct.pack('<HHH', uc.reg_read(UC_X86_REG_IP), TSEG, fl))
        uc.reg_write(UC_X86_REG_SP, sp)
        uc.reg_write(UC_X86_REG_EFLAGS, fl)
        uc.reg_write(UC_X86_REG_IP, self.sig[sig])
        self.sig[sig] = 0

    def run(self, limit=50_000_000):
        try:
            self.uc.emu_start(0x80 + TSEG * 16, 0, count=limit)
        except UcError as e:
            ip = self.uc.reg_read(UC_X86_REG_IP)
            return f'cpu fault {e} at ip {ip:#x}'
        return self.code if self.code is not None else 'instruction limit reached'

if __name__ == '__main__':
    prog = sys.argv[1]
    code = Sim(prog, [prog] + sys.argv[2:], ['HOME=/', 'TERM=ansi', 'PATH=/bin:/usr/bin']).run()
    if isinstance(code, int):
        sys.exit(code)
    print(f'xsim: {code}', file=sys.stderr); sys.exit(127)
