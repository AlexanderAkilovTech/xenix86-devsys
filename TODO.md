# To do

Work in order. Nothing counts until it is verified on real XENIX.

## Done: v0.10.1-alpha released on GitHub (M66b verified on XENIX).

## Done: M67 passed on XENIX

135/135 files as built; xtop's uptime the same as uptime's with the
clock set to 2030; the .cshrc template's history 10, for new accounts;
older ones change their own line.

## Next, in order

1. Done: M67 committed and pushed.
2. Done on XENIX (M68, M68b): the prompt `alex@xenix86 ~ %`; asktime
   makes the block cursor the default at every boot. Alex: commit.
3. Done on XENIX (M69b, M70): crypt matches XENIX's hash; sudo as root,
   alex refused until listed, logged; `sudo shutdown -h now` from alex,
   with root's password, warned and halted. Alex: commit.
4. Done on XENIX (M71): `$USER` and `$LOGNAME` in the templates and, by
   hand, /etc/profile and /etc/cshrc. The M71 set is v0.11.0-alpha:
   build/release-v0.11.0. Alex: commit, push, release (pre-release).
5. Done on XENIX (M72 to M72f): xtfetch, xtop under that name: the PC/XT,
   the console's colors, the V20 told from the 8088 by AAD (M72f: NEC V20
   with an 8087, and Intel 8088 without, in 86Box; the Wyse to come).
   Committed. Its config file
   (~/.xtfetchrc, /etc/default/xtfetch) when wanted.
5b. Found by Alex on XENIX: `kill` (signal 15) from another console left
   xtop's last frame on the screen and the tty raw, since xtop caught
   SIGINT, SIGQUIT and SIGHUP but not SIGTERM. Fixed: SIGTERM ends it as q
   does. The simulator can now send a signal (XSIM_KILL), and the test
   fails without the fix. Done on XENIX (M73: 144/144 files as built;
   killed from a second console, a clean prompt). Committed.
