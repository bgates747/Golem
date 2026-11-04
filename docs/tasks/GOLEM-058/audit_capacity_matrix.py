#!/usr/bin/env python3
"""Replay saved wire only: package identity, calls, independent oracle and frees."""
import argparse
import hashlib
import importlib.util
import json
import struct
from pathlib import Path


def audit(root, cohort, generator):
    spec = importlib.util.spec_from_file_location('capacity_generator', generator)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    plan = json.loads((cohort / 'cohort.json').read_text())
    modules = {e['name']: e for e in plan['modules']}
    packages = {hashlib.sha256((cohort / 'modules' / (n + '.vdpl')).read_bytes()).hexdigest(): n for n in modules}
    for name, expected in plan['files'].items():
        assert hashlib.sha256((cohort / name).read_bytes()).hexdigest() == expected, name
    manifest = json.loads((root / 'SHA256.json').read_text())
    # The retrieval manifest hashes every saved file except itself.
    for name, expected in manifest.items():
        assert hashlib.sha256((root / name).read_bytes()).hexdigest() == expected, name
    audits, compared = [], []
    for engine in ('jit', 'interp'):
        base = root / engine
        runtime = base / 'runtime'
        tx = (runtime / 'wire-tx.dat').read_bytes()
        rx = (runtime / 'wire-rx.dat').read_bytes()
        assert tx == (runtime / 'mos-to-vdp.bin').read_bytes()
        assert rx == (runtime / 'vdp-to-mos-delivered.bin').read_bytes() == (runtime / 'vdp-to-mos.bin').read_bytes()
        replies, modes, audio = [], [], []
        at = 0
        while at < len(rx):
            if rx[at] < 128:
                at += 1
                continue
            kind, count = rx[at:at + 2]
            body = rx[at + 2:at + 2 + count]
            assert len(body) == count
            if kind == 129 and body.startswith(b'VDPL'):
                assert count == 10 and body[4] == 1
                replies.append(struct.unpack_from('<BHH', body, 5))
            elif kind == 134:
                assert count == 8
                modes.append(list(struct.unpack('<HHBBBB', body)))
            elif kind == 133:
                assert count == 2
                audio.append(list(body))
            at += count + 2
        assert b''.join(b'VDPL' + bytes((1, k)) + struct.pack('<HH', t, v) for k, t, v in replies) == (runtime / 'results.dat').read_bytes()
        at = index = calls = refused = stale = handle = 0
        loaded = None
        buffers, records, words, loads, infos = {}, [], [], [], []
        while at < len(tx):
            if tx[at] == 22:
                at += 2
                continue
            assert tx[at] == 23 and at + 2 < len(tx), (at, tx[at:at + 16].hex())
            if tx[at + 1] == 1:
                at += 3
                continue
            assert tx[at + 1] == 0
            op = tx[at + 2]
            if op == 128:
                at += 4
                continue
            if op == 132:
                at += 7
                continue
            if op == 133:
                action = tx[at + 4]
                at += {0: 10, 1: 5, 2: 6, 4: 6, 8: 5, 9: 5, 10: 5}[action]
                continue
            if op == 160:
                bid, action = struct.unpack_from('<HB', tx, at + 3)
                at += 6
                if action == 0:
                    size = struct.unpack_from('<H', tx, at)[0]
                    at += 2
                    buffers.setdefault(bid, bytearray()).extend(tx[at:at + size])
                    at += size
                elif action == 2:
                    buffers.pop(bid, None)
                else:
                    assert action == 14
                continue
            assert op == 216, (at, tx[at:at + 16].hex())
            size = struct.unpack_from('<H', tx, at + 3)[0]
            body = tx[at + 5:at + 5 + size]
            at += 5 + size
            version, operation, token, flags, reserved = struct.unpack_from('<BBHBB', body)
            assert (version, flags, reserved) == (1, 1, 0)
            kind, reply_token, value = replies[index]
            index += 1
            assert token == reply_token == index
            if operation == 1:
                bid, count, zero = struct.unpack_from('<HIH', body, 6)
                assert not handle and zero == 0 and count == len(buffers[bid])
                name = packages[hashlib.sha256(buffers[bid]).hexdigest()]
                loads.append((name, kind == 2))
                if kind == 5:
                    assert value == 6
                    refused += 1
                else:
                    assert kind == 2 and value
                    handle = value
                    loaded = name
                    sequence = 0
            elif operation == 2:
                called, entry, args = struct.unpack_from('<HHH', body, 6)
                if kind == 5:
                    assert handle == 0 and value == 8
                    stale += 1
                    continue
                assert called == handle and handle and entry == 0 and args == 60022 and (kind, value) == (3, 0)
                seed, out, zero = struct.unpack('<IHH', buffers[args])
                assert (out, zero) == (60021, 0)
                sequence += 1
                g = modules[loaded]['generator']
                expected = module.oracle(seed, sequence, g['steps'], g['data_words'], g['bss_words'])
                calls += 1
            elif operation == 3:
                assert struct.unpack_from('<H', body, 6)[0] == handle and (kind, value) == (3, 0)
                handle = 0
            elif operation == 4:
                assert kind == 4
                infos.append((struct.unpack_from('<H', body, 6)[0], value))
            elif operation == 5:
                bid, offset = struct.unpack_from('<HI', body, 6)
                assert bid == 60021 and kind == 4 and offset == len(words) * 2
                words.append(value)
                if len(words) == 16:
                    actual = struct.pack('<16H', *words)
                    assert actual == expected
                    records.append(actual.hex())
                    words = []
            else:
                raise AssertionError(operation)
        assert not words and not handle and index == len(replies)
        report = json.loads((base / 'result.json').read_text())
        expected_calls = plan.get('expected_calls', 147)
        expected_load_count = plan.get('expected_loads', 49)
        expected_refusals = plan.get('expected_memory_refusals', 8)
        assert report['passed'] and report['normal_shutdown'] and report['calls'] == calls == len(records) == expected_calls
        assert refused == expected_refusals and stale == expected_load_count
        expected_loads, expected_records, resource_groups, configurations = [], [], [], []

        def check_load(item):
            expected_loads.append((item['name'], item['loaded']))
            resource_groups.extend([item['before'], item['admission']])
            admitted = item['admission']
            entry = modules[item['name']]
            insufficient = admitted['exec_largest'] < entry['resident_text_bytes'] or admitted['internal_largest'] < entry['resident_data_bytes']
            assert insufficient == item['predicted_insufficient'] == (not item['loaded'])
            if item['loaded']:
                resource_groups.append(item['loaded_resources'])
                expected_records.extend(item['outputs'])
            else:
                assert item['expected_refusal'] == 6
            resource_groups.append(item['after'])
            if not item['warmup']:
                for field in ('exec_free', 'internal_free'):
                    assert item['before'][field] == item['after'][field]
            for state in (item['before'], item['after']):
                assert state['raw']['2'] == 0 and state['raw']['17'] > 1024

        for config in report['configurations']:
            check_load(config['warmup'])
            resource_groups.append(config['steady'])
            for item in config['modules']:
                check_load(item)
                if not item['loaded']:
                    check_load(item['recovery'])
            resource_groups.append(config['after_audio_cleanup'])
            if config['active_audio']:
                for key in ('audio_before', 'audio_after'):
                    assert set(config[key]) == {'0', '31'} and all(v & 7 == 7 for v in config[key].values())
            configurations.append(dict(mode=config['mode'], active_audio=config['active_audio'], steady={k: config['steady'][k] for k in ('exec_free', 'exec_largest', 'internal_free', 'internal_largest')}, refused=[v['name'] for v in config['modules'] if not v['loaded']]))
        for item in report['history']:
            check_load(item)
        assert expected_loads == loads and expected_records == records
        # Every resource group must be backed by consecutive real INFO transactions.
        cursor = 0
        for group in resource_groups:
            values = [(int(n), value) for n, value in group['raw'].items()]
            while infos[cursor:cursor + len(values)] != values:
                cursor += 1
                assert cursor < len(infos)
            cursor += len(values)
            for key, n in (('exec_free', 9), ('exec_largest', 11), ('internal_free', 13), ('internal_largest', 15)):
                assert group[key] == group['raw'][str(n)] | (group['raw'][str(n + 1)] << 16)
        expected_modes = [c['actual_MODE'] for c in report['configurations']] + [[320, 240, 40, 30, 2, 11]]
        # The stock startup emits its initial mode before the first request.
        # Retain that exact boot witness separately; allow no later extras.
        assert modes == [[640, 480, 80, 60, 16, 0]] + expected_modes
        statuses = [p for p in audio if p[0] in (0, 31) and p[1] & 7 == 7]
        assert len(statuses) == 12 and [c for c, _ in statuses] == [0, 31] * 6
        stats = json.loads((runtime / 'stats.json').read_text())
        assert not stats['failed'] and all(stats[k] == 0 for k in ('unmapped', 'unhandled', 'unregistered'))
        compared.append(records)
        audits.append(dict(engine=engine, transactions=index, successful_loads=expected_load_count, calls=calls, refused_MEMORY=refused, stale_handle_refusals=stale, resource_groups=len(resource_groups), requested_MODE_replies=len(expected_modes), boot_MODE_reply=modes[0], active_audio_statuses=len(statuses), configurations=configurations, tx_sha256=hashlib.sha256(tx).hexdigest(), rx_sha256=hashlib.sha256(rx).hexdigest()))
    assert compared[0] == compared[1]
    return dict(passed=True, scope='Independent saved-wire audit; no physical, timing or universal allocation guarantee', identical_oracle_outputs=True, engines=audits)


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    for name in ('root', 'cohort', 'generator', 'output'):
        p.add_argument('--' + name, type=Path, required=True)
    a = p.parse_args()
    result = audit(a.root, a.cohort, a.generator)
    a.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))
