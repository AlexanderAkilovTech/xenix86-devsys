"""objclass.py - set the module class of an as86 object file.

    python3 tools/objclass.py file.o class

ld86 gives a symbol defined in two modules of different classes to the lower
class, unless the higher-class module is linked anyway, for some other symbol;
then that one's definition wins. libc uses this like a weak symbol: printf's
floating point conversions, and scanf's, are stubs (class 0) unless the
modules with the real ones (class 1) are pulled in, which happens when the
compiler marks a program as needing them (see lib/stdio/fltcvt.c). as86
always writes class 0; the class is byte 15 of the file (one module)."""
import sys

path, cls = sys.argv[1], int(sys.argv[2])
d = bytearray(open(path, 'rb').read())
if d[0:2] != b'\xa3\x86' or d[2:4] != b'\x01\x00':
    raise SystemExit(f'{path}: not a one-module as86 object')
d[15] = cls
open(path, 'wb').write(d)
