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
- stime (25) takes the new time itself, BX = low word and CX = high, as time
  returns it, not a pointer to it; the super-user's only (M62, tests/m62/settime.c,
  which moved the clock an hour and back, and to 2026 and back).
- times (43) returns the clock ticks since the machine started, which go on
  whatever the clock is set to: a program can put the clock back from them
  (tests/m63/settime.c, save and back).
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
- XENIX 2.1.3's date command takes a two-digit year (`date mmddhhmm[yy]`), so a
  clock set for 2026 is set to 1998, which has the same calendar. Files carried
  in from a modern machine then look decades newer than anything built on
  XENIX, and make rebuilds everything every time; tools/mkdisk.sh dates the
  files on its floppy images 1 January 1986.
- The kernel and the files are fine past 1999: with the clock set to October
  2026 by stime, a new file's time was 2026's, this project's `ls -l` showed it
  right and `find -mtime -1` found it (M62).
- XENIX's own date, and its tar, take 2000 for a common year (M62, M63). The
  weekday stays right, but from February 29, 2000 on the month and day are a
  day ahead, and from 2001 on the day of the year too: February 29 shows as
  03/01, March 1 as 03/02, December 31, 2000 as 01/01 of 2001 (%j 001),
  October 13, 2026 as October 14 (%j 287, not 286). %y is (year - 1900) / 10
  as a character counted from '0', then the units: `:0` for 2000, `<6` for
  2026, `=7` for 2037. This project's date (TOOLS) replaces it.
- XENIX's date (M63, against this project's on the same arguments): the
  formats %n %t %m %d %y %D %H %M %S %T %j %w %a %h %r and %%; any other
  letter is "date: bad format character - c", status 2, and so is a % at the
  end (the character printed is a NUL). %r is `03:38:01 PM`. `+` alone prints
  an empty line. `-u` says "date: no TOY clock", status 2 (on the XT).
  Setting: `date mmddhhmm[yy]` sets the clock at second 0 and prints the new
  time; "date: bad conversion", status 2, for 6 digits, letters, month 00 or
  13, hour 25, minute 60; it took February 30 (to March 2) and `1013120098x`
  and 11 digits, ignoring the end; arguments after the first are ignored. A
  user other than root gets "date: no permission", then the time it did not
  set printed as if it had, status 2.
- Setting the clock, XENIX's date writes two utmp entries, type 3 (OLD_TIME)
  with line `old time` and the time before, and type 4 (NEW_TIME) with `new
  time` and the time after, pid 0, id and user empty: in /etc/utmp, at its end
  the first time and in place after that, and at the end of /etc/wtmp (M63).
- With TZ set but empty, XENIX's date printed PDT: its default zone is
  PST8PDT (/etc/rc sets TZ=PST8PDT as well). This project's library has the
  same default.
- /bin/date is bin's, mode 711, 11864 bytes, dated Sep 12 1985. There are also
  /etc/asktime (bin, 700, 1818 bytes), which with an empty answer printed
  `Current System Time is ...` and `Enter new time ([yymmdd]hhmm): ` and kept
  the time, and /etc/setclock (bin, 700, 9086 bytes).
- /etc/rc runs `eval /etc/asktime $autoflag $autoboot` at every start. asktime
  is a shell script: the clock starts from the date in the root file system's
  super-block (so it stands still while the machine is off), it shows `date`'s
  output and asks for `[yymmdd]hhmm`, turns the answer into date's form with
  /lib/cvtdate and sets it with `date`; with a real-time clock it uses
  /etc/setclock. With this project's date in /bin, the boot prompt shows the
  date right past 1999 (M64). Whether cvtdate passes a year past 1999 on is
  not yet tried.

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

- `custom`'s package menu prints each `#!` line's description from column 32
  (the name, Inst and Size columns at tab stops), so a description longer than
  47 characters runs into the last column and wraps (M51).
