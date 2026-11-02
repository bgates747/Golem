#!/usr/bin/env python3
"""Independent byte-count/vector proof for the unallocated PCM wire sketch."""
import json
import struct
from pathlib import Path

HERE = Path(__file__).resolve().parent
state_format = struct.Struct('<IIHHHHIIIBBBB')
assert state_format.size == 32
values = (0x12345678, 24000, 8192, 240, 480, 7712,
          0xFFFFFFFE, 123456, 42, 3, 31, 0, 3)
state = state_format.pack(*values)
token = 0x3456
packets = [b'\x81\x0e' + b'PS1' + bytes([ord('0') + part]) +
           struct.pack('<H', token) + state[8*part:8*part+8]
           for part in range(4)]
assert all(packet[1] == len(packet)-2 <= 16 for packet in packets)
assert sum(map(len, packets)) == 64
assert b''.join(packet[8:] for packet in packets) == state
assert state_format.unpack(state) == values
ack = b'\x81\x0ePS1A' + struct.pack('<HBBIH', token, 2, 0, values[0], 7712)
assert len(ack) == 16 and ack[1] == len(ack)-2
request = bytes([23, 0, 0x85, 31, 16, 1, 2]) + struct.pack('<HHIHIH',
    token, 8, values[0], 60020, 0x12345678, 240)
assert len(request) == 23
assert request[9:11] == b'\x08\x00'

# Marker convention cannot match a proper four-byte physical key packet:
# its fourth byte would need to be key-up/down0/1, not ASCII0..3/A.
assert all(packet[5] not in (0, 1) for packet in packets + [ack])
result = dict(scope='draft layout/count proof only; no firmware/parser qualification',
              request_bytes=len(request), write_ack_wire_bytes=len(ack),
              state_bytes=len(state), state_wire_bytes=sum(map(len, packets)),
              state_packet_body_bytes=[packet[1] for packet in packets],
              request_hex=request.hex(), ack_hex=ack.hex(),
              state_packet_hex=[packet.hex() for packet in packets])
(HERE/'layout-evidence.json').write_text(json.dumps(result, indent=2)+'\n')
print(json.dumps(result, indent=2))
