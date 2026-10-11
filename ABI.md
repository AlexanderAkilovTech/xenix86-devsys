# Xenix 2.1.3 (8086) program ABI, as verified on a running system

Milestone 1 reached 2026-10-09: `hello, world` built by this project's tools ran on
SCO Xenix System V 2.1.3 (i8086), both written directly as x.out and linked by SCO's
native `ld` from our OMF objects.

## Executable (x.out, segmented, small model, separate I&D)
- Header: magic 0x0206, x_cpu = 0x44 (XC_8086|XC_WSWAP), x_renv = 0xC80F
  (OS-version, segmented, fixed stack, pure text, separate I&D, executable),
  xe_stksize = 1024, ostype 1, osvers 2, text segment number 63, data 71.
- SCO `ld` takes x_cpu from the OMF COMENT class 0x9D record ("0sO" = 8086, small).
  Without it x_cpu is 0 and the kernel refuses to exec the file.
- Link: `ld -i -F 400 -o prog prog.o` (-i separate I&D, -F stack size in hex).
- Data and bss are kept in whole 16-byte paragraphs. An odd-sized data segment
  panicked the kernel at process exit.

## Symbol table (verified with SCO's adb)
- An executable's symbols are a third segment table entry, type 3 (symbol table),
  attributes 1, after text and data; x_syms in the header holds its size.