- `custom` takes the root file system's free space from plain `df
  /dev/root`, the number before "blocks", so that form of `df` must stay as
  XENIX prints it. Its option 6, Display current disk usage, only prints
  `df -v` between two rules, so TOOLS' `df -v`, in K and M, shows there
  (M54).
- A package may replace the Operating System's own files (M51, the TOOLS
  package): `tar` wrote `/bin/ls` over XENIX's, which kept one inode with its
  six names (`ls`, `l`, `lc`, `lf`, `lr`, `lx`) and mode x1711, sticky; `df` and
  `xtop` (with `top`) came out set-user-id to sysinfo, as listed; every file was
  byte for byte the set's. `mkuser` then gave a new C shell account the
  replaced `mkuser.cshrc` and `mkuser.login`, and that user ran `xtop`, which
  read `/dev/kmem`, `/dev/mem`, `/dev/swap` and the root disk as sysinfo.
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
- XENIX's `/bin/csh` is an early Berkeley one, from before job control: its
  only version string is `@(#)sh.c 2.1`, and its C library's printf is dated
  6/5/79. It has no `jobs`, `fg`, `bg`, `limit`, `dirs`, `pushd` or `popd`, and
  keeps no `$cwd`. `if ("abc" =~ a*)` is "Expression syntax": there is no `=~`
  or `!~`, though `switch` takes patterns. The `$var` modifiers are `:h`, `:t`,
  `:r`, `:q` and `:x`. A `!` followed by anything but a blank is taken for
  history even in a file read with `source` ("Event not found"). Its strings
  name `/etc/cshrc` besides `~/.cshrc`, `~/.login` and `~/.logout`, so it
  probably reads a system-wide file first (not yet tested).
- The csh templates (`skel/`) keep `$cwd` themselves: `cd` is an alias that
  sources `/usr/lib/cdprompt.csh`, which changes directory and works out the
  new path from the old one, running `pwd` only for names it cannot (those
  with a `.` or a `/` inside them).
  Tested on XENIX: absolute and relative names, `..`, `../tmp`, a bare `cd`,
  `pushd` and `popd`, and a directory that does not exist (the prompt stays).
- `uname -n` prints `(empty)` when no node name has been set, and uname has no
  `-S` to set one (its usage: `[-snrvmdupal]`). The machine's name, for Micnet
  and UUCP, is the first line of /etc/systemid (`xenix86` here, bin's, 644).
- csh has no `unsetenv` ("unsetenv: Command not found."). A `switch` takes
  variables in its `case` labels (`case "$home"/*:`). `$cwd:` followed by a
  space is "Variable syntax": a colon after a variable starts a modifier, so
  write `${cwd}:` (M61, M62).
- csh keeps a history: after `set history = 5`, `history` lists the commands
  typed, numbered (M64, `csh -f -i` reading them).
- A script whose first character is `#` is run by csh, not sh, so an sh script
  starts with `:` instead (tests/tools/t).
- XENIX's grep has no `\|`: `grep "a\|b"` matched nothing. On a program,
  `grep -c` counted 0 even for a message the program prints (M63).
- `echo "--- XENIX"` printed an empty line: an argument that begins with `-` is
  lost. Test scripts mark their sections with other characters (`+++`, `===`).
- Starting a process costs a second or two on the emulated XT, swapping
  included; a test script that ran about 900 (two `ls`, `cmp` and `expr` per
  case) took over half an hour. Scripts now run only what they measure and
  compare the results on the host.

## The kernel (read through /dev/kmem with tests/kprobe.c)
- The clock ticks 20 times a second (HZ 20); process times are in ticks.
- The process table `_proc` has 48-byte slots: stat at 0, flag 1 (SLOAD 01: in
  memory), pri 2, time 3, cpu 4, nice 5, uid 10, pid 18, ppid 20, addr 22, size 44
  (in 16-byte clicks), wchan 34. `v.ve_proc` (at 26 in `_v`) points just past
  the last slot in use, not at the end of the table.
- addr is a click address for a process in memory (`ps -l` ADDR 8700) and a swap
  block for one swapped out (b9); `ps -l` SZ is size in K.
- The swap map is in blocks of 1024 bytes (1599 blocks on this system); the core
  map in clicks. Both are System V maps: `{nwaiting, ncells, entries}`, each
  entry `{size, address}`.
- The u area of a process in memory is at its addr: the command name at 1388, the
  user and system times (longs, in ticks) at 1296 and 1300, the pointer back to
  the proc slot at 1070; `ps`'s TIME is (utime + stime) / 20, rounded.
- A swapped-out process's image is on /dev/swap at (addr + swplo) * 1024 bytes,
  with `_swplo` -1: a System V map cannot hold address 0, so its addresses
  start at 1. The image is laid out as in memory, the u area at the same
  offsets (verified with five processes forced out by memory hogs, M38).
- /dev/mem reads stop at 640K, the memory installed; `_maxmem` (25536 clicks,
  399K) is what is left for processes.
- The u area's controlling terminal: `u_ttyp` at 1336 (non-zero when there is
  one) and `u_ttyd`, its device number, at 1338 (M48: getty on tty02 has 0001,
  matching `ps`).
- Terminal device numbers (M48, `/dev`): the console 0000 and the screens
  tty02 to tty10 0001 to 0009 (major 0); `/dev/tty` 0300; the serial lines major
  5, tty1a (also tty11) 0500 and tty2a (tty12) 0508, the modem-control names
  128 higher (tty1A 0580). Some names share a device: tty3a is tty1e's 0504.
- The kernel keeps no `sysinfo`: M49 listed all 1164 symbols in `/xenix`, and
  there are no counters of idle time, disk blocks or swaps. Among the data
  symbols are `_Got8087`, `_cpu`, `_cputype`, `_Idlef`, `_Fpcnt`, `_Fpecnt`,
  `_lticks`, `_seconds`, the floppy driver's `_fltrnscnt` and `_fl_stats`, and
  the hard disk drivers' tables `_xetab` (Xebec) and `_omtab` (OMTI); what each
  holds is not yet measured. So the only CPU time to show is the processes'
  own (xtop).
- The running process's u area is the kernel's `_u` (data, at 36da, about
  1.5K before the next symbol), copied back to the process's memory only
  when another process runs. A program reading its own u area through
  `/dev/mem` sees its times as of then, seconds old on a quiet system: xtop
  showed itself at 0.0% just after redrawing the whole screen (M52). Read
  through `/dev/kmem` at `_u`, they are current: xtop's last screen showed
  9.30 s for itself where `tm` measured 9.50 s in all, the rest being that
  screen's own drawing; read the old way, 8.70 s against 9.15 s, and 0.0% on
  its first screen (M54).

## The console and the CGA (measured with tests/cgatest.c, 86Box CGA)
- The console takes ANSI colors: `ESC[3nm` foreground and `ESC[4nm` background,
  n in the ANSI order (1 red, 2 green, 3 brown, 4 blue, 5 magenta, 6 cyan, 7
  white); `ESC[1m` makes the foreground bright, so 16 foregrounds and 8
  backgrounds. A sequence is acted on for its first number only: `ESC[1;31m` is
  `ESC[1m`. `ESC[0m` ends bold, blink and reverse but leaves the colors, so a
  program puts back `ESC[37m` and `ESC[40m` itself. `ESC[5m` blinks, `ESC[7m`
  reverses, `ESC[4m` shows nothing on a color screen, and `ESC[2m` makes the
  text invisible. SCO's later `ESC[=nF` and `ESC[=nG` are read as `ESC[nF` and
  `ESC[nG` and move the cursor.
- ioctl GIO_COLOR (`'c' << 8`) returns 1 on the color console; GIO_ATTR (`'a' <<
  8`) returns the attribute in use, 7.
- The kernel has no graphics modes, but a program can drive the CGA itself: the
  6845's registers through ports 3D4/3D5, the mode at 3D8, the palette at 3D9
  (an 8086 has no I/O protection), and the picture copied straight to B800:0 with
  ES loaded (tests/cgaio.s). Mode 4 (320 x 200, four colors) and both palettes
  worked, and the BIOS's mode 3 values put the console back as it was.
- The cursor's shape is the 6845's registers 10 and 11 (its first and last scan
  lines); a program can make it a block (0 to 7 on the CGA, 0 to 12 on a
  monochrome card) or the usual line (6 and 7, 11 and 12) through 3D4/3D5 or
  3B4/3B5 (cmd/cursor.c, M60). It is one shape for all the console's screens,
  and back to the line after the machine is started again. The kernel has no
  escape sequence for it.
- The console's stty already has `echoe`, with `erase` ^H (M60).
- `/dev/mem` ends with the memory XENIX has: reads and writes at B8000 fail, and
  the BIOS's data at 400 is overwritten by the kernel's own.
- Display adapters in 86Box: XENIX 2.1.3 runs with a CGA, an MDA and a Hercules
  card. With an EGA the machine starts and the boot program loads
  `hd(40,0)xenix`, but the kernel goes no further.
- GIO_COLOR returns 1 with the CGA and 0 with the MDA and the Hercules card
  (xtop, M46). The two monochrome cards are told apart by their status port,
  3BA: a Hercules card's bit 7 follows the vertical retrace, an MDA's does not
  change (cmd/hwprobe.s, reading only).
- XENIX's `/etc/termcap` entry `ansi` (line 21) has `so=\E[7m:se=\E[m` and
  `us=\E[4m:ue=\E[m`, and the console's line-drawing set: `GS=\E[12m` and
  `GE=\E[10m` around the characters `GV=3`, `GH=D` and corners `G1`-`G4`
  (`?`, `Z`, `@`, `Y`): by their codes the IBM PC's box characters B3, C4, BF,
  DA, C0 and D9 less 80 (not yet tried).
- The IBM PC's font draws `|` (7C) as a broken bar on the CGA, the MDA and the
  Hercules card alike; the CGA's 8 x 8 font is coarser than the monochrome
  cards' 9 x 14, which is most of why the same screen reads more easily on them.

## Speed on the XT (86Box, measured with tests/tm.c, M42 to M45)
- XENIX 2.1.3 has no `time` command (csh's builtin aside); tests/tm.c runs a
  command and reports its processor time from `times()` (system call 43, four
  longs in ticks of 1/20 s: user, system, children's user, children's system).
- `ps -el` takes 18 s of processor time, three quarters of it in the kernel:
  it reads the kernel's symbols from `/xenix` one read at a time.
- On an 8088 a multiplication (`imul`) takes about 130 clock cycles, and BCC
  compiles every subscript into a two-dimensional array as one; the string
  instructions (`rep movsb`, `repne scasb`) are the fastest way to move or
  search bytes. xtop's drawing loop got cheaper by a third when it walked its
  screen with pointers instead of `scr[r][c]`.
- Writing to the console is the kernel's work, about as costly per character
  as making the character in C; programs that redraw should send only what
  changed.
- xtop (then top, M45): about 4 s to start on the console, then about 0.37 s for each
  screen at 3 s apart (0.28 s in itself, 0.08 s in the kernel): 12%.
- xtop with its TTY column (M54): about 7.5 s of processor to start, up from
  under 3 s in M47, most of it in the kernel; the new work at the start is
  looking up the terminals' names in `/dev` (63 of them, each stat'ed). Then
  0.30 s for each screen at 2 s apart, 15%.
- xtop looking up only the terminals in use (M55): 6 s on the clock from the
  command to the first screen, timed by hand and by `tm`; 4.65 s of processor
  for one screen to a file, against M54's 9.7 s.
- xtop's start step by step (M56, tests/m56, a build with `-DPHASES` writing
  its times at each step; on the console, in ticks of 1/20 s): loading the 30K
  program, 17 ticks in the kernel before `main` runs; finding the kernel's
  symbols in `/xenix`, 36 (18 of them xtop's own scanning of the 19K symbol
  table, read 1K at a time); the devices, file systems, memory size and `/dev`,
  15; termcap and clearing the screen, 6; the two looks at the processes, 7;
  the first screen, 37 (24 in xtop's own code, half making the lines and half
  drawing them, and the 4 stats of the terminals in use). About 6 s in all.
