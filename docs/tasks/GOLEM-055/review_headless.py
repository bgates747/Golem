"""Capture the already staged human scene headlessly, then stop actual Fab."""
import json
import time
import qualify as f

destination = f.PROFILE / 'sdcard/golem/ram'
session = f.q.Session((f.PROFILE / 'sdcard/autoexec.txt').read_text(), 'review')
report = {}
try:
    session.wait(destination / 'done.dat', 180)
    report = f.verify_saved(destination)
    matching = f.matching_files(f.BUILD / 'reference', destination / 'package')
    picture = f.ppm.convert(session.capture('human-scene'))
    (f.HERE / 'review-reference.png').write_bytes(picture.read_bytes())
    report.update({'seconds': time.monotonic() - session.started,
                   'matching_package_files': matching,
                   'worker_sha256': f.sha(f.PROFILE / 'sdcard/golem/compiler/portable.bin'),
                   'actual_MOS_compilation': True, 'GUI_launched': False,
                   'human': 'pending', 'scene': 'review-reference.png'})
finally:
    report['frontend_exit'] = session.close()
(f.HERE / 'review-headless-evidence.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report), flush=True)
