#!/usr/bin/env python3
"""Generate the task-local Flower renderer from the compiler's shared layout."""
from pathlib import Path
import argparse
import hashlib
import json
import struct

HERE = Path(__file__).resolve().parent
word = lambda n: struct.pack('<H', n)
packet = lambda key, op: bytes((23, 0, 160)) + word(key) + bytes((op,))
call = lambda key: packet(key, 1)
write = lambda key, data: packet(key, 2) + packet(key, 0) + word(len(data)) + data
copy = lambda key, offset, source, start, count=2: packet(key, 5) + b'\xe2' + word(offset) + word(count) + word(source) + word(start)
plot = lambda op, x, y: bytes((25, op)) + struct.pack('<hh', x, y)
sha = lambda data: hashlib.sha256(data).hexdigest()


def generate(package, output, ids=(60002, 60003, 60004, 60005)):
    interface, root, backend = [json.loads((package / name).read_text())
                                for name in ('interface.json', 'root.json', 'backends.json')]
    fields = {f['name']: f for f in interface['globals']}
    arrays = {a['name']: a for a in interface['arrays']}
    entries = {e['name']: e['logical_id'] for e in interface['entries']}
    assert root['schema'] == 1 and interface['state_buffer'] == backend['shared_state']
    assert entries.keys() >= {'prepare', 'batch'}
    for name, kind in [('startX', 'sint16'), ('startY', 'sint16'), ('batchCount', 'uint16')]:
        f = fields[name]
        assert f['type'] == kind and f['bytes'] == 2 and f['offset'] + 2 <= interface['state_bytes']
    for name in ('pointsX', 'pointsY'):
        a = arrays[name]
        assert (a['type'], a['lower'], a['count'], a['element_bytes'], a['bytes']) == ('sint16', 0, 32, 2, 64)
        assert a['offset'] == 0
    borrowed = {backend['shared_state'], *(a['buffer'] for a in interface['arrays'])}
    occupied = borrowed | set(backend['native_owned']) | set(backend['buffered_owned'])
    assert len(set(ids)) == 4 and not occupied.intersection(ids)
    assert all(0 < key < 65535 and any(r['first'] <= key <= r['last']
               for r in interface['external_reservations']) for key in ids), 'renderer IDs must be reserved in the compiler layout'
    render, move, line, guard = ids
    state = backend['shared_state']
    offsets = root['fields']
    count = fields['batchCount']['offset']

    def stop(op, source, offset, operand_offset):
        # Jump to the end of this buffer when the unsigned 16-bit test passes.
        # Both values must be buffer fetched: firmware preserves the upper sentinel
        # bytes on fetch, whereas a literal operand is zero extended.
        return packet(65535, 8) + bytes((0xa0 | op,)) + word(source) + word(offset) + word(guard) + word(operand_offset)

    def check_byte(op, offset, operand_offset=0):
        # A padded byte never equals the firmware's invalid 0xffff word sentinel.
        return copy(guard, 6, state, offset, 1) + stop(op, guard, 6, operand_offset)

    code = bytearray()
    for offset in (offsets['interface_status'], offsets['language_status'],
                   offsets['runtime_result'], offsets['runtime_result'] + 2):
        code += check_byte(3, offset) + check_byte(3, offset + 1)
    assert entries['batch'] < 256
    for offset in (offsets['last_success_entry'], offsets['requested_entry']):
        code += check_byte(3, offset, 2) + check_byte(3, offset + 1)
    code += check_byte(3, count + 1) + check_byte(2, count) + check_byte(5, count, 4)
    code += copy(move, 2, state, fields['startX']['offset'])
    code += copy(move, 4, state, fields['startY']['offset']) + call(move)
    for slot in range(32):
        if slot:
            code += stop(2, state, count, 8 + slot * 2)
        code += copy(line, 2, arrays['pointsX']['buffer'], slot * 2)
        code += copy(line, 4, arrays['pointsY']['buffer'], slot * 2) + call(line)
    constants = word(0) + word(entries['batch']) + word(32) + word(0) + b''.join(word(slot) for slot in range(32))
    blocks = {render: bytes(code), move: plot(4, 0, 0), line: plot(5, 0, 0), guard: constants}
    install = b''.join(write(key, data) for key, data in blocks.items())
    cleanup = b''.join(packet(key, 2) for key in ids)
    metadata = {
        'schema': 1, 'scope': 'GOLEM-044 task-local stock VDU bridge',
        'entry': render, 'owned': list(ids), 'borrowed': sorted(borrowed),
        'state_buffer': state, 'batch_entry': entries['batch'],
        'batch_native_wrapper': backend['native_wrappers'][entries['batch']],
        'count_offset': count, 'max_segments': 32,
        'root_fields': offsets,
        'logical_coordinates': {'width': 1280, 'height': 1024, 'origin': [640, 512], 'y_direction': 'up'},
        'screen': {'mode': 0, 'width': 640, 'height': 480},
        'runtime_completion_owner': 'host must check invocation runtime result before render; six-byte calls are submission only',
        'zero_error_or_invalid_count': 'no plotting; shared mathematical state and arrays unchanged',
        'blocks': [{'buffer': key, 'bytes': len(data), 'sha256': sha(data)} for key, data in blocks.items()],
        'resident_bytes': sum(map(len, blocks.values())), 'installation_wire_bytes': len(install),
        'render_call_wire_bytes': len(call(render)), 'cleanup_wire_bytes': len(cleanup),
        'source_hashes': {name: sha((package / name).read_bytes()) for name in ('interface.json', 'root.json', 'backends.json')},
    }
    output.mkdir(parents=True, exist_ok=True)
    for name, data in [('renderer-install.bin', install), ('renderer-cleanup.bin', cleanup), ('renderer-call.bin', call(render))]:
        (output / name).write_bytes(data)
    (output / 'renderer.json').write_text(json.dumps(metadata, indent=2) + '\n')
    asm = ['; Generated task-local Flower bridge. Install before calling; wait/check runtime separately.',
           f'flower_renderer_entry: equ {render}', f'flower_renderer_state: equ {state}',
           'flower_render:', ' ld hl,flower_render_call', ' ld bc,6', ' rst.lil $18', ' ret']
    for label, data in [('flower_render_call', call(render)), ('flower_renderer_install', install), ('flower_renderer_cleanup', cleanup)]:
        asm += [label + ':']
        asm += [' db ' + ','.join(str(b) for b in data[start:start+16]) for start in range(0, len(data), 16)]
        asm += [label + '_bytes: equ $-' + label]
    (output / 'renderer.inc').write_text('\n'.join(asm) + '\n')
    return metadata


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('package', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--ids', type=int, nargs=4, default=(60002, 60003, 60004, 60005))
    args = parser.parse_args()
    print(json.dumps(generate(args.package, args.output, args.ids), indent=2))
