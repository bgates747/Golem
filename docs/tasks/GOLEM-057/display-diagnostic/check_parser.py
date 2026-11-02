#!/usr/bin/env python3
"""Bounded binary-journal decoder checks; no emulator dependency."""
import struct
from parse_journal import parse


def record(event, ordinal, attempt=1, phase=1, controller=0x3ffb92d4):
    w = [0] * 16
    w[0] = 0x47570100 | event
    w[1] = ordinal | ((1 << 15) if ordinal == 8 else 0)
    w[2] = 1
    w[3] = 20 | 11 << 8 | (20 if attempt == 1 else 11) << 16 | (phase | attempt << 2) << 24
    w[4] = controller
    w[5] = 3
    w[6] = 2048
    return struct.pack('<16I', *w)


data = b'boot\r\n\x00\xff' + b''.join(record(e, i + 1) for i, e in enumerate([1, 2, 3]))
result = parse(data)
assert len(result['records']) == 3 and result['ordinals_contiguous']
assert result['records'][0]['offset'] == 8
assert result['observed_groups_have_preactivate'] and not result['saturated_incomplete']
assert result['records'][0]['controller'] == '0x3ffb92d4'
assert 'independent' in result['transaction_completion']
result = parse(data + b'noise' + b''.join(record(e, i + 4, 2, 2) for i, e in enumerate([1, 2, 3])))
assert len(result['records']) == 6 and len(result['observed_controller_groups']) == 2
assert result['ordinals_contiguous'] and result['observed_groups_have_preactivate']
assert parse(record(1, 1))['observed_groups_have_preactivate'] is False
assert parse(b''.join(record(1, i + 1) for i in range(8)))['saturated_incomplete']
assert len(parse(record(1, 9) + record(1, 1))['records']) == 1
assert not parse(record(1, 2))['ordinals_contiguous']
assert parse(record(3, 1)[:30])['incomplete_magic_offsets'] == [0]
assert not parse(b'ordinary UART log')['records']
print('PASS: binary noise, arbitrary offsets, little-endian words, attempts/address reuse, cap8, malformed ordinal, missing/truncated events, external completion requirement')
