"""xsim - run XENIX 2.1.3 (8086) small-model x.out programs on an emulated 8086.

A development aid only: it loads an x.out the way the kernel does (text at CS:0,
data/bss at DS:0, fixed stack above bss, argv/envp above the stack, entry at
CS:0080), then services `int 5` system calls on the host. It implements just the
calls the project's libc uses; real XENIX remains the final test.
Usage: python3 xsim.py prog [args...]"""
import io, os, struct, sys
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
    def __init__(self, path, argv, env, stdin=sys.stdin.buffer, stdout=sys.stdout.buffer, files=None):
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
        self.brk = self.maxbrk = (self.argend + 15) & ~15
        for r, v in [(UC_X86_REG_CS, TSEG), (UC_X86_REG_DS, DSEG), (UC_X86_REG_ES, DSEG),
                     (UC_X86_REG_SS, DSEG), (UC_X86_REG_SP, top), (UC_X86_REG_IP, 0x80)]:
            uc.reg_write(r, v)
        self.umask = 0o022
        self.files = files if files is not None else {0: stdin, 1: stdout, 2: sys.stderr.buffer}
        self.pending_exec = None
        self.child = False             # True in a process made by fork()
        self.pids = {}                 # XENIX pid -> host pid of our children
        uc.hook_add(UC_HOOK_INTR, self.intr)
        # XSIM_PROFILE=file: count the instructions run at each address, and
        # write "address count" lines to the file at the end (tools/xprof.py)
        # XSIM_VSYNC=1: the monochrome display's status port (3BA) has its
        # vertical retrace bit change at every read, as a Hercules card's
        # does now and then; otherwise in reads 0
        if os.environ.get('XSIM_VSYNC'):
            from unicorn.x86_const import UC_X86_INS_IN
            from unicorn import UC_HOOK_INSN
            self.vsync = 0
            def port_in(uc, port, size, data):
                if port == 0x3BA:
                    self.vsync ^= 0x80
                    return self.vsync
                return 0
            uc.hook_add(UC_HOOK_INSN, port_in, None, 1, 0, UC_X86_INS_IN)
        # XSIM_PORTLOG=file: each out instruction's port and byte, a line
        # "port value" in hex
        if os.environ.get('XSIM_PORTLOG'):
            from unicorn.x86_const import UC_X86_INS_OUT
            from unicorn import UC_HOOK_INSN
            def port_out(uc, port, size, value, data):
                with open(os.environ['XSIM_PORTLOG'], 'a') as log:
                    log.write('%x %x\n' % (port, value & 0xff))
            uc.hook_add(UC_HOOK_INSN, port_out, None, 1, 0, UC_X86_INS_OUT)
        self.prof = None
        if os.environ.get('XSIM_PROFILE'):
            from unicorn import UC_HOOK_CODE
            import collections
            self.prof = collections.Counter()
            def count(uc, addr, size, data, prof=self.prof): prof[addr - TSEG * 16] += 1
            uc.hook_add(UC_HOOK_CODE, count)
        self.code = None
        self.sig = [0] * 20
        self.alarm = 0

    def mem(self, off, n): return bytes(self.uc.mem_read(DSEG * 16 + off, n))
    # XSIM_STATMAP: a file of "host-i-number xenix-i-number change-time" lines, for
    # tests that rebuild a tree as it was on XENIX: stat and directories report those
    statmap = {}
    if os.environ.get('XSIM_STATMAP'):
        for l in open(os.environ['XSIM_STATMAP']):
            if l.split(): h, x, c = (int(w) for w in l.split()); statmap[h] = (x, c)
    # XSIM_MAP: "path=hostpath,..." to open another file for a name (/etc/passwd)
    # (a name ending in / maps everything under it: /dev/=/tmp/dev/)
    pathmap = dict(kv.split('=', 1) for kv in os.environ.get('XSIM_MAP', '').split(',') if '=' in kv)
    uid = int(os.environ.get('XSIM_UID') or 0)     # getuid's answer; not 0: stime is refused
    now = None                                      # the clock, once stime has set it

    def mapped(self, name):
        if name in self.pathmap:
            return self.pathmap[name]
        for k in sorted((k for k in self.pathmap if k.endswith('/')), key=len, reverse=True):
            if name.startswith(k):
                return self.pathmap[k] + name[len(k):]
        return name
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
                else:
                    try: f.write(self.mem(cx, si)); f.flush(); r1 = si
                    except OSError as e: err = e.errno or 5      # EPIPE when the reader has gone
            elif ax in (5, 8):              # open(name, flags, mode) / creat(name, mode)
                name = self.mapped(self.cstr(bx))
                if ax == 8:
                    flags, mode = os.O_WRONLY | os.O_CREAT | os.O_TRUNC, cx
                else:
                    flags = (os.O_RDONLY, os.O_WRONLY, os.O_RDWR, os.O_RDWR)[cx & 3]
                    for bit, hf in ((0o400, os.O_CREAT), (0o1000, os.O_TRUNC), (0o2000, os.O_EXCL), (0o10, os.O_APPEND)):
                        if cx & bit: flags |= hf
                    mode = si
                if ax == 5 and (flags & 3) == os.O_RDONLY and os.path.isdir(name):
                    # a directory reads as XENIX keeps it: 16-byte entries, a
                    # 2-byte i-number and a 14-byte name
                    ino = lambda i: self.statmap[i][0] if i in self.statmap else (i & 0xffff) or 1
                    d = io.BytesIO(b''.join(struct.pack('<H14s', ino(os.stat(os.path.join(name, n)).st_ino),
                                   n.encode('latin1')[:14]) for n in ['.', '..'] +
                                   sorted(os.listdir(name), key=lambda n: (os.lstat(os.path.join(name, n)).st_ino, n))))
                    d.dirpath = name
                    fd = min(set(range(20)) - set(self.files)); self.files[fd] = d; r1 = fd
                else:
                  try:
                    hfd = os.open(name, flags, mode & 0o7777 & ~self.umask)
                    acc = flags & 3
                    f = os.fdopen(hfd, 'rb' if acc == os.O_RDONLY else 'wb' if acc == os.O_WRONLY else 'r+b', buffering=0)
                    fd = min(set(range(20)) - set(self.files)); self.files[fd] = f; r1 = fd
                  except FileNotFoundError: err = ENOENT
                  except FileExistsError: err = 17
                  except IsADirectoryError: err = 21
                  except PermissionError: err = 13
            elif ax in (18, 28):            # stat(name, buf) / fstat(fd, buf): System V layout, 30 bytes
                try:
                    if ax == 18:
                        if os.environ.get('XSIM_STATLOG'):     # the names stat'ed, one a line
                            with open(os.environ['XSIM_STATLOG'], 'a') as log: log.write(self.cstr(bx) + '\n')
                        st = os.stat(self.mapped(self.cstr(bx)))
                    else:
                        f = self.files.get(bx)
                        if f is None: raise KeyError
                        st = os.stat(f.dirpath) if hasattr(f, 'dirpath') else os.fstat(f.fileno())
                    m = st.st_mode & 0o7777 | {0o040000: 0o040000, 0o020000: 0o020000, 0o060000: 0o060000,
                                               0o010000: 0o010000}.get(st.st_mode & 0o170000, 0o100000)
                    size, rdev = st.st_size, 0
                    if st.st_mode & 0o170000 == 0o040000:   # a directory: 16 bytes an entry, as on XENIX
                        size = 16 * (len(os.listdir(f.dirpath if ax == 28 and hasattr(f, 'dirpath') else self.mapped(self.cstr(bx)))) + 2)
                    elif st.st_mode & 0o170000 in (0o020000, 0o060000):
                        rdev = (os.major(st.st_rdev) & 0xff) << 8 | os.minor(st.st_rdev) & 0xff
                    ino, ctime = self.statmap.get(st.st_ino, (st.st_ino & 0xffff, int(st.st_ctime)))
                    uc.mem_write(DSEG * 16 + cx, struct.pack('<hHHhHHHllll', st.st_dev & 0x7fff, ino, m,
                        min(st.st_nlink, 32767), st.st_uid & 0xffff, st.st_gid & 0xffff, rdev, min(size, 2**31 - 1),
                        int(st.st_atime), int(st.st_mtime), ctime))
                except FileNotFoundError: err = ENOENT
                except KeyError: err = EBADF
            elif ax == 15:                  # chmod
                try: os.chmod(self.cstr(bx), cx & 0o7777)
                except FileNotFoundError: err = ENOENT
            elif ax == 60: r1, self.umask = self.umask, bx & 0o777   # umask (kept inside the simulator)
            elif ax == 33:                  # access
                name = self.cstr(bx)
                if not os.path.exists(name): err = ENOENT
                elif not os.access(name, cx or os.F_OK): err = 13
            elif ax == 6:
                f = self.files.pop(bx, None)
                if f is None: err = EBADF
                elif bx > 2 or self.child: f.close()
            elif ax == 19:
                f = self.files.get(bx); off = cx | (si << 16)
                if off & 0x80000000: off -= 1 << 32
                if f is None: err = EBADF
                else:
                    try: p = f.seek(off, di); r1, r2 = p & 0xffff, p >> 16
                    except OSError as e: err = e.errno or EINVAL
            elif ax == 9:                   # link(old, new)
                try: os.link(self.cstr(bx), self.cstr(cx))
                except FileNotFoundError: err = ENOENT
                except FileExistsError: err = 17
            elif ax == 30:                  # utime(name, times): times[0] access, times[1] modification
                name = self.cstr(bx)
                try:
                    if cx: at, mt = struct.unpack('<ll', self.mem(cx, 8)); os.utime(name, (at, mt))
                    else: os.utime(name)
                except FileNotFoundError: err = ENOENT
            elif ax == 17:
                if bx < self.argend: err = EINVAL
                else: self.brk = bx; self.maxbrk = max(self.maxbrk, bx)
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
            elif ax == 13:                  # time: XSIM_NOW, or the clock stime set, or the host's
                import time; t = self.now if self.now is not None else int(os.environ.get('XSIM_NOW') or time.time())
                r1, r2 = t & 0xffff, t >> 16
            elif ax == 25:                  # stime: the seconds in BX (low) and CX (high), as on
                if self.uid: err = 1        # XENIX (M62); EPERM but for the super-user
                else: self.now = bx | cx << 16
            elif ax == 20: r1, r2 = (os.getpid() & 0x7fff, os.getppid() & 0x7fff) if self.child else (1234, 1)  # getpid: pid, ppid
            elif ax == 24: r1, r2 = self.uid, self.uid      # getuid: real, effective (XSIM_UID)
            elif ax == 47: r1, r2 = 0, 0    # getgid: real, effective
            elif ax == 2:                   # fork: a real host fork of the whole simulator
                for f in self.files.values():
                    try: f.flush()
                    except Exception: pass
                pid = os.fork()
                if pid == 0:
                    self.child = True
                    r1, r2 = os.getppid() & 0x7fff, 0      # child: BX = 0
                else:
                    self.pids[pid & 0x7fff] = pid
                    r1, r2 = pid & 0x7fff, 1               # parent: AX = child pid
            elif ax == 59:                  # execve(name, argv, envp)
                name = self.cstr(bx)
                def strv(p):
                    v = []
                    while p:
                        q = struct.unpack('<H', self.mem(p, 2))[0]
                        if not q: break
                        v.append(self.cstr(q)); p += 2
                    return v
                if not os.path.isfile(name): err = ENOENT
                elif open(name, 'rb').read(2) != b'\x06\x02': err = 8    # ENOEXEC
                else:
                    self.pending_exec = (name, strv(cx), strv(si))
                    uc.emu_stop(); return
            elif ax == 7:                   # wait: AX = pid, BX = status (exit << 8 | signal)
                try:
                    real, status = os.waitpid(-1, 0)
                    r1, r2 = real & 0x7fff, status & 0xffff
                except ChildProcessError: err = 10                # ECHILD
            elif ax == 42:                  # pipe: AX = read end, BX = write end
                rfd, wfd = os.pipe()
                fr = min(set(range(20)) - set(self.files)); self.files[fr] = os.fdopen(rfd, 'rb', buffering=0)
                fw = min(set(range(20)) - set(self.files)); self.files[fw] = os.fdopen(wfd, 'wb', buffering=0)
                r1, r2 = fr, fw
            elif ax == 41:                  # dup
                f = self.files.get(bx)
                if f is None: err = EBADF
                else:
                    fd = min(set(range(20)) - set(self.files))
                    self.files[fd] = os.fdopen(os.dup(f.fileno()), f.mode, buffering=0); r1 = fd
            elif ax == 10:
                try: os.unlink(self.cstr(bx))
                except FileNotFoundError: err = ENOENT
            elif ax == 43:                  # times: the process's and its children's times, in
                t = os.times()              # ticks of 1/20 s; the result is the ticks since boot
                v = [int(x * 20) for x in (t.user, t.system, t.children_user, t.children_system)]
                uc.mem_write(DSEG * 16 + bx, struct.pack('<4l', *v))
                e = int(t.elapsed * 20) & 0xffffffff; r1, r2 = e & 0xffff, e >> 16
            elif ax == 54:                  # ioctl: ENOTTY, but for the descriptors in
                # XSIM_TTY ("1" or "0 1"), which pass for terminals (isatty), for
                # testing line-buffered output; nothing is filled in
                if str(bx) in os.environ.get('XSIM_TTY', '').split():
                    # XSIM_COLOR: what the console's GIO_COLOR ('c' << 8) answers
                    r1 = int(os.environ['XSIM_COLOR']) if cx == 0x6300 and os.environ.get('XSIM_COLOR') else 0
                else: err = 25
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

    def run(self, limit=int(os.environ.get('XSIM_LIMIT', 50_000_000))):
        try:
            self.uc.emu_start(0x80 + TSEG * 16, 0, count=limit)
        except UcError as e:
            ip = self.uc.reg_read(UC_X86_REG_IP)
            code = f'cpu fault {e} at ip {ip:#x}'
        else:
            code = self.code if self.code is not None else 'instruction limit reached'
        if self.pending_exec:               # execve: replace this program, keep the open files
            name, argv, env = self.pending_exec
            new = Sim(name, argv, env, files=self.files)
            new.child, new.umask = self.child, self.umask
            return new.run(limit)
        if self.child:                      # a forked process ends here, never in the caller
            for f in self.files.values():
                try: f.flush()
                except Exception: pass
            os._exit(code if isinstance(code, int) else 127)
        return code

if __name__ == '__main__':
    prog = sys.argv[1]
    env = ['HOME=/', 'TERM=ansi', 'PATH=/bin:/usr/bin']
    if os.environ.get('XSIM_TZ'):           # pass a time zone through for tests
        env.append('TZ=' + os.environ['XSIM_TZ'])
    for kv in os.environ.get('XSIM_ENV', '').split():   # NAME=value pairs for the program
        env = [e for e in env if e.split('=', 1)[0] != kv.split('=', 1)[0]] + [kv]
    sim = Sim(prog, [prog] + sys.argv[2:], env)
    code = sim.run()
    if sim.prof is not None:
        with open(os.environ['XSIM_PROFILE'], 'w') as f:
            for a, n in sorted(sim.prof.items()): f.write(f'{a:x} {n}\n')
    if os.environ.get('XSIM_BRK'):          # how far the data segment grew
        print(f'xsim: highest break {sim.maxbrk:#x}', file=sys.stderr)
    if isinstance(code, int):
        sys.exit(code)
    print(f'xsim: {code}', file=sys.stderr); sys.exit(127)
