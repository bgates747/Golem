#!/usr/bin/env python3
"""Audit the retrieved frozen VDPL-012 campaign without starting an emulator."""
from collections import Counter
import hashlib
import json
from pathlib import Path
import shutil
import struct

HERE = Path(__file__).resolve().parent
SDK = Path('/Agon/mystuff/agon-vdplet')
RAW = SDK / 'build/vdpl012qualification/oryx'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    freeze = json.loads((HERE / 'freeze.json').read_text())
    expected_modules = {item['name']: item['sha256'] for item in freeze['modules']}
    runs = []
    for name, profile, engine, capabilities in (
        ('rows-r1-jit', 'rows-r1', 'jit', 287),
        ('rows-r1-interp', 'rows-r1', 'interp', 287),
        ('base-r1-jit', 'base-r1', 'jit', 31),
    ):
        directory = RAW / name
        report = json.loads((directory / 'evidence.json').read_text())
        stats = json.loads((directory / 'stats.json').read_text())
        assert report['passed'] and report['engine'] == engine
        assert report['expected_capabilities'] == capabilities
        assert report['contract_sha256'] == freeze['contract_sha256']
        assert report['firmware_sha256'] == freeze['firmware'][profile]['firmware.bin']['sha256']
        assert report['elf_sha256'] == freeze['firmware'][profile]['firmware.elf']['sha256']
        assert report['backend_sha256'] == freeze['backend_provenance']['sha256']
        for key, file in (('qualifier_sha256', 'qualify.py'), ('target_sha256', 'target.py'),
                          ('worker_sha256', 'worker.c')):
            assert report[key] == freeze['sources'][file]
            assert sha(SDK / 'docs/tasks/VDPL-012/qualification' / file) == report[key]
        for module, digest in report['modules'].items():
            if module in expected_modules:
                assert digest == expected_modules[module]
            else:
                assert sha(SDK / 'build/modules' / module) == digest
        assert report['execution_stats'] == stats
        assert not any(stats[key] for key in ('failed', 'unhandled', 'unregistered', 'unmapped'))

        outbound = (directory / 'mos-to-vdp.bin').read_bytes()
        emitted = (directory / 'vdp-to-mos.bin').read_bytes()
        delivered = (directory / 'vdp-to-mos-delivered.bin').read_bytes()
        received = (directory / 'wire-rx.dat').read_bytes()
        replies = (directory / 'results.dat').read_bytes()
        assert len(outbound) == report['outbound_bytes'] == stats['uart_rx']
        assert emitted == delivered == received
        assert len(emitted) == stats['uart_tx'] == stats['uart_delivered']
        assert len(replies) == report['result_bytes'] and len(replies) % 10 == 0
        frames = list(struct.iter_unpack('<4sBBHH', replies))
        assert all(tag == b'VDPL' and version == 1 for tag, version, _, _, _ in frames)
        assert [frame[3] for frame in frames] == list(range(1, len(frames) + 1))
        audit = dict(emitted_delivered_received_equal=True, sequential_reply_tokens=True,
                     vdpl_reply_count=len(frames), vdpl_reply_bytes=len(replies),
                     reply_kinds=dict(sorted(Counter(frame[2] for frame in frames).items())),
                     outbound_bytes=len(outbound), delivered_bytes=len(delivered))
        raw_files = {path.name: dict(bytes=path.stat().st_size, sha256=sha(path))
                     for path in sorted(directory.iterdir()) if path.is_file()}
        destination = HERE / name
        destination.mkdir(exist_ok=True)
        shutil.copy2(directory / 'evidence.json', destination / 'evidence.json')
        runs.append(dict(name=name, profile=profile, engine=engine, passed=True,
                         groups=len(report['checks']), pixels_compared=report['pixels_compared'],
                         elapsed_seconds=report['elapsed_seconds'], checks=report['checks'],
                         boot_resources=report['boot_resources'], final_resources=report['final_resources'],
                         execution_stats=stats, raw_audit=audit, raw_artifact_directory=str(directory),
                         raw_files=raw_files, firmware_sha256=report['firmware_sha256'],
                         elf_sha256=report['elf_sha256'], modules=report['modules']))

    # The engines received the identical fixture; scheduler/stack counters may differ.
    assert (RAW / runs[0]['name'] / 'mos-to-vdp.bin').read_bytes() == (
        RAW / runs[1]['name'] / 'mos-to-vdp.bin').read_bytes()
    release = json.loads((HERE / 'lease-release.json').read_text())
    assert release['generation'] == 20 and release['released']
    assert release['release_observation']['emulators'] == []
    summary = dict(
        passed=True,
        scope='Three sequential fresh actual-Xtensa direct-library processes on Oryx; no MOS, human or physical-target qualification',
        frozen_inputs_sha256=sha(HERE / 'freeze.json'),
        contract_sha256=freeze['contract_sha256'],
        clarification_sha256=freeze['clarification_sha256'],
        source_pins=freeze['sources'], backend=freeze['backend_provenance'],
        total_groups=sum(run['groups'] for run in runs),
        total_pixels_compared=sum(run['pixels_compared'] for run in runs),
        total_execution_seconds=round(sum(run['elapsed_seconds'] for run in runs), 3),
        positive_engines_identical_outbound_fixture=True,
        module_local_stack=freeze['module_local_stack'],
        release=dict(owner=release['owner'], generation=release['generation'], lease=release['lease'],
                     granted=release['granted'], released=release['released'], fresh_host_idle=True),
        limitations=[
            'Target checks ordinary C3 then RUN rebinding and hidden-before-C3. Injected pending-Canvas-swap ordering belongs to the separate runtime host test/source proof.',
            'Run elapsed time includes boot, transport, qualification and decoded frame capture; it is not an isolated rendering benchmark.',
            'INFO17 is observed command-task stack high-water headroom; the 848-byte own-worker static chain is not a whole-firmware stack bound.',
            'Boot and final heaps use different screen/buffer states; the values are observations, not a leakage or resident feature-cost measurement.',
        ],
        runs=runs,
    )
    (HERE / 'results.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(f"PASS: {summary['total_groups']} groups, {summary['total_pixels_compared']} decoded pixels; raw transport audit passed.")


if __name__ == '__main__':
    main()
