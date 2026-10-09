# xenix86-devsys

A from-scratch **Development System for SCO XENIX System V 2.1.3 on the Intel 8086/8088**.

XENIX 2.1.3 for the 8086 shipped with an empty slot for a Development System
(C compiler, assembler, libraries, `make`, and the rest), sold separately by SCO.
No copy of that 8086 Development System is known to survive. This project builds
a new one: open source, written in 2026, installable through XENIX's own
`custom` utility, the way the original would have been.

The target machine is a 1986 Wyse WY-1400-20 (WYSEpc+), an IBM PC/XT-class
computer with a backplane design, being restored to run XENIX with an NEC V20.
Development and testing happen in 86Box on an emulated XT.

## Status

**Work in progress.**

- [x] XENIX 2.1.3 (8086) installed and verified in 86Box (IBM/Xebec MFM controller)
- [x] Program ABI reverse-engineered and documented, see [ABI.md](ABI.md)
- [x] Milestone 1: project-built `hello, world` runs on XENIX, both as a directly
      generated x.out executable and as an OMF object linked by SCO's native `ld`
- [x] Milestone 2: a K&R C program compiled with BCC and linked against this
      project's own crt0, system call stubs and libc runs on XENIX (argv, environment,
      printf, exit status)
- [ ] C compiler (K&R) running natively on XENIX (cross-compiling with BCC works today)
- [x] Dynamic memory: sbrk, malloc, free, calloc, realloc (heap up to ~59K)
- [x] Milestone 3: real C library: buffered stdio (FILE, fopen, fgets, fprintf, sprintf ...),
      32-bit long arithmetic, ctype, string routines. A `wc` clone built with it gives the
      same counts as SCO's own `wc` on XENIX
- [x] Milestone 4: process control: fork, the exec family (with PATH search), wait, pipe,
      signals (signal, kill, alarm, pause), sleep, system, time, process and user ids
- [ ] setjmp/longjmp
- [ ] Assembler, `ar`, `nm`, `size`, `strip`, `make`, `lex`/`yacc`, debugger
- [ ] Remaining standard C headers (stdio.h, string.h, ctype.h, errno.h, signal.h
      and time.h are done)
- [ ] Packaging as an installable `custom` set (the SOFT package)
- [ ] Languages beyond C and assembly: FORTRAN (using the 8087), COBOL and BASIC subsets,
      possibly Pascal later
- [ ] Text Processing System (nroff/troff, tbl, eqn)

## Layout

    ABI.md          verified rules for XENIX 2.1.3 8086 executables
    tools/omf.py    writes Intel OMF object modules that SCO's ld accepts
    tools/xout.py   writes XENIX x.out executables directly
    tools/crt.py    the text reserved block and data header XENIX expects
    tools/gensys.py generates the system call stubs (lib/sys.s)
    tools/aout2xout.py  converts ld86 output into a XENIX x.out executable
    tools/xsim.py   runs XENIX x.out programs on an emulated 8086 for testing
    include/        C headers
    lib/            crt0, system call stubs, runtime helpers and libc
    build.sh        builds the runtime and a C program: ./build.sh prog.c
    test.sh         runs the unit and program tests on the host
    tests/          the test programs from each round of experiments

## Building a C program

Install BCC and the 8086 binutils (on Debian/Ubuntu: `apt install bcc bin86`), then:

    ./build.sh tests/m9.c        # produces build/m9, a XENIX executable
    ./build.sh tests/regdump.c tests/regdump.s   # C and assembly can be mixed

## Testing

`./test.sh` runs the 32-bit arithmetic helpers on an emulated 8086 against Python's exact
arithmetic, then builds the test programs and runs them under `tools/xsim.py`, a small
simulator that loads XENIX executables and services their system calls on the host.
It needs the Python `unicorn` package (`pip install unicorn`). Everything is also
checked on a real XENIX 2.1.3 installation in 86Box before it counts as done.

## Running programs on XENIX

The tools need Python 3. The build scripts in `tests/` write `.o` files or
executables. To move them into XENIX, pack them as a v7-format tar on a 360K
floppy image:

    gtar --format=v7 --owner=0 --group=0 -cf disk.tar <files>
    # pad to 368640 bytes, insert in 86Box, then on XENIX:
    tar xvf /dev/fd0

## How it was made

This project is built with heavy AI assistance (Anthropic's Claude) working alongside its
author. Every result claimed here was verified on a real XENIX 2.1.3 system, and the
reverse-engineering findings in ABI.md come from experiments on that system, including the
ones where the first theory turned out to be wrong.

## What this repository does not contain

No SCO, Microsoft, AT&T or IBM code, binaries or disk images. Everything here was
written for this project. To use it, you need your own XENIX 2.1.3 installation.

## License

GPL version 2 or (at your option) any later version. See [LICENSE](LICENSE).
