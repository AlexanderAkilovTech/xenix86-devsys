"""Direct writer for Xenix 2.x 8086 x.out executables (small model, separate I&D),
laid out like SCO's own binaries (e.g. /bin/tail): 32-byte header, 44-byte
extension, segment table at 96, text then data, then the symbol table if there
is one, as SCO's ld writes it: a third segment table entry of type XS_TSYMS
(3) in the x.out segmented format (attribute 1), whose records are struct sym
(type, segment number, value) each followed by the name and a 0."""
import struct

TEXTSEG, DATASEG = 63, 71       # the segment numbers XENIX's small model uses
S_ABS, S_TEXT, S_DATA, S_BSS, S_SEG, S_EXTERN = 1, 2, 3, 4, 8, 0x20
SYMLENGTH = 50                  # the longest name XENIX's tools take

def sym(stype, seg, value, name):
    """one symbol table record"""
    return struct.pack('<HHl', stype, seg, value) + name[:SYMLENGTH] + b'\0'

def segsyms():
    """the segment name symbols SCO's ld puts first"""
    return sym(S_SEG | S_EXTERN, TEXTSEG, 0, b'_TEXT') + sym(S_SEG | S_EXTERN, DATASEG, 0, b'_DATA')

def xout(text, data=b'', bss=0, stack=1024, fph=False, syms=b''):
    data = bytes(data) + bytes(-len(data) % 16)   # data padded to a paragraph
    bss = (bss + 15) & ~15                        # bss in whole paragraphs
    text = bytes(text) + bytes(len(text) % 2)     # even-length text
    hdr_len, segtab = 96, 96 if syms else 64
    tpos = hdr_len + segtab
    dpos = tpos + len(text)
    renv = 0xc80f                      # OS-version, segmented, fixed stack, pure, sep I&D, exec
    if fph:
        renv |= 0x0080                 # needs floating point hardware (an 8087)
    h = struct.pack('<HHlllllLBBH', 0x0206, 44, len(text), len(data), bss, len(syms), 0, 0, 0x44, 0x64, renv)
    e = struct.pack('<9lBBBBHH', 0, 0, 0, 0, stack, hdr_len, segtab, 0, 0, 0, 0, 1, 2, TEXTSEG, 0)
    out = bytearray(h + e)
    out += bytes(hdr_len - len(out))
    out += struct.pack('<HHHBBllllHHl', 1, 0x8008, TEXTSEG, 0, 0, tpos, len(text), len(text), 0, 0, 0, 0)
    out += struct.pack('<HHHBBllllHHl', 2, 0x8000 | (4 if bss else 0), DATASEG, 0, 0, dpos, len(data), len(data) + bss, 0, 0, 0, 0)
    if syms:
        out += struct.pack('<HHHBBllllHHl', 3, 1, 0, 0, 0, dpos + len(data), len(syms), 0, 0, 0, 0, 0)
    out += text + data + bytes(syms)
    return bytes(out)
