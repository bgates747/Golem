#!/usr/bin/env python3
"""Compare the same Pi renderer fixture under both actual Xtensa engines."""
from pathlib import Path
import hashlib
import json

HERE = Path(__file__).resolve().parent
EVIDENCE = HERE / 'evidence'
ROOT = HERE.parents[3]
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()


def compare_runs():
    reports = [json.loads((EVIDENCE / ('pi-' + engine + '.json')).read_text()) for engine in ('jit', 'interp')]
    for report in reports:
        assert report['passed'] and report['host'] == 'raspberrypi'
        assert not any(report['backend_stats'][key] for key in ('failed', 'unhandled', 'unregistered', 'unmapped'))
        assert all(t['outbound_bytes'] == 49 for t in report['timings'])
    for key in ('firmware_sha256', 'elf_sha256', 'adapter_sha256', 'curves', 'lifecycle', 'pixel_comparisons'):
        assert reports[0][key] == reports[1][key], key
    a, b = [(EVIDENCE / ('pi-' + engine + '-results.dat')).read_bytes() for engine in ('jit', 'interp')]
    assert a == b, 'VDPlet replies differ'
    curves = reports[0]['curves']
    assert curves[0]['pixels_sha256'] == next(c['pixels_sha256'] for c in curves if c['case'] == 'full-circle')
    assert curves[0]['pixels_sha256'] == curves[-1]['pixels_sha256'], 'relocated default curve differs'
    for image in EVIDENCE.glob('*-jit.png'):
        assert image.read_bytes() == image.with_name(image.name.replace('-jit.png', '-interp.png')).read_bytes()
    sources = [HERE / 'build_renderer.py', HERE / 'qualify.py', Path(__file__), HERE.parent / 'flower.golem']
    report = {'passed': True, 'engines': ['jit', 'interp'], 'scope': 'identical final Pi fixture; no Oryx or physical qualification',
              'identical_vdplet_reply_bytes': len(a), 'reply_sha256': sha(EVIDENCE / 'pi-jit-results.dat'),
              'pixel_comparisons_per_engine': reports[0]['pixel_comparisons'], 'curve_count_per_engine': len(curves),
              'all_curve_images_counts_and_numeric_ULPs_identical': True, 'default_equals_full_circle': True,
              'layout_relocation_preserves_default_image': True, 'per_batch_outbound_bytes': 49,
              'per_batch_reply_bytes': 30, 'core_compute_render_outbound_bytes': 12,
              'per_engine_steady_batches': len(reports[0]['timings']),
              'sources': {str(p.relative_to(ROOT)): sha(p) for p in sources},
              'images': {p.name: sha(p) for p in sorted(EVIDENCE.glob('*.png'))}}
    (EVIDENCE / 'parity.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f"Pi engine parity passes: {len(a)} identical reply bytes; {len(curves)} identical curves.")


if __name__ == '__main__':
    compare_runs()
