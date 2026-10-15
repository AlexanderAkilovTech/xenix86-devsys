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
3. The security and power round. M69b done (passwords: DES crypt in
   /etc/passwd). Built and tested here (31 passed): crypt and getpass in
   libc, sudo (/etc/default/sudo: USERS=, PASSWORD=root|self), shutdown
   [-h] now|+minutes. Now M70 (tests/m70), then by hand: a root password,
   alex in USERS, and `sudo shutdown -h now` from alex.
4. Later: `$USER` and `$LOGNAME` set to the login name for every account,
   as on macOS and Linux (XENIX's login sets neither; `logname` works),
   like `$HOST`.
5. xtfetch: a neofetch-style system summary.
6. The text processing system (to be scoped first).
7. emacs: a small MicroEMACS-style editor, for historical accuracy.

## Deferred

- `/etc/hosts`: with the SLIP/TCP/IP work.
- Root password: with the SLIP/UUCP work.
- Adding Claude as a contributor (a Co-Authored-By line in commits).
- Name ideas: xtfetch (system info), xtsh (shell), XTFS (file system;
  XtreemFS already uses xtfs), XPhreax (windowing), xtify, XTSI.
