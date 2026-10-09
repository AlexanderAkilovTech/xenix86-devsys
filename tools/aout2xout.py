"""Convert an ld86 separate-I&D a.out (link with `ld86 -0 -i`, crt0.o first) into a
XENIX 2.1.3 8086 x.out. Data and bss keep the addresses ld86 assigned; the data
segment is zero-padded to a paragraph and the bss shrunk to match, so the in-memory
image is identical while sizes stay on 16-byte boundaries."""
import struct, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from xout import xout

def convert(src, dst, stack=4096):
    d = open(src, 'rb').read()
    if d[0:2] != b'\x01\x03':
        raise SystemExit(f'{src}: not an ld86 a.out')
    flags, cpu, hlen = d[2], d[3], d[4]
    if not flags & 0x20:
        raise SystemExit(f'{src}: not separate I&D (link with ld86 -i)')
    text, data, bss = struct.unpack_from('<lll', d, 8)
    t = d[hlen:hlen + text]
    dd = d[hlen + text:hlen + text + data]
    if t[0:2] != b'\xEB\x7E' or dd[0:4] != b'\x00\x00\xff\xff':
        raise SystemExit(f'{src}: crt0.o must be linked first')
    total = data + bss
    padded = (data + 15) & ~15
    bss_out = max(0, ((total + 15) & ~15) - padded)
    open(dst, 'wb').write(xout(t, dd + bytes(padded - data), bss_out, stack))
    os.chmod(dst, 0o755)
    print(f'{dst}: text {len(t)}, data {padded}, bss {bss_out}, stack {stack}')

if __name__ == '__main__':
    convert(sys.argv[1], sys.argv[2])
