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
- [x] Dynamic memory: sbrk, malloc, free, calloc, realloc (heap up to ~59K)
- [x] Milestone 3: real C library: buffered stdio (FILE, fopen, fgets, fprintf, sprintf ...),
      32-bit long arithmetic, ctype, string routines. A `wc` clone built with it gives the
      same counts as SCO's own `wc` on XENIX
- [x] Milestone 4: process control: fork, the exec family (with PATH search), wait, pipe,
      signals (signal, kill, alarm, pause), sleep, system, time, process and user ids
- [x] setjmp/longjmp
- [x] Milestone 5: **a native C compiler on XENIX.** `cc` runs the BCC preprocessor,
      compiler, assembler and linker (see [ports/bcc](ports/bcc/README.md)) as XENIX
      programs. `hello.c` compiled on XENIX is byte-identical to the cross-compiled
      program, and in the simulator the native toolchain rebuilds its own compiler
      byte for byte. The C library is now an archive, `libc.a`, so programs carry only
      what they use
- [x] Milestone 6: `make` and `ar` on XENIX. `make` is this project's own, written in K&R C
      after the make of Version 7 and System V (macros, suffix rules, the automatic
      macros, `::` rules, `include`, `-n -s -i -k -t -q -p -e -r`); `ar86` is GNU ar from
      Dev86. A three-file project with its own library builds, rebuilds only what changed,
      and runs, all on XENIX. The C library gained the scanf family, perror and strerror,
      rename, memmove and the BSD byte routines
- [x] Milestone 7: **installs with XENIX's own `custom`.** The Development System is a
      set of 360K disks (D1, ...) that `custom -d` installs into the slot XENIX 2.1.3
      left for it. As SCO laid it out, `custom` also asks for the Operating System's
      disk N5, which carries SCO's `adb` and `/usr/include/sys`; this set supplies
      the rest. After the install, `make` built a three-file project with the
      installed `cc` and `ar86`
- [x] Milestone 8: **floating point.** `float` and `double` work, in software, so no
      8087 is needed: IEEE double precision arithmetic, correctly rounded (checked
      against Python on random operands, infinities, NaNs and denormals included);
      `printf`'s `%e %f %g`, `scanf` and `strtod`, exact to the last digit; and a math
      library, `libm.a` (`cc ... -lm`): `sqrt`, `exp`, `log`, `log10`, `pow`, the
      trigonometric, inverse trigonometric and hyperbolic functions, `atan2`, `hypot`,
      `fmod`, `floor`, `ceil` and the rest, each within 1.5 units in the last place of the
      exact result (`sqrt` correctly rounded), with System V's `matherr` error handling.
      The compiler on XENIX handles floating point too; to fit it in 64K, `cc` gained
      `-O` (Dev86's peephole optimizer, made to run on XENIX) and `-Mc`. Float programs
      compiled on XENIX by its own `cc` give results identical to the bit to the
      cross-compiled ones
- [x] Milestone 9: **developer tools.** Programs keep a symbol table in XENIX's own
      format, so SCO's debugger `adb` (from the Operating System's disk N5) shows C
      names: breakpoints by function, stack traces, variables (`cc -s` or `strip`
      leave it out). New commands: `nm`, `size` and `strip`, which read executables,
      object files and libraries; `clear`; and `man`, with manual pages for every
      command and library function of the Development System, sections 1 and 3, in
      the style of BSD's manual, each with a HISTORY of where the command or function
      came from (only what was so by 1986), formatted ahead of time since XENIX 2.1.3
      has no `nroff`. A termcap library,
      `libtermcap.a` (`cc ... -ltermcap`), reads XENIX's own `/etc/termcap`: checked
      against every entry there