- Reading `/dev`, about 190 entries, 16 bytes at a time cost about a second
  of kernel time: read 1K at a time, xtop's start went from 3.10 to 3.45 s in
  the kernel to 2.40 s (M56).
- `times()` (system call 43) returns the clock as well: the ticks since the
  machine started, the same as `_lbolt` at every step (M56).
- xtop keeping the kernel's symbols in `/usr/lib/xtop.syms` (M57): the file
  installs empty, `-rw------- sysinfo bin`, and the first run fills it (82
  bytes). Timed by `tm` from the command to the first screen: 6 s that first
  time, then 4 s. Step by step with the symbols kept, finding them takes 4
  ticks instead of 35, and the whole start about 3.75 s: loading the program
  12 ticks, the devices and `/dev` 10, termcap and clearing 5, the two looks
  7, the first screen 36 (23 of them in xtop's own code, and the 4 stats).
  With nothing kept, the symbol table is read in 5 pieces of 4K. An ordinary
  user's xtop, set-user-id to sysinfo, starts with the kept file (by hand).
- xtop keeping the terminals' names (while `/dev`'s time is the same) and the
  size of memory (while `_bootime` is) as well, its process lines put
  together without `sprintf`, and rows still blank from clearing the screen
  written whole (M58): from its first line to the first screen 47 ticks, with
  13 of loading before it, 3.0 s, and 3 s by hand. Processor for a start:
  0.70 to 0.75 s in its own code (M57: 1.45 s) and 1.80 to 1.95 s in the
  kernel; the first screen 21 ticks (M57: 36). In the simulator, xtop's own
  instructions for such a start went from 339,000 to 193,000.