6. The text processing system (to be scoped first). Found by Alex: XENIX's
   custom has a slot for it, `custom -t`, as `-d` is the Development
   System's. First learn what -t expects, as was done for -d: the name of
   its permissions list (as /etc/perms/soft is -d's), how it names and
   numbers its disks (it asks for "Text Processing System volume T1", as
   -d asks for D1: so T01, T02 ...), and its prompts; observed only,
   nothing of SCO's copied. Then it installs with `custom -t`, as SCO's
   own would have.
7. emacs: a small MicroEMACS-style editor, for historical accuracy.

## Deferred

- Phreaxnix, after the dev system: start from Version 7 UNIX (open under
  Caldera's 2002 license; keep its notice; its advertising clause may not
  mix with the GPL). Source: TUHS, or the unix-history-repo on GitHub; the
  kernel is in usr/sys. Name idea: Phreaxnix System VI (no "Unix" in it).
- Our own passwd (first, look at how XENIX's handles passwords: its rules,
  messages, /etc/passwd's fields, aging), enforcing what XENIX's only
  suggests (it takes all lower case): at least 10 characters, with upper
  case, lower case, a digit and a symbol; refused otherwise, no way past
  it. But XENIX's crypt, which login uses, reads only the first 8: the
  9th and 10th add nothing, so the rule must hold within the first 8 too,
  or login replaced along with the hash (a bigger job; to be decided).
- shutdown's warning names root under sudo; the account that ran sudo
  would be better.
- `/etc/hosts`: with the SLIP/TCP/IP work.
- Root password: with the SLIP/UUCP work.
- Adding Claude as a contributor (a Co-Authored-By line in commits).
- To consider: our own Games set (not SCO's Games disk), ported from open
  source where it exists, as with BCC, keeping each one's notice: a
  curses Pong; Brickscape, a falling-blocks game from BSD's 1992 tetris
  (BSD license; its own look, as Tetris's is protected); Zork I-III (MIT
  since Nov 2025) on a Z-machine interpreter of our own (check the name's
  trademark); a
  chess game of our own (small engines' licenses do not mix with the GPL).
  Each a finished game, not a demo: Pong with a computer player and levels;
  Brickscape with the next piece, levels and a high-score table; the
  interpreter passing the standard Z-machine test files (czech, praxix);
  chess with every rule (castling, en passant, promotion, check, mate,
  stalemate, draws), checked by perft counts, with levels of strength.
- To consider: a terminal file manager after yazi (by sxyazi): curses,
  three columns (the parent, this directory, a preview of the file under
  the cursor: a text file's first lines, a directory's names, a program's
  size and type); each file's size beside its name; a directory's size
  (all it holds, as du counts) on a key, since adding it up reads every
  i-node under it, slow on an XT's disk, and kept once counted; copy,
  move, rename, delete with a question first, and permissions; its keys
  like vi's. Name to choose.
- Man pages of our own for XENIX's own commands (BSD style, HISTORY,
  no year after 1986), written from what each command does on XENIX, as
  observed; never from SCO's manuals' words.
- A Whetstone benchmark (Curnow and Wichmann's, 1976; public domain),
  with the 8087 and with our software floating point, printing KWIPS beside
  published figures for a stock PC/XT, a PC/AT, and minicomputers such as
  the PDP-11: each figure with its source, none made up. Perhaps Dhrystone
  (1984) beside it, for whole numbers.
- Languages beyond C and assembly (README's roadmap names FORTRAN, COBOL,
  BASIC, maybe Pascal): FORTRAN (the 8087), COBOL, BASIC, Pascal, ALGOL,
  BCPL, B and LISP. Each scoped first (which standard or dialect, compiler
  or interpreter), each finished and tested like the rest. B and BCPL are
  small and C's own ancestors; BASIC and LISP as interpreters, as they
  were then. BASIC: GW-BASIC, from Microsoft's 8086 source (MIT since
  2020, keeping its notice) ported to XENIX if that source proves usable,
  or else a GW-BASIC-compatible interpreter of our own; the programs of
  the '80s magazines and books to run on it.
- Found by Alex: custom's menu has a fourth choice, "Add a Supported
  Product", which asks for "distribution volume 1": SCO's way in for other
  products. Learn (observed only) what it reads from that first disk, its
  permissions list and how it names the product; then our sets beyond the
  Development and Text Processing Systems (the Games set, the languages,
  the file manager) could each install through it, as their own products.
  Alex's thought: most of our tools are not development tools (ls, df,
  date, xtop, xtfetch, sudo, shutdown, hostname, uptime, cursor, asktime,
  the prompt files) and belong in a product of their own, a System Tools
  set, leaving the Development System the compiler, assembler, linker,
  make, libraries, headers and their pages. Decide once option 4's format
  is known (a probe round: an empty disk, our D01, a one-file product).
- Floating point on the 8087, safely: XENIX 2.1.3 does not save its
  registers between processes (two at once corrupt each other, measured),
  so a program built for it (cc -f87, say) first takes a lock on it; if
  another holds it, that run uses the software floating point instead.
  One program on the 8087 at a time, at full speed. The real fix, the
  8087 saved at every switch, is the kernel's: Phreaxnix.
- ARCnet (the Wyse's Thomas-Conrad TC6042, 8-bit, RG-62 coax): long way
  round. Reference: Linux's drivers/net/arcnet (com90xx, for COM9026-based
  ISA cards; GPL v2). Against it: 86Box has no ARCnet, so no testing
  before the real Wyse, and a second ARCnet machine or a bridge is
  needed. Networking
  goes by SLIP over the DB-25 serial port first.
- Config files (Alex's idea): settings in files rather than flags. Either
  a ~/.config/ directory with one per program (xtop, xtfetch), as modern
  systems do, or dotfiles in the home directory (~/.xtoprc, ~/.xtfetchrc),
  as the '80s did: to decide. The cursor's own, ~/.cursorrc, read by
  cursor and asktime at login: `shape = block` or `shape = line` (the
  underscore), and `blink = 1` or `blink = 0` (if the console's
  hardware can stop it blinking: to check). Simple `name = value` lines,
  0 or 1 for on and off; one shape setting, not one switch per shape, so
  two can never both be on. Names within XENIX's 14 characters.
  /etc/default/ for the system-wide default. xtop already keeps its theme
  in ~/.xtoprc (the c key): decide once, for xtop, xtfetch and the cursor
  alike, and move xtop's file if ~/.config/ wins.
  First, the installer (found M73): each custom -d puts its own copies of
  /etc/default/sudo and /etc/default/date in place, and empty sudolog,
  lastlog and xtop.syms, so a reinstall wipes who may use sudo, the date
  format, and the logs. Leave a file that is already there alone, as
  installers do with settings, before adding more config files.
- Name ideas: xtfetch (system info), xtsh (shell), XTFS (file system;
  XtreemFS already uses xtfs), XPhreax (windowing), xtify, XTSI.
