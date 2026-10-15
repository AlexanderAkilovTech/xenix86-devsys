# To do

Work in order. Nothing counts until it is verified on real XENIX.

## Done: M66b passed on XENIX (135/135 files as built; hostname, date,
asktime, uptime and $HOST all working).

## Now: release v0.10.1-alpha

- Done: ABI.md, README.md, release notes, `build/release-v0.10.1` (the M66
  disks, SHA256SUMS) and the commit message (COMMIT-MSG.txt there).
- Alex: commit, push, tag `v0.10.1-alpha`, and publish the release with the
  files in `build/release-v0.10.1`, marked as a pre-release.

## Next round

- xtop: uptime shows "10229 days" after the clock is set. Use ticks
  from `times()`, as `uptime` does, not now minus boot time.
- xtop.1 HISTORY: credit htop, no year: "Its screen follows the
  general look of htop, by Hisham Muhammad." Then name what is
  xtop's own (FPU detection, disk usage, ...; check the page for
  the full list).
- History and uptime follow-ups, then emacs (a small MicroEMACS-style
  editor, for historical accuracy).

## Deferred

- `/etc/hosts`: with the SLIP/TCP/IP work.
- Root password: with the SLIP/UUCP work.
- Adding Claude as a contributor (a Co-Authored-By line in commits).
- Name ideas: xtfetch (system info), xtsh (shell), XTFS (file system;
  XtreemFS already uses xtfs), XPhreax (windowing), xtify, XTSI.
