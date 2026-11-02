#!/usr/bin/env python3
"""Audit the four completed Pi processes; never load an emulator library."""
from collections import Counter
import hashlib
import json
from pathlib import Path
import shutil
import struct

HERE = Path(__file__).resolve().parent
SDK = Path('/Agon/mystuff/agon-vdplet')
RAW = SDK / 'build/vdpl012arm64qualification'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    freeze = json.loads((HERE / 'execution-freeze.json').read_text())
    prepared = json.loads((HERE / 'source-freeze.json').read_text())
    idle = json.loads((HERE / 'pi-final-idle.json').read_text())
    assert idle['machine'] == 'raspberrypi' and idle['emulators'] == []
    runs = []
    for name, kind, engine in (
        ('toy-jit-r01', 'toy', 'jit'), ('toy-interp-r01', 'toy', 'interp'),
        ('rows-r1-jit', 'rows', 'jit'), ('rows-r1-interp', 'rows', 'interp'),
    ):
        directory = RAW / name
        report = json.loads((directory / 'evidence.json').read_text())
        stats = json.loads((directory / 'stats.json').read_text())
        assert report['passed'] and report['engine'] == engine
        assert stats == report['execution_stats']
        assert not any(stats[key] for key in ('failed', 'unhandled', 'unregistered', 'unmapped'))
        emitted = (directory / 'vdp-to-mos.bin').read_bytes()
        assert emitted == (directory / 'vdp-to-mos-delivered.bin').read_bytes() == (directory / 'wire-rx.dat').read_bytes()
        sent = (directory / 'mos-to-vdp.bin').read_bytes()
        replies = (directory / 'results.dat').read_bytes()
        assert len(sent) == report['outbound_bytes'] == stats['uart_rx']
        assert len(emitted) == stats['uart_tx'] == stats['uart_delivered']
        assert len(replies) == report['result_bytes']
        frames = list(struct.iter_unpack('<4sBBHH', replies))
        assert all(frame[:2] == (b'VDPL', 1) for frame in frames)
        assert [frame[3] for frame in frames] == list(range(1, len(frames) + 1))
        observed_backend = report['backend']['sha256'] if kind == 'toy' else report['backend_sha256']
        assert observed_backend == freeze['permit']['backend_sha256']
        if kind == 'toy':
            assert report['harness_sha256'] == prepared['sources']['toy_gate.py']
            assert report['toy_state_sha256'] == prepared['toy_inputs']['oracle']['sha256']
            assert sha(directory / 'toy-state.bin') == report['toy_state_sha256']
            assert report['complete_64byte_oracle_match'] and (report['toy_steps'], report['toy_cycles']) == (8, 29)
            assert report['init_complete'] and report['fini_unload_complete'] and report['empty_slot']
            assert report['before_load']['executable_free_bytes'] == report['after_unload']['executable_free_bytes']
            before, after = report['before_load'], report['after_unload']
            checks = {key: report[key] for key in ('text_bytes', 'data_bytes', 'toy_state_sha256', 'toy_steps',
                      'toy_cycles', 'complete_64byte_oracle_match', 'init_complete', 'fini_unload_complete',
                      'empty_slot', 'same_mode_executable_heap_recovered')}
            pixels, groups = 0, 0
        else:
            assert report['expected_capabilities'] == 287 and report['rows_expected']
            assert report['firmware_sha256'] == freeze['profile']['pins']['esp32-firmware.bin']
            assert report['elf_sha256'] == freeze['profile']['pins']['esp32-firmware.elf']
            for key, source in (('qualifier_sha256', 'qualify.py'), ('worker_sha256', 'worker.c'), ('target_sha256', 'target.py')):
                assert report[key] == prepared['unchanged_rows_qualifier'][source]
            before, after = report['boot_resources'], report['final_resources']
            pixels, groups, checks = report['pixels_compared'], len(report['checks']), report['checks']
            assert (pixels, groups) == (2620416, 11)
        assert after['command_stack_free_bytes'] > 0 and before['tasks'] == after['tasks'] == 12
        destination = HERE / name
        destination.mkdir(exist_ok=True)
        shutil.copy2(directory / 'evidence.json', destination / 'evidence.json')
        runs.append(dict(name=name, kind=kind, engine=engine, passed=True,
                         elapsed_seconds=report['elapsed_seconds'], row_groups=groups, pixels_compared=pixels,
                         before_resources=before, after_resources=after, checks=checks, execution_stats=stats,
                         raw_audit=dict(emitted_delivered_received_equal=True, sequential_reply_tokens=True,
                                        vdpl_reply_count=len(frames), vdpl_reply_bytes=len(replies),
                                        reply_kinds=dict(Counter(frame[2] for frame in frames)),
                                        outbound_bytes=len(sent), delivered_bytes=len(emitted)),
                         raw_directory=str(directory.relative_to(SDK)),
                         raw_files={path.name: dict(bytes=path.stat().st_size, sha256=sha(path))
                                    for path in sorted(directory.iterdir()) if path.is_file()}))
    for left, right in ((0, 1), (2, 3)):
        assert (RAW / runs[left]['name'] / 'mos-to-vdp.bin').read_bytes() == (RAW / runs[right]['name'] / 'mos-to-vdp.bin').read_bytes()
    summary = dict(
        schema='vdpl012-arm64-mechanism-results-v1', passed=True,
        scope='Four fresh sequential Pi ARM64 direct-library processes; finite CPU/row mechanism only, no real application/MOS/human/physical or speed qualification',
        permit_id=freeze['permit']['permit_id'], preparation_freeze_sha256=sha(HERE / 'source-freeze.json'),
        execution_freeze_sha256=sha(HERE / 'execution-freeze.json'),
        architecture=freeze['architecture'], compiler=freeze['compiler'], compiler_target=freeze['compiler_target'],
        owner_artifacts=freeze['owner_build'], host_checks=freeze['host_checks'],
        diagnostic_exception=freeze['diagnostic_exception'], profile=freeze['profile'],
        source_equivalent_x86_backend='0866265bd6f3d73ad572f31dc1bb954f308ed9a5d857efa9414d37e1ade13374',
        previous_generic_x86_backend='f7e5bf316258793c32313e2f367e28d09519dda82a72c40e4c3e204b61dab615',
        toy_inputs=prepared['toy_inputs'], toy_harness_sha256=prepared['sources']['toy_gate.py'],
        immutable_sdk=prepared['immutable_sdk'], unchanged_rows_qualifier=prepared['unchanged_rows_qualifier'],
        row_modules=prepared['row_modules'], legacy_modules=prepared['legacy_modules'],
        processes=4, toy_cases=2, row_groups=sum(run['row_groups'] for run in runs),
        total_pixels_compared=sum(run['pixels_compared'] for run in runs),
        total_execution_seconds=round(sum(run['elapsed_seconds'] for run in runs), 3),
        identical_outbound_fixture_within_each_engine_pair=True, normal_shutdown_all=True,
        fresh_pi_idle=idle, oryx_used=False, target_retry=False, frozen_source_or_expectation_changes=False,
        limitations=[
            'Native ARM64 artifact is distinct from x86; source equivalence refers to maintained 0866265b, not relabelling the prior f7e5 generic row evidence.',
            'Elapsed times include startup, transport and capture; host architecture/JIT/compiler differences preclude a consumer or physical speed claim.',
            'Toy executable/internal heap fully recover in unchanged mode 11; each observes 44 fewer PSRAM bytes after buffer cleanup, not a PSRAM recovery claim.',
            'Row boot/final screen states differ; heap readings are observations. Actual command stack headroom is measured, not a static whole-call guarantee.',
            'Public C3/RUN rebinding is exercised; injected pending-Canvas-swap ordering remains separate host/source evidence.',
        ], runs=runs)
    (HERE / 'results.json').write_text(json.dumps(summary, indent=2) + '\n')
    print('PASS: four ARM64 processes, two original toy oracle/lifecycle cases, 22 frozen row groups / 5,240,832 decoded pixels; raw audit passed.')


if __name__ == '__main__':
    main()
