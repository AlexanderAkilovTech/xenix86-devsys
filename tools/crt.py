"""Xenix 2.x 8086 text-segment reserved block (first 0x80 bytes).

The kernel begins execution at text offset 0x80. Offsets below that form a
fixed interface used by Xenix programs: 0 = jump to entry, 2 = system call gate
(int 5 ; ret), 4/6 = stack-growth request, 8..0x12 = unused traps.
Written for this project; matches the interface, not SCO's bytes."""
ENTRY = 0x80
def _j(at, to): return bytes([0xEB, (to - (at + 2)) & 0xff])
def reserved():
    # Exact Xenix 2.x offsets: the kernel relies on these positions, not just the jumps.
    #  0: jmp 0x80 | 2: jmp 0x17 (gate) | 4,6: jmp 0x14 (stack grow) | 8..0x12: jmp $
    #  0x14: mov ax,0828h | 0x17: int 5 | 0x19: jmp 0x7f | 0x7f: ret
    t = bytearray(ENTRY)
    t[0:2] = _j(0, ENTRY); t[2:4] = _j(2, 0x17); t[4:6] = _j(4, 0x14); t[6:8] = _j(6, 0x14)
    for a in range(8, 0x14, 2): t[a:a+2] = b'\xEB\xFE'
    t[0x14:0x17] = b'\xB8\x28\x08'
    t[0x17:0x19] = b'\xCD\x05'
    t[0x19:0x1b] = _j(0x19, 0x7f)
    t[0x7f] = 0xC3
    return t

# Data-segment header every Xenix 8086 program starts with: word 0 = 0, word 2 = stack
# limit (0xFFFF until the runtime first grows the stack). Program data follows at offset 4.
DATA_HEADER = b'\x00\x00\xff\xff'