- The sticky bit (`chmod 1755`, shown `-rwxr-xr-t`) on a copy of xtop made no
  difference to its processor time: 0.65 to 0.75 s in its own code and 1.70
  to 2.05 s in the kernel, against 0.70 to 0.75 and 1.80 to 1.95 without. The
  time on the clock was not measured: M58's `tm` printed three numbers from
  two buffers, so its "in all" repeated the first (fixed for M59).
- The release's disks installed (M59): from the command to the first screen,
  5.25 s the first time (xtop fills `/usr/lib/xtop.syms`, 190 bytes), then
  3.15 s and 3.05 s, timed by `tm` to 1/20 s. Eleven screens 1 s apart took
  6.15 s of processor in all, so each screen after the first about 0.31 s:
  a tenth of the machine at the usual 3 s between screens.
- `/etc/termcap` is 36,790 bytes, 804 lines; `ansi` is the entry at line 21,
  so finding it costs little either way.

## Getting files off XENIX
- `tar cvf /dev/fd0 file` once printed `37777777000  2113 36446` and wrote
  nothing. `cp file /dev/fd0` writes the file's bytes from the start of the
  floppy; read back from the image by the file's length, or up to an end marker
  the script wrote last.

## Logins (utmp and wtmp)
- /etc/utmp and /etc/wtmp hold System V entries of 36 bytes: user (8) at 0, id
  (4) at 8, line (12) at 12, pid (short) at 24, type (short) at 26, exit status
  (two shorts) at 28, time (long) at 32. Types seen: 3 OLD_TIME and 4 NEW_TIME
  (date), 5 INIT_PROCESS, 6 LOGIN_PROCESS (`LOGIN` on a waiting terminal), 7
  USER_PROCESS (the user, with the pid of the login shell). The console's line
  is `console`, id `cons`; the others are `tty02` and so on, id `02` (M60, M63).
- /etc/wtmp held only the entries since the machine last started, and there is
  no /usr/adm/wtmp, so the time of the login before this one is not kept
  anywhere: cmd/lastlogin.c keeps it, in /usr/adm/lastlog (M60, M61).

## Commands present
- Not there: `last`, `hostname`, `lastlogin`, `tput`. There: `finger`,
  `setcolor` (M60).
- XENIX 2.1.3 has `/usr/bin/more` (with `--More--(15%)`) but no `pg`, and no
  `nroff`, so the manual is formatted ahead of time (tools/man.py) and `man`
  pages it through `more`.
- `mkdir` takes no options: `mkdir -p test` made two directories, `test` and
  one named `-p` (removed with `rmdir ./-p`).
- `crontab` is there (`/usr/bin/crontab`), with System V's cron.
