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
- [ ] crt0 startup and the first pieces of libc
- [ ] C compiler (K&R), cross-built first, then self-hosting on XENIX
- [ ] Assembler, `ar`, `nm`, `size`, `strip`, `make`, `lex`/`yacc`, debugger
- [ ] Standard C headers (`stdio.h` and friends)
- [ ] Packaging as an installable `custom` set (the SOFT package)
- [ ] Stretch goals: FORTRAN and COBOL subsets, Text Processing System

## Layout

    ABI.md          verified rules for XENIX 2.1.3 8086 executables
    tools/omf.py    writes Intel OMF object modules that SCO's ld accepts
    tools/xout.py   writes XENIX x.out executables directly
    tools/crt.py    the text reserved block and data header XENIX expects
    tests/          the test programs from each round of experiments

## Running the tests

The tools need Python 3. The build scripts in `tests/` write `.o` files or
executables. To move them into XENIX, pack them as a v7-format tar on a 360K
floppy image:

    gtar --format=v7 --owner=0 --group=0 -cf disk.tar <files>
    # pad to 368640 bytes, insert in 86Box, then on XENIX:
    tar xvf /dev/fd0

## What this repository does not contain

No SCO, Microsoft, AT&T or IBM code, binaries or disk images. Everything here was
written for this project. To use it, you need your own XENIX 2.1.3 installation.

## License

GPL version 2 or (at your option) any later version. See [LICENSE](LICENSE).
