#!/usr/bin/env python3
"""Decode a retained binary UART0 stream, without claiming transaction completion."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

MAGIC = 0x47570100
EVENTS = {1: 'DEFAULT_PLANE', 2: 'INITIAL_LIST', 3: 'PREACTIVATE'}


def parse(data):
    records = []
    incomplete_magic_offsets = []
    cursor = 0
    while cursor + 4 <= len(data):
        magic, = struct.unpack_from('<I', data, cursor)
        if magic not in tuple(MAGIC | e for e in EVENTS):
            cursor += 1
            continue
        if cursor + 64 > len(data):
            incomplete_magic_offsets.append(cursor)
            break
        w = struct.unpack_from('<16I', data, cursor)
        ordinal = w[1] & 255
        requested, previous, attempted, tag = struct.pack('<I', w[3])
        phase, attempt = tag & 3, tag >> 2
        if not (1 <= ordinal <= 8 and requested == 20 and 1 <= phase <= 3 and 1 <= attempt <= 3):
            cursor += 1
            continue
        records.append({'offset': cursor, 'event': EVENTS[magic & 255],
                        'ordinal': ordinal, 'flags': hex(w[1] & ~255),
                        'request_sequence': w[2], 'requested': requested,
                        'previous': previous, 'attempted': attempted,
                        'phase': phase, 'attempt': attempt,
                        'controller': hex(w[4]), 'pixel_format': w[5],
                        'table_size': w[6], 'request_bytes': w[7],
                        'capabilities': hex(w[8]), 'allocation_return': hex(w[9]),
                        'map0': hex(w[10]), 'list': hex(w[11]),
                        'current_item': hex(w[12]), 'signals': hex(w[13]),
                        'palette_list_tag': hex(w[14]), 'dimensions': [w[15] & 65535, w[15] >> 16],
                        'words': [hex(x) for x in w]})
        cursor += 64
    ordinals = [r['ordinal'] for r in records]
    saturated = len(records) >= 8 or any(int(r['flags'], 16) & (1 << 15) for r in records)
    groups = {}
    for r in records:
        key = (r['request_sequence'], r['attempt'], r['controller'])
        groups.setdefault(str(key), []).append(r['event'])
    return {'schema': 'g057-display-journal-v1',
            'stream_bytes': len(data), 'stream_sha256': hashlib.sha256(data).hexdigest(),
            'records': records, 'ordinals_contiguous': ordinals == list(range(1, len(records) + 1)),
            'saturated_incomplete': saturated, 'incomplete_magic_offsets': incomplete_magic_offsets,
            'observed_controller_groups': groups,
            'observed_groups_have_preactivate': bool(groups) and all('PREACTIVATE' in e for e in groups.values()),
            'transaction_completion': 'Requires independent consumer/MODE completion evidence; UART prefix alone cannot establish it.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('uart0', type=Path)
    args = parser.parse_args()
    print(json.dumps(parse(args.uart0.read_bytes()), indent=2))


if __name__ == '__main__':
    main()
