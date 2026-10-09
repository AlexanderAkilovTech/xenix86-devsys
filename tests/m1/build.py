"""Milestone 1 tests: exit42.o and hello.o for Xenix 2.1.3 (8086)."""
import sys, os, struct
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'tools'))
from omf import Module, gate_prologue, syscall
from crt import DATA_HEADER

out = sys.argv[1] if len(sys.argv) > 1 else '.'

def mov(reg, val, m):            # mov r16, imm16
    m.text += bytes([0xB8 + reg]) + struct.pack('<H', val)
AX, CX, DX, BX, SP, BP, SI, DI = range(8)

# exit42: exit(42)
m = Module('exit42.s'); gate_prologue(m); m.data += DATA_HEADER
mov(AX, 1, m); mov(BX, 42, m); syscall(m)
m.text += bytes([0xEB, 0xFE])
open(os.path.join(out, 'exit42.o'), 'wb').write(m.build())

# hello: write(1, msg, len); exit(0)
msg = b'hello, world\n'
m = Module('hello.s'); gate_prologue(m)
mov(AX, 4, m); mov(BX, 1, m)
m.data_fixups.append(len(m.text) + 1); mov(CX, len(DATA_HEADER), m)   # cx = offset msg (DGROUP)
mov(SI, len(msg), m); syscall(m)
mov(AX, 1, m); mov(BX, 0, m); syscall(m)
m.text += bytes([0xEB, 0xFE])
m.data += DATA_HEADER + msg
open(os.path.join(out, 'hello.o'), 'wb').write(m.build())
