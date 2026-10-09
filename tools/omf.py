"""Minimal Intel OMF (8086 object module) writer for the Xenix Development System project.

Produces small-model objects with _TEXT (class CODE) and _DATA (class DATA)
grouped in DGROUP, the same layout Xenix 2.x's crt0 uses, so SCO's native
ld(CP) can link them.
"""
import struct

def _rec(rtype, body):
    data = bytes([rtype]) + struct.pack('<H', len(body) + 1) + body
    chk = (-sum(data)) & 0xff
    return data + bytes([chk])

def _name(s):
    b = s.encode('ascii')
    return bytes([len(b)]) + b

class Module:
    def __init__(self, name):
        self.name = name
        self.text = bytearray()
        self.data = bytearray()
        self.data_fixups = []   # offsets in text holding a 16-bit DGROUP offset
        self.publics = {}       # name -> text offset
        self.start = 0
        self.model = '0sO'

    def build(self):
        self.data += bytes(-len(self.data) % 16)
        self.text += bytes(len(self.text) % 2)
        out = bytearray()
        out += _rec(0x80, _name(self.name))
        # COMENT class 0x9D: CPU/memory-model tag ('0' = 8086, 's' = small, 'O' = optimized).
        # SCO's ld takes the x.out CPU type from this record; without it x_cpu is left as 0.
        out += _rec(0x88, bytes([0x00, 0x9D]) + self.model.encode('ascii'))
        # LNAMES: 1="" 2=_TEXT 3=_DATA 4=DGROUP 5=CODE 6=DATA
        out += _rec(0x96, b''.join(_name(n) for n in
                    ['', '_TEXT', '_DATA', 'DGROUP', 'CODE', 'DATA']))
        # SEGDEF: ACBP 0x48 = word aligned, public combine
        out += _rec(0x98, bytes([0x48]) + struct.pack('<H', len(self.text)) + bytes([2, 5, 1]))
        out += _rec(0x98, bytes([0x48]) + struct.pack('<H', len(self.data)) + bytes([3, 6, 1]))
        # GRPDEF DGROUP = { _DATA }
        out += _rec(0x9a, bytes([4, 0xff, 2]))
        for n, off in self.publics.items():
            out += _rec(0x90, bytes([0, 1]) + _name(n) + struct.pack('<H', off) + b'\x00')
        # code
        out += _rec(0xa0, bytes([1]) + struct.pack('<H', 0) + bytes(self.text))
        if self.data_fixups:
            fx = bytearray()
            for off in self.data_fixups:
                # locat: fixup, segment-relative, 16-bit offset; fixdat: frame=GRPDEF, target=SEGDEF, no displacement
                fx += bytes([0xC4 | (off >> 8), off & 0xff, 0x14, 1, 2])
            out += _rec(0x9c, bytes(fx))
        if self.data:
            out += _rec(0xa0, bytes([2]) + struct.pack('<H', 0) + bytes(self.data))
        # MODEND: main module with start address _TEXT:start
        out += _rec(0x8a, bytes([0xC1, 0x00, 1, 1]) + struct.pack('<H', self.start))
        return bytes(out)

def gate_prologue(m):
    """Lay down the Xenix reserved block; code that follows starts at the 0x80 entry."""
    from crt import reserved, ENTRY
    m.text += reserved()
    m.start = 0

def syscall(m):
    here = len(m.text)
    rel = (2 - (here + 3)) & 0xffff
    m.text += bytes([0xE8]) + struct.pack('<H', rel)
