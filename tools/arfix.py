"""arfix.py - make an ar archive depend only on what is in it.

    python3 tools/arfix.py lib.a ...

ar records each member's modification time, owner, group and mode, so the
same objects archived at another time, or by another user, make a different
file. This sets every member's date to 1 January 1986 (as tools/mkdisk.sh and
tools/mkset.py date their files), its owner and group to 0 and its mode to
644, so that a rebuild from the same source gives the same libc.a and libm.a,
byte for byte, and so the same Development System disks."""
import sys

DATE = 504921600                       # 1986-01-01 00:00:00 UTC

def fix(path):
    d = bytearray(open(path, 'rb').read())
    if d[:8] != b'!<arch>\n':
        raise SystemExit(f'{path}: not an ar archive')
    i = 8
    while i < len(d):
        if d[i + 58:i + 60] != b'`\n':
            raise SystemExit(f'{path}: bad member header at {i}')
        size = int(d[i + 48:i + 58])
        d[i + 16:i + 48] = b'%-12d%-6d%-6d%-8o' % (DATE, 0, 0, 0o100644)
        i += 60 + size + (size & 1)
    open(path, 'wb').write(d)

for p in sys.argv[1:]:
    fix(p)
