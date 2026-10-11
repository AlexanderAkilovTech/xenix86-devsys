"""Convert an ld86 separate-I&D a.out (link with `ld86 -0 -i -y`, crt0.o first) into a
XENIX 2.1.3 8086 x.out. Data and bss keep the addresses ld86 assigned; the data
segment is zero-padded to a paragraph and the bss shrunk to match, so the in-memory
image is identical while sizes stay on 16-byte boundaries. ld86's symbol table, if
it wrote one, becomes an x.out one (cmd/aout2x.c does the same on XENIX)."""
import struct, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from xout import xout, sym, segsyms, TEXTSEG, DATASEG, S_ABS, S_TEXT, S_DATA, S_BSS, S_EXTERN

N_SECT, C_EXT, C_CLASS = 0o7, 0o20, 0o370

def symbols(raw):
    """ld86's nlist records (8-byte name, value, class, aux, type) as x.out
    symbols. With ld86 -y, the rest of a name longer than 8 characters
    follows in records of class 0."""
    out, cur = bytearray(), None
    def flush():
        if cur:
            stype, seg, value, name = cur
            out.extend(sym(stype, seg, value, name))
    for i in range(0, len(raw) - 15, 16):
        name = raw[i:i + 8].split(b'\0')[0]
        value, sclass = struct.unpack_from('<lB', raw, i + 8)
        if sclass == 0 and cur:                 # a continuation of the name
            cur[3] += name
            continue
        flush()
        sect = sclass & N_SECT
        stype, seg = {1: (S_ABS, 0), 2: (S_TEXT, TEXTSEG), 3: (S_DATA, DATASEG),
                      4: (S_BSS, DATASEG)}.get(sect, (0, 0))
        if sclass & C_CLASS == C_EXT:
            stype |= S_EXTERN
        cur = [stype, seg, value, name]
    flush()
    return segsyms() + bytes(out) if out else b''

def convert(src, dst, stack=4096, fph=False, strip=False):
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
    nsyms = struct.unpack_from('<l', d, 28)[0]
    syms = b'' if strip else symbols(d[hlen + text + data:hlen + text + data + nsyms])
    total = data + bss
    padded = (data + 15) & ~15
    bss_out = max(0, ((total + 15) & ~15) - padded)
    open(dst, 'wb').write(xout(t, dd + bytes(padded - data), bss_out, stack, fph, syms))
    os.chmod(dst, 0o755)
    print(f'{dst}: text {len(t)}, data {padded}, bss {bss_out}, stack {stack}'
          + (f', symbols {len(syms)}' if syms else ''))

if __name__ == '__main__':
    # usage: aout2xout.py [--fph] [--strip] in.aout out [stack-bytes]
    # --fph marks the program as needing floating point hardware (an 8087);
    # --strip leaves out the symbol table
    a = sys.argv[1:]
    fph, strip = '--fph' in a, '--strip' in a
    a = [x for x in a if x not in ('--fph', '--strip')]
    convert(a[0], a[1], int(a[2], 0) if len(a) > 2 else 4096, fph, strip)
