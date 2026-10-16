# To do

Work in order. Nothing counts until it is verified on real XENIX.

## Done: v0.10.1-alpha released on GitHub (M66b verified on XENIX).

## Done: M67 passed on XENIX (135/135 files as built; xtop's uptime
the same as uptime's with the clock set to 2030; the .cshrc template's
history 10, for new accounts; older ones change their own line).

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
5. Done on XENIX (M72 to M72d): xtfetch, xtop under that name: the PC/XT,
   the console's colors, the V20 told from the 8088 (an 8088 so far: the
   V20 is still to be seen on the Wyse). Alex: commit. Its config file
   (~/.xtfetchrc, /etc/default/xtfetch) when wanted.
6. The text processing system (to be scoped first).
7. emacs: a small MicroEMACS-style editor, for historical accuracy.

## Deferred

- Phreaxnix, after the dev system: start from Version 7 UNIX (open under
  Caldera's 2002 license; keep its notice; its advertising clause may not
  mix with the GPL). Source: TUHS, or the unix-history-repo on GitHub; the
  kernel is in usr/sys. Name idea: Phreaxnix System VI (no "Unix" in it).
- Our own passwd, enforcing the upper case, lower case and digit that
  XENIX's only suggests (it takes all lower case).
- shutdown's warning names root under sudo; the account that ran sudo
  would be better.
- `/etc/hosts`: with the SLIP/TCP/IP work.
- Root password: with the SLIP/UUCP work.
- Adding Claude as a contributor (a Co-Authored-By line in commits).
- To consider: our own Games set (not SCO's Games disk), ported from open
  source where it exists, as with BCC, keeping each one's notice: a curses
  Pong; Brickscape, a falling-blocks game from BSD's 1992 tetris (BSD license;
  its own look, as Tetris's is protected); Zork I-III (MIT since Nov 2025)
  on a Z-machine interpreter of our own (check the name's trademark); a
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
  i-node under it, slow on an XT's disk, and kept once counted; copy, move, rename, delete with a question first, and
  permissions; its keys like vi's. Name to choose.
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
- Name ideas: xtfetch (system info), xtsh (shell), XTFS (file system;
  XtreemFS already uses xtfs), XPhreax (windowing), xtify, XTSI.