- [x] Milestone 10: **system tools.** A screen library, `libcurses.a` (`cc ... -lcurses
      -ltermcap`). `ls` and `df` in place of XENIX's own: the same output as XENIX's,
      checked against them on the same files, with an option `-h` for sizes in K and M;
      `df -v`, the table `custom` shows as the disk usage, is in K and M too.
      `whereis`. And `xtop` (also `top`), a full-screen view of the machine after `top`
      and `htop`: meters for the processor, all 640K of memory, the swap area and each
      file system, the processes with their terminals, the swapping as it happens,
      whether an 8087 is fitted, and the display adapter (CGA, MDA or Hercules), in a
      Color or a Monochrome theme. New accounts that `mkuser` makes get a C shell prompt
      showing the current directory, in color on the console. To make `xtop` cheap
      enough for a 4.77 MHz 8088, `printf`, `fgets` and the termcap library got faster;
      it takes about an eighth of the processor at 3 seconds between screens, and starts
      in about 3 seconds. Since v0.10.0: `hostname` (XENIX keeps the machine's name in
      `/etc/systemid`), `cursor` for a block cursor on the console, `lastlogin` for the
      "Last login" that XENIX's login does not print, `~` for the home directory in
      the C shell prompt, which can also go without its colors, and `date` in place of
      XENIX's own, which cannot be set past 1999 and shows any date from February 29,
      2000 on a day ahead (it takes 2000 for a common year): this one sets and shows
      any date up to 2106, taking the time as people write it (`date 3:36 PM`,
      `date 10/14 9:05 AM`), and the library's time functions follow the daylight
      saving rules of 2007 on as well. `asktime`, the time prompt at every start, takes
      the time the same way; `uptime`; and `hostname` sets the kernel's node name too,
      so `uname -n` shows it (all verified on XENIX, M66)
      Since v0.10.1: `sudo` (who may, and whose password, in /etc/default/sudo),
      `shutdown -h now`, `crypt` and `getpass` in the C library, `$USER` and
      `$LOGNAME`, and a block cursor at every start (verified on XENIX, M70, M71)