- Each symbol is 8 bytes, then its name and a NUL: a type word (1 absolute,
  2 text, 3 data, 4 bss, 8 a segment's name; 0x20 added for an external), the
  segment number (63 text, 71 data), and a 32-bit value. The segment names _TEXT
  and _DATA come first. tools/aout2xout.py and cmd/aout2x.c write it.
- With it, adb shows C names: `_main:br` sets a breakpoint (`_main:b` does not
  work: `:` there reads as segment:offset), `:r` runs, `:co` continues, `:s`
  steps, `:k` kills, `$c` lists the calls, `_count/d` prints a variable,
  `_main,8?ia` disassembles, `$q` quits. A breakpoint stops at the function's
  first instruction, before its frame is made, so `$c` then shows its caller.

## Text segment: reserved block 0x00-0x7F (fixed offsets, the kernel depends on them)
    00: jmp 0x80           entry: execution really starts at 0x80 (x_entry is ignored)
    02: jmp 0x17           system call gate
    04: jmp 0x14           stack-growth request
    06: jmp 0x14
    08..12: jmp $          unused traps
    14: mov ax,0828h       stack-growth system call
    17: int 5              the only kernel entry
    19: jmp 0x7f
    7F: ret
Moving the int 5 away from 0x17 makes system calls that *return* fail
(exit still works, since it never returns).

## Data segment header
    DS:0 = 0x0000 (datloc0)    DS:2 = 0xFFFF (stack limit, STKHQQ)
Program data starts at DS:4. Putting program data here corrupts the return frame
on the first returning system call (SIGILL, core dumped).

## System calls
- AX = call number (Version 7 numbering: 1 exit, 3 read, 4 write, 5 open, 6 close,
  20 getpid, 48 signal ...); Xenix extensions use AX = (sub << 8) | 0x28.
- Arguments: BX, CX, SI, DI (arg1..arg4). Some library stubs instead pass BX = pointer
  to the argument list on the stack.
- Call the gate with a near `call 2`. On error the carry flag is set and AX = errno.

## Return values (measured with tests/regdump.s)
- AX holds the result. A second word comes back in BX, and the kernel copies it
  to DX as well. CX is left alone.
- time (13): AX = low word, BX = high word of the seconds since 1970.
- getpid (20): AX = pid, BX = parent pid. There is no separate getppid call (V7 style).
- getuid (24) / getgid (47): AX = real id, BX = effective id.
- fork (2): AX = the other process's pid; BX = 0 in the child, nonzero in the parent.
- wait (7): AX = pid of the child, BX = its status. pipe (42): AX = read fd, BX = write fd.
- signal (48): the kernel is given a handler address in our text. On delivery it
  pushes FLAGS, CS, IP on the user stack and jumps there; the interrupted call
  fails with EINTR (4). The handler is reset to SIG_DFL (V7 semantics).
- In 16-bit C an undeclared function returns int, so `time()` must be declared
  `long` (include/time.h) or its high word is lost.

## Process start
SP points at argc, followed by the argv pointers, NULL, then the envp pointers, NULL.

## Process memory layout (small model, measured)
    DS:0 [ data | bss ][ fixed stack (xe_stksize) ][ argv/envp + strings ][ heap ->
- The stack sits directly above bss; the kernel places argc/argv/envp and the
  argument strings above the stack.
- brk() to an address below the end of the argument strings fails with EINVAL (22),
  so the heap must start above the highest argv/envp string, not at `_end`.
- brk() (V7 call 17, BX = new break) succeeds up to just under 64K: a test program
  reached a break of 0xFC1C, about 59K of heap.
- Because the stack is below the heap, code written for ELKS or Minix, where the
  heap grows up toward a stack at the top of the segment, must not size its heap
  from the stack pointer (BCC's as86 did; see ports/bcc patch 03).
- `sbrk` takes an int, so a request of 32K or more is negative to it; our malloc
  grows the heap in two halves for those.
- crt0 sets ES = DS rather than relying on the kernel to, because the library's
  string routines use the 8086 string instructions (movs, stos, scas), which
  address their destination through ES.

## Files (measured with tests/sysprobe.c)
- open (5) takes System V flags, with the mode as the third argument:
  O_RDWR | O_CREAT | O_TRUNC (2 | 0400 | 01000) created a file, and O_WRONLY | O_TRUNC
  emptied it. The other System V values in include/fcntl.h (O_NDELAY 04,
  O_APPEND 010, O_EXCL 02000) are not yet tested.
- stat (18) and fstat (28) fill the System V structure, 30 bytes:
  dev (short), ino (unsigned short), mode (unsigned short), nlink (short), uid,
  gid (unsigned short), rdev (short), size, atime, mtime, ctime (long).
  /etc/passwd read back as size 432, mode 0100644, uid 3 (bin), matching `ls -l`.
- There is no rename system call; the library's rename() links the new name and
  unlinks the old one, as Version 7 did.

## Dates
- XENIX 2.1.3's date command will not take a year past 1999, so a clock set for
  2026 is set to 1998, which has the same calendar. Files carried in from a
  modern machine then look decades newer than anything built on XENIX, and make
  rebuilds everything every time; tools/mkdisk.sh dates the files on its floppy
  images 1 January 1986.

## Installation sets (custom), verified with `custom -d`
- `custom`'s Development System entry installs two permissions lists: the
  Operating System's own `/etc/dsmd.perms`, whose package (adb and
  `/usr/include/sys`) is on SCO's disk N05, and `/etc/soft.perms`, which the
  Development System's disks (volumes D01, D02 ...) must supply.
- Each disk is a version 7 tar with `./` names. The first member is the label
  file `./tmp/_lbl/prd=xnxsv/ver=ALL86/rel=2.1.3/vol=D01`, by which custom
  recognizes the disk; disk D01 also carries the permissions list, stored with
  its second name `./etc/perms/soft` as a hard link.
- The permissions list gives, per file: package, type and mode (`d755`, `f644`,
  `x711`), owner/group, link count, path, volume. `#!PKG size description` lines
  declare the packages; sizes are 512-byte blocks, indirect blocks included.
  tools/mkset.py writes all of this; the files are owned by bin (3/3) like SCO's.
- Once installed, `custom` keeps using the installed `/etc/soft.perms` (and its
  link `/etc/perms/soft`) when the set is installed again, not the list on the new
  D01: with a newer set laid out differently, it looked for files on D01 that had
  moved to D02 ("tar: 13 file(s) not extracted"), and with the old one-disk list it
  never asked for D02. Removing both names of the old list first makes it read the
  new one (verified: D01, D02 and N05 were then asked for and installed).

- `custom` option 3 pipes `fixperm -l` through `pr -2ptl24`: blank while fixperm
  works, then a bell and a wait for Return before each page. Returns typed ahead
  reach the main menu as empty answers, each repeating the menu (one `rm` per
  pass, a second or two on an XT); the menu loop does not test its `read`, and
  SIGINT is trapped back to the menu, while SIGQUIT (`Ctrl-\`) keeps the script's
  first trap, which exits (read from the script, not tried). Verified: one Return
  per bell, then q, quits cleanly.

## The 8087 (measured with tests/fpu87.c on 86Box, 8087 fitted and not)
- `fninit` leaves the control word 0x03FF and status word 0x0000 on an 8087; with
  no 8087, `fnstcw`/`fnstsw` write nothing (memory set to 0x5A5A beforehand stays),
  which is how a program can tell.
- The kernel does not save or restore the 8087's state when it switches
  processes. Two processes, one with its own control word (rounding toward zero),
  each repeatedly loading values and checking them: one of them saw wrong values,
  and a wrong control word, in 13,040 of 16,100 checks. With the header flag
  `XE_FPH` (x_renv 0x0080, "floating point hardware required" in SCO's
  `sys/a.out.h`) set, both processes were corrupted; the kernel also ran that
  program on a machine without an 8087. So XE_FPH does not make the kernel save the
  8087, and an 8087 can be used safely by only one process at a time.

## The shell
- XENIX 2.1.3's `/bin/sh` has no shell functions: `name() { ... }` is a syntax
  error ("'(' unexpected"). Scripts meant for it use only `for`, `if`, `case`,
  `while` and plain commands.

## Commands present
- XENIX 2.1.3 has `/usr/bin/more` (with `--More--(15%)`) but no `pg`, and no
  `nroff`, so the manual is formatted ahead of time (tools/man.py) and `man`
  pages it through `more`.
