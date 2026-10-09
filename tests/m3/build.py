import sys, os, struct
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'tools'))
from xout import xout
w = lambda v: struct.pack('<H', v)
GATE = bytes([0xEB, 0x0E, 0xCD, 0x05, 0xC3]) + bytes([0x90] * 11)   # 0: jmp 0x10 / 2: int 5; ret
INT5 = bytes([0xCD, 0x05]); HANG = bytes([0xEB, 0xFE])
def call_gate(at): return bytes([0xE8]) + w((2 - (at + 3)) & 0xffff)
def mov(r, v): return bytes([0xB8 + r]) + w(v)
AX, CX, DX, BX, SP, BP, SI, DI = range(8)
def prog(body_fn):
    t = bytearray(GATE); body_fn(t); return bytes(t)
P = {}
P['t0'] = prog(lambda t: t.extend(HANG))                                  # runs forever if our code runs
P['t1'] = prog(lambda t: (t.extend(mov(AX,1)+mov(BX,42)), t.extend(call_gate(len(t))), t.extend(HANG)))
P['t2'] = prog(lambda t: t.extend(mov(AX,1)+mov(BX,42)+INT5+HANG))        # direct int 5
def t3(t):  # 42 everywhere: registers and stack, bx -> pointer to 42s
    t.extend(mov(CX,42)+mov(DX,42)+mov(SI,42)+mov(DI,42))
    for _ in range(4): t.extend(bytes([0x6A]) if False else mov(AX,42)+bytes([0x50]))  # push 42 (8086 has no push imm)
    t.extend(bytes([0x8B, 0xDC]))   # mov bx,sp
    t.extend(mov(AX,1)+INT5+HANG)
P['t3'] = prog(t3)
msg = b'hello, world\n'
P['h1'] = prog(lambda t: (t.extend(mov(AX,4)+mov(BX,1)+mov(CX,0)+mov(SI,len(msg))), t.extend(call_gate(len(t))),
                          t.extend(mov(AX,1)+mov(BX,0)), t.extend(call_gate(len(t))), t.extend(HANG)))
P['h2'] = prog(lambda t: t.extend(mov(AX,4)+mov(BX,1)+mov(CX,0)+mov(SI,len(msg))+INT5+mov(AX,1)+mov(BX,0)+INT5+HANG))
def h3(t):  # pointer form: push args, bx = sp
    t.extend(mov(AX,len(msg))+b'\x50'+mov(AX,0)+b'\x50'+mov(AX,1)+b'\x50'+bytes([0x8B,0xDC])+mov(AX,4)+INT5)
    t.extend(mov(AX,0)+b'\x50'+bytes([0x8B,0xDC])+mov(AX,1)+INT5+HANG)
P['h3'] = prog(h3)
out = sys.argv[1]
for n, text in P.items():
    data = msg if n.startswith('h') else b''
    open(os.path.join(out, n), 'wb').write(xout(text, data))
print(' '.join(P))