- [ ] An 8087 version of the floating point. XENIX 2.1.3 does not save the 8087's
      registers when it switches processes (measured: two programs using it at once
      corrupt each other's results), so it can only ever be an option
- [ ] `lex` and `yacc`
- [ ] Remaining standard C headers (stdio.h, stdlib.h, string.h, ctype.h, errno.h,
      signal.h, setjmp.h, time.h, fcntl.h, unistd.h, math.h, float.h and values.h are
      done; `sys/` is SCO's, from N5)
- [ ] Languages beyond C and assembly: FORTRAN (using the 8087), COBOL and BASIC subsets,
      possibly Pascal later
- [ ] Text Processing System (nroff/troff, tbl, eqn)

## Layout

    ABI.md          verified rules for XENIX 2.1.3 8086 executables
    tools/omf.py    writes Intel OMF object modules that SCO's ld accepts
    tools/xout.py   writes XENIX x.out executables directly
    tools/crt.py    the text reserved block and data header XENIX expects
    tools/gensys.py generates the system call stubs, one module per call
    tools/aout2xout.py  converts ld86 output into a XENIX x.out executable
    tools/xsim.py   runs XENIX x.out programs on an emulated 8086 for testing
    tools/mkdisk.sh packs files into a 360K floppy image for XENIX
    tools/mkset.py  makes the installable set: soft.perms and the D disk images
    tools/mathcoef.py   derives and checks the math library's polynomials
    tools/objclass.py   sets an object module's link class (how printf's
                    floating point conversions are linked only when used)
    tools/arfix.py  gives library archives fixed dates and owners, for
                    reproducible builds
    tools/man.py    formats the manual pages as plain text, and makes the
                    whatis index that man -k reads
    include/        C headers; include/sys holds stand-ins used only for
                    cross-compiling (on XENIX, /usr/include/sys is SCO's)
    lib/            the C library, laid out like Version 7's: csu/ (crt0), crt/
                    (compiler support), sys/ (system calls), gen/, stdio/, fp/
                    (software floating point); m/, the math library; and
                    termcap/ and curses/, the termcap and screen libraries
    cmd/            XENIX commands of our own: cc, make, aout2x, nm, size,
                    strip (with objfile.c, which reads the file formats),
                    clear and man; and the system tools ls, df, date,
                    asktime, whereis, xtop, hostname, cursor, lastlogin and
                    uptime (date, asktime and uptime with clock.c; xtop and
                    cursor with hwprobe.s,
                    which looks for an 8087 and a Hercules card and sets
                    the display adapter's registers)
    skel/           the shell start-up files that mkuser copies into new
                    accounts, and cdprompt.csh and setprompt.csh, the C
                    shell prompt's cd and the prompt itself
    man/            the manual pages, in -man form: man1 commands, man3
                    library functions
    ports/bcc/      the BCC compiler, assembler and linker, built for XENIX
    build.sh        builds crt0.o, libc.a and a C program: ./build.sh prog.c
    dist.sh         builds everything, laid out as it installs on XENIX
    test.sh         runs the unit and program tests on the host
    tests/          the test programs from each round of experiments

## Building a C program

Install BCC and the 8086 binutils (on Debian/Ubuntu: `apt install bcc bin86`), then:

    ./build.sh tests/m9.c        # produces build/m9, a XENIX executable
    ./build.sh tests/regdump.c tests/regdump.s   # C and assembly can be mixed

## Building the Development System

    ./dist.sh

This builds the C library, fetches BCC's source (checked by SHA-256; see
[ports/bcc](ports/bcc/README.md)), builds the toolchain as XENIX programs, and lays
out the result under `build/xenix/root` as it installs: `cc`, `make`, `as86`, `ld86`,
`ar86`, `nm`, `size`, `strip`, `clear` and `man` in `/usr/bin`; the compiler passes, the
optimizer `copt` and its rules, `crt0.o`, `libc.a`, `libm.a`, `libtermcap.a` and
`libcurses.a` in `/usr/lib/cc`; headers in `/usr/include`; the formatted manual in
`/usr/man`; and the system tools: `ls`, `df` and `date` in `/bin`, `whereis`, `xtop`,
`hostname`, `cursor`, `lastlogin` and `uptime` in `/usr/bin`, `asktime` and
`default/date` in `/etc`, and the shell start-up files in `/usr/lib/mkuser`. Then it makes that tree
into an installable set under `build/xenix/set`: the disk images `D01.img`, `D02.img`
and `D03.img` and the permissions list `soft.perms` they carry. The build is
reproducible: the same source gives the same disk images, byte for byte, whenever
and wherever it is built (every date in them is 1 January 1986; see `tools/arfix.py`).

To try the tools on XENIX without installing, unpack the tree into a directory and
set `CCROOT` to it; `cc` then finds everything there.

## Installing on XENIX

Log in as root and run

    custom -d

which installs the Development System (pick the whole set, ALL). `custom` asks for
volume D1: insert `D01.img`, then `D02.img` and `D03.img` as it asks for them. It
also asks for the Operating System's volume N5, your own SCO disk, which holds `adb`
and the system headers in `/usr/include/sys` that the Development System uses. When
it has checked the file permissions, `cc`, `make`, `as86`, `ld86`, `ar86`, `nm`,
`size`, `strip`, `clear` and `man` are in `/usr/bin`. `man intro` lists the
commands, and `man 3 intro` the libraries. `custom` can also list, reinstall or
remove the set, the same as SCO's own. On a 640K machine, `cron` may print "cannot
fork" while `custom` reads N5; the extraction carries on underneath, so let it
finish.

The set has three packages, and any of them can be installed alone: SOFT, the
compiler, libraries and headers; MAN, the manual; and TOOLS, the system tools.
All three take about 810K on the root file system. `custom`'s menu lists each
package's size in its Size column, in 512-byte blocks as all of `custom`'s lists
do, and at the end of its description in K; its option 6, Display current disk
usage, shows how much is free.
TOOLS puts its `ls` (with its other names `l`, `lc`, `lf`, `lr` and `lx`), `df` and
`date` in place of XENIX's own in `/bin`, and its `asktime` in place of XENIX's in
`/etc`, with the owners and permissions XENIX gives them,
and replaces the start-up files in `/usr/lib/mkuser` with ones whose C shell prompt
shows the directory and that say when the account last logged in (accounts made
before keep theirs). `xtop` is set-user-id to
sysinfo, as `ps` is, so any user can run it; it keeps where it found the
kernel's tables in `/usr/lib/xtop.syms`, installed empty, and starts faster from
its second run on. `lastlogin` is set-user-id to bin, which owns the file it keeps
the logins in, `/usr/adm/lastlog`; installing TOOLS again starts that file over.
With TOOLS, root can set the clock to the real year, as `date 10/13/2026 12:00 PM` or
XENIX's `date 101312002026`, and the time prompt at each start takes `5:36 PM` or
`10/14 9:05 AM`; `/etc/default/date` says 12- or 24-hour, month or day first.
XENIX's own programs that show dates, `tar tv` among them, show them a day ahead
past 1999, so a clock kept in 1998, whose calendar is 2026's, stays the choice that
suits all of them.
Removing TOOLS with `custom` removes `ls`, `df`, `date` and `asktime` with it.
XENIX's own `ls` and `df` are on the Operating System's volume B01; to put them back,
insert it and, as root (not yet tried):

    cd /
    tar xvf /dev/fd0 ./bin/ls ./bin/l ./bin/lc ./bin/lf ./bin/lr ./bin/lx ./bin/df

and `date` and `asktime` the same way, as `./bin/date` and `./etc/asktime`, from
whichever of its volumes has them (`tar tvf /dev/fd0` lists a volume).

(XENIX's start-up files for mkuser are on its volume B02, as
`./usr/lib/mkuser/mkuser.prof`, `mkuser.login` and `mkuser.cshrc`.)

`custom`'s option 3, listing the files in a package, shows the list a page at a time:
the screen stays blank while it is made, then the bell rings before each page.
Press Return once for each bell, and nothing else: every extra Return waits in the
input and is taken, at the menu, as an empty answer, so the menu repeats once for
each of them, slowly on an XT, and a `q` typed meanwhile waits behind them. The
interrupt key does not stop `custom` (it is caught, and returns to the menu); the
quit key, `Ctrl-\`, is not caught, and ends it.

**`$HOST`, `$USER` and `$LOGNAME` for every account (optional).** The new start-up templates set `$HOST`
to the machine's name and `$USER` and `$LOGNAME` to the login name (XENIX's
`login` sets neither), but only accounts made after installing get them; root and
older accounts keep their own files. To give every account `$HOST`, add a line to
the system's shared start-up files, as root:

    echo 'HOST=`hostname`; export HOST' >> /etc/profile
    echo 'setenv HOST "`hostname`"' >> /etc/cshrc
    echo 'USER=`logname`; LOGNAME=$USER; export USER LOGNAME' >> /etc/profile
    echo 'setenv USER "`logname`"; setenv LOGNAME "$USER"' >> /etc/cshrc

It takes effect at the next login (verified for `sh` and `csh`).

**Upgrading.** `custom` remembers which disk each file is on from the list it
installed the first time, `/etc/soft.perms`, and uses that list again rather than
the one on the new D1. A newer set whose files fall on different disks then fails
("tar: 13 file(s) not extracted") or never asks for its later disks. Before
installing a newer set over an older one, remove both names of the old list
(with only one of them gone, `custom` still installs by the old list, and quietly
leaves out every file it does not name):

    rm -f /etc/perms/soft /etc/soft.perms
    custom -d

## Using cc

`cc` takes the options of the Unix cc of its day: `-c`, `-S`, `-E`, `-o name`,
`-Dname`, `-Uname`, `-Idir`, `-lname` (`-lm` for the math library, `-ltermcap` for the
termcap library), `-s` (leave out the symbol table) and `-F size` (the stack size in
hexadecimal, as with SCO's cc). Two more come from BCC:

- `-O` runs the peephole optimizer and has the assembler make each jump as short
  as it can be: programs are smaller and faster.
- `-Mc` compiles "callersaves" code, where a function keeps no registers for its
  caller, which is smaller again. Compile all of a program's C files with it, or
  none of them: a function compiled without it expects its registers back from
  one compiled with it. The C library is compiled without it, and works with both.

`man cc` has the details, and how to debug a program with `adb`.

## Testing

`./test.sh` runs the 32-bit arithmetic helpers on an emulated 8086 against Python's exact
arithmetic, then builds the test programs and runs them under `tools/xsim.py`, a small
simulator that loads XENIX executables and services their system calls on the host.
It needs the Python packages `unicorn`, `mpmath` and `numpy` (`pip install unicorn
mpmath numpy`); the floating point tests compare every result with exact or
200-bit arithmetic. `python3 ports/bcc/test.py`
(after `./dist.sh`) checks the native toolchain against the host's BCC and checks that it
rebuilds itself. Everything is also checked on a real XENIX 2.1.3 installation in 86Box
before it counts as done.

## Running programs on XENIX

To move programs into XENIX, put them in a directory and pack it as a version 7
tar on a 360K floppy image:

    tools/mkdisk.sh disk.img somedir
    # insert disk.img in 86Box, then on XENIX:
    tar xvf /dev/fd0

## How it was made

This project is written with heavy AI assistance (Anthropic's Claude), working alongside
its author, who decides what gets built and what counts as done, and tests every change
on XENIX. The work is checked three ways:

- **On XENIX itself.** Every feature runs on XENIX 2.1.3 (in 86Box, on an emulated IBM
  XT) from test disks before it counts, and nothing is claimed here or in the release
  notes that wasn't seen working there. A milestone usually takes a dozen or more rounds
  of test disks; `xtop` alone took fifteen.
- **Against the original.** Where a tool stands in for one of XENIX's own, like `ls` and
  `df`, its output is compared with XENIX's on the same files.
- **By the test suite.** `test.sh` runs 29 groups of tests in an 8086 simulator before
  each release, and `ports/bcc/test.py` checks that the compiler running on XENIX
  rebuilds itself byte for byte.

The release disks are reproducible from the source, and ABI.md records the experiments
behind every finding about XENIX, including the ones where the first theory turned out
to be wrong.

## What this repository does not contain

No SCO, Microsoft, AT&T or IBM code, binaries or disk images. Everything here was
written for this project, apart from the BCC compiler, assembler and linker and
GNU ar (Dev86, GPL version 2), whose source is not stored here: `ports/bcc/fetch.sh`
downloads it, checks it, and applies this project's patches. To use the
Development System, you need your own XENIX 2.1.3 installation.

## License

GPL version 2 or (at your option) any later version. See [LICENSE](LICENSE).
Built together with BCC, which is GPL version 2 only, the Development System as a
whole is distributed under GPL version 2.
