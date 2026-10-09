import struct, sys, os
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools'))
from xout import xout
from crt import reserved
w = lambda v: struct.pack('<H', v & 0xffff)
def mov(r, v): return bytes([0xB8 + r]) + w(v)
AX, CX, DX, BX, SP, BP, SI, DI = range(8)
INT5 = b'\xCD\x05'; HANG = b'\xEB\xFE'
def prog(body):
    out = bytearray(reserved())
    for item in body:
        out += (bytes([0xE8]) + w(2 - (len(out) + 3))) if item == 'GATE' else item
    return bytes(out)
msg = b'hello, world\n'
P = {
 'exit42': (prog([mov(AX,1), mov(BX,42), 'GATE', HANG]), b''),
 'hello':  (prog([mov(AX,4), mov(BX,1), mov(CX,0), mov(SI,len(msg)), 'GATE', mov(AX,1), mov(BX,0), 'GATE', HANG]), msg),
}
out = sys.argv[1]
for n, (t, d) in P.items():
    open(os.path.join(out, n), 'wb').write(xout(t, d, bss=16))
