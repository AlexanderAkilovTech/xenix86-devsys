"""Direct writer for Xenix 2.x 8086 x.out executables (small model, separate I&D),
laid out like SCO's own binaries (e.g. /bin/tail): 32-byte header, 44-byte
extension, segment table at 96, text then data."""
import struct

def xout(text, data=b'', bss=0, stack=1024):
    data = bytes(data) + bytes(-len(data) % 16)   # data padded to a paragraph
    bss = (bss + 15) & ~15                        # bss in whole paragraphs
    text = bytes(text) + bytes(len(text) % 2)     # even-length text
    hdr_len, segtab = 96, 64
    tpos = hdr_len + segtab
    dpos = tpos + len(text)
    renv = 0xc80f                      # OS-version, segmented, fixed stack, pure, sep I&D, exec
    h = struct.pack('<HHlllllLBBH', 0x0206, 44, len(text), len(data), bss, 0, 0, 0, 0x44, 0x64, renv)
    e = struct.pack('<9lBBBBHH', 0, 0, 0, 0, stack, hdr_len, segtab, 0, 0, 0, 0, 1, 2, 63, 0)
    out = bytearray(h + e)
    out += bytes(hdr_len - len(out))
    out += struct.pack('<HHHBBllllHHl', 1, 0x8008, 63, 0, 0, tpos, len(text), len(text), 0, 0, 0, 0)
    out += struct.pack('<HHHBBllllHHl', 2, 0x8000 | (4 if bss else 0), 71, 0, 0, dpos, len(data), len(data) + bss, 0, 0, 0, 0)
    out += text + data
    return bytes(out)
