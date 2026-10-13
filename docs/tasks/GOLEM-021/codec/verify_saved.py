"""Recheck retrieved codec evidence offline, including measured retained hashes."""
from contextlib import redirect_stdout
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import struct
import zlib

HERE = Path(__file__).resolve().parent
EVIDENCE = HERE / 'evidence'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    spec = importlib.util.spec_from_file_location('saved_consumer',
                                                HERE.parent / 'consumer/verify.py')
    consumer = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(consumer)
    manifest = json.loads((EVIDENCE / 'manifest.json').read_text())
    for name, expected in manifest['files'].items():
        assert Path(name).name == name and digest((EVIDENCE / name).read_bytes()) == expected
    summary = json.loads((EVIDENCE / 'summary.json').read_text())
    observed = []
    for case in summary['cases']:
        name = case['fixture'] + '-' + case['engine']
        expected = EVIDENCE / (name + '-expected.json')
        meta = json.loads(expected.read_text())
        result = (EVIDENCE / (name + '.dat')).read_bytes()
        with redirect_stdout(io.StringIO()):
            transcript = consumer.verify(EVIDENCE / (name + '.dat'), expected)
        pixels = consumer.verify_pixels(EVIDENCE / (name + '-final.ppm'),
                    HERE / 'build/cases' / case['fixture'] / 'build/apple-assets/loderunner.packed')
        stats = json.loads((EVIDENCE / (name + '-stats.json')).read_text())
        assert not any(stats[k] for k in ('failed', 'unhandled', 'unregistered', 'unmapped'))
        retained = {}
        for i, check in enumerate(meta['checks']):
            prefix = 'complete retained word '
            if check['label'].startswith(prefix):
                offset = int(check['label'][len(prefix):])
                retained[offset] = result[i*10+8:i*10+10]
        receipt = dict(fixture=case['fixture'],engine=case['engine'],
                       observed_reply_sha256=transcript['observed_sha256'],
                       expected_retained_sha256=meta['retained_sha256'],
                       native_entry_addresses=transcript['native_entry_addresses'],
                       pixel_checks=pixels['pixel_checks'])
        if meta['retained_bytes_observed_completely']:
            assert sorted(retained) == list(range(0, 7680, 2))
            image = b''.join(retained[offset] for offset in range(0, 7680, 2))
            receipt['observed_retained_sha256'] = digest(image)
            assert receipt['observed_retained_sha256'] == meta['retained_sha256']
            receipt['observed_retained_bytes'] = len(image)
        else:
            assert not retained
            receipt['observed_retained_bytes'] = 0
        observed.append(receipt)
    report = dict(passed=True,runs=len(observed),
                  completely_observed_retained_bytes=sum(r['observed_retained_bytes'] for r in observed),
                  pixel_checks=sum(r['pixel_checks'] for r in observed),
                  notes='Expected retained digests are not observations; observed digests exist only for complete reads.',
                  cases=observed)
    (EVIDENCE / 'independent-verification.json').write_text(json.dumps(report,indent=2)+'\n')
    # Lossless format conversion only: retain every pixel of the actual VGA
    # capture, including the source-role labels and guest review instructions.
    ppm = (EVIDENCE / 'f3-step07-jit-final.ppm').read_bytes()
    magic, dimensions, maximum, rgb = ppm.split(b'\n',3)
    width,height = map(int,dimensions.split())
    assert magic == b'P6' and maximum == b'255' and len(rgb)==width*height*3
    rows = b''.join(b'\0'+rgb[y*width*3:(y+1)*width*3] for y in range(height))
    def chunk(tag,data):
        return struct.pack('>I',len(data))+tag+data+struct.pack('>I',zlib.crc32(tag+data))
    png = (b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,2,0,0,0))+
           chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b''))
    (EVIDENCE / 'f3-native-output.png').write_bytes(png)
    print(f'Offline verification passes: {len(observed)} runs, '
          f'{report["pixel_checks"]:,} image/margin pixels, '
          f'{report["completely_observed_retained_bytes"]:,} fully observed retained bytes.')


if __name__ == '__main__':
    main()
