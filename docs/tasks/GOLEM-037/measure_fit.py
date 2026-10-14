"""Measure the accepted renderer with unchanged SDK target flags and limits.

No images are deployed and no admission policy is modified. Build products are
ignored; the compact source/tool/resource receipt is retained in evidence/.
"""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import struct
import subprocess

HERE = Path(__file__).resolve().parent
SDK = HERE.parents[2].parent / 'agon-vdplet'
COMPILER = Path.home() / '.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-gcc'
BUILD = HERE / 'build'
EVIDENCE = HERE / 'evidence'
SHA = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
LIMITS = {'text': 8192, 'data': 4096, 'declared_stack': 512, 'image': 16384}


def run(args):
    result = subprocess.run([str(x) for x in args], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError(result.stdout)
    return result.stdout


def sections(path):
    data = path.read_bytes()
    assert data[:5] == b'\x7fELF\x01' and data[5] == 1
    shoff = struct.unpack_from('<I', data, 32)[0]
    shsize, count, names_index = struct.unpack_from('<HHH', data, 46)
    assert shsize == 40
    name_offset, name_bytes = struct.unpack_from('<II', data, shoff + names_index * 40 + 16)
    names = data[name_offset:name_offset + name_bytes]
    result = {}
    for i in range(count):
        fields = struct.unpack_from('<10I', data, shoff + i * 40)
        start = fields[0]
        name = names[start:names.index(0, start)].decode('ascii')
        result[name] = {'bytes': fields[5], 'flags': fields[2], 'type': fields[1]}
    return result


def frames(path):
    result = []
    for line in path.read_text().splitlines():
        name, size, kind = line.split('\t')
        assert kind == 'static', line
        result.append({'function': name.rsplit(':', 1)[-1], 'bytes': int(size)})
    return result


def call_evidence(disasm, caller, callee, elf):
    """Resolve this pinned build's direct or literal-loaded Xtensa calls."""
    body = disasm.split('<' + caller + '>:', 1)[1].split('\n\n', 1)[0]
    direct = re.search(r'^.*call8\s+[0-9a-f]+ <' + callee + r'>.*$', body, re.M)
    if direct:
        return [direct.group(0).strip()]
    address = int(re.search(r'^([0-9a-f]+) <' + callee + r'>:', disasm, re.M).group(1), 16)
    data = elf.read_bytes()
    shoff = struct.unpack_from('<I', data, 32)[0]
    count = struct.unpack_from('<H', data, 48)[0]
    for load in re.finditer(r'^.*l32r\s+(a\d+), ([0-9a-f]+) .*$', body, re.M):
        literal = int(load[2], 16)
        value = None
        for index in range(count):
            section = struct.unpack_from('<10I', data, shoff + index * 40)
            if section[1] != 8 and section[3] <= literal and literal + 4 <= section[3] + section[5]:
                value = struct.unpack_from('<I', data, section[4] + literal - section[3])[0]
                break
        if value != address:
            continue
        # In these pinned paths the intervening argument setup does not write
        # the function-pointer register; retain every instruction for review.
        call = re.search(r'^.*callx8\s+' + load[1] + r'\s*$', body[load.end():], re.M)
        assert call is not None
        between = body[load.end():load.end() + call.start()]
        assert not re.search(r'\t\w+\s+' + load[1] + r',', between), between
        return [s.strip() for s in body[load.start():load.end() + call.end()].splitlines()]
    raise AssertionError('Missing resolved call: ' + caller + ' -> ' + callee)


def linked_build(module, source, name):
    error = None
    package = None
    package_path = BUILD / (name + '.vdpl')
    package_path.unlink(missing_ok=True)
    try:
        package = module.build_one(source, BUILD, COMPILER, name, 1, 0x474c3730, 1)
    except ValueError as exc:
        error = str(exc)
        assert not package_path.exists()
    elf = BUILD / (name + '.elf')
    sec = sections(elf)
    stripped = BUILD / (name + '.upload.elf')
    result = {'resident_text_bytes': (sec['.text']['bytes'] + 3) & ~3,
        'resident_data_bytes': sum((sec.get(n, {}).get('bytes', 0) + 3) & ~3
                                  for n in ('.rodata', '.data', '.bss', '.data.rel.ro')),
        'would_be_image_bytes': 64 + stripped.stat().st_size, 'frames': frames(BUILD / (name + '.su')),
        'relative_relocations': sec.get('.rela.dyn', {}).get('bytes', 0) // 12,
        'elf_sha256': SHA(elf), 'stripped_elf_sha256': SHA(stripped),
        'sdk_package_error': error, 'sdk_admitted': error is None,
        'allocated_sections': {n: v for n, v in sec.items() if v['flags'] & 2}}
    relocations = run([str(COMPILER).removesuffix('gcc') + 'readelf', '-Wr', stripped])
    assert '.rela.plt' not in sec, 'Only one public SDK dispatcher is allowed; no PLT calls.'
    assert all('R_XTENSA_RELATIVE' in line and line.split()[1] == '00000005'
               for line in relocations.splitlines() if 'R_XTENSA_' in line)
    result['relocation_check'] = 'only symbol-zero R_XTENSA_RELATIVE; no PLT section'
    if package is not None:
        result['sdk_package'] = {k: v for k, v in package.items() if k not in ('source', 'package', 'elf')}
        run([SDK / 'build/check_image', BUILD / (name + '.vdpl'), result['relative_relocations']])
        result['sdk_image_validator'] = 'pass (structure/relocations; not stack or execution qualification)'
    for suffix in ('readelf.txt', 'disassembly.txt'):
        (EVIDENCE / (name + '.' + suffix)).write_bytes((BUILD / (name + '.' + suffix)).read_bytes())
    return result


def main():
    BUILD.mkdir(exist_ok=True)
    EVIDENCE.mkdir(exist_ok=True)
    pin = json.loads((HERE / 'helper-pin.json').read_text())
    for name, digest in pin['copied_sources'].items():
        assert SHA(HERE / 'helper' / name) == digest, name
    assert SHA(HERE / 'helper' / 'rally_render.h') == '2adcc445e416f3dd95db2f52afb7ef8803c67e6fc444556be5e9d1505b133e50'
    builder = SDK / 'tools/build_module.py'
    assert SHA(builder) == pin['sdk_builder_sha256']
    assert SHA(SDK / 'include/vdplet.h') == pin['sdk_header_sha256']
    spec = importlib.util.spec_from_file_location('sdk_builder', builder)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    version = run([COMPILER, '--version']).splitlines()[0]
    assert '8.4.0' in version and 'esp-2021r2-patch5' in version
    flags = ['-I', SDK / 'include', '-Os', '-fPIC', '-mlongcalls', '-mtext-section-literals',
             '-fvisibility=hidden', '-ffunction-sections', '-fdata-sections', '-fno-builtin',
             '-ffp-contract=off', '-fstack-usage', '-fno-common']
    # A separate object exposes helper frames without entry/glue inlining.
    run([COMPILER, *flags, '-DVDPL_VARIANT=1', '-c', HERE / 'helper/rally_render.c', '-o', BUILD / 'helper.o'])
    helper_sections = sections(BUILD / 'helper.o')
    helper_frames = frames(BUILD / 'helper.su')
    prefix = str(COMPILER).removesuffix('gcc')
    helper_disassembly = run([prefix + 'objdump', '-dr', BUILD / 'helper.o'])
    (EVIDENCE / 'helper-disassembly.txt').write_text(helper_disassembly)
    result = {'schema': 1, 'scope': 'resource gate only; no generated binding or execution qualification',
              'compiler': version, 'compiler_sha256': SHA(COMPILER),
              'measurement_script_sha256': SHA(HERE / 'measure_fit.py'),
              'compile_flags': [str(f) for f in flags],
              'host_compiler': run(['g++', '--version']).splitlines()[0],
              'tool_sha256': {name: SHA(Path(prefix + name)) for name in ('ld', 'strip', 'objdump', 'readelf')},
              'sdk_builder_sha256': SHA(builder), 'sdk_header_sha256': SHA(SDK / 'include/vdplet.h'),
              'dispatcher_sha256': SHA(HERE / 'fit_dispatch.c'), 'helper_pin': pin,
              'limits_unchanged': LIMITS,
              'helper_object': {'allocated_sections': {n: v for n, v in helper_sections.items() if v['flags'] & 2},
                                'frames': helper_frames, 'object_sha256': SHA(BUILD / 'helper.o')},
              'dispatcher': {'includes': 'unchanged helper in one translation unit',
                             'omits': ['Golem source execution', 'argument/root leases', 'typed marshaling',
                                       'retained completion publication', 'root success/error handling']}}
    result['dispatcher'].update(linked_build(module, HERE / 'fit_dispatch.c', 'fit-dispatch'))
    # Confirm an actual linked call path, rather than adding unrelated frames.
    disasm = (EVIDENCE / 'fit-dispatch.disassembly.txt').read_text()
    prepare = disasm.split('<rally_render_prepare_v1>:', 1)[1].split('<rally_render_stop_v1>:', 1)[0]
    preflight = disasm.split('<preflight>:', 1)[1].split('<rally_render_bind_v1>:', 1)[0]
    assert re.search(r'entry\s+a1, 0x160', prepare)
    assert re.search(r'entry\s+a1, 128', preflight)
    edges = [{'caller': caller, 'callee': callee,
              'instructions': call_evidence(disasm, caller, callee, BUILD / 'fit-dispatch.elf')}
             for caller, callee in [('vdplet_entry', 'rally_render_prepare_v1'),
                                    ('rally_render_prepare_v1', 'preflight'), ('preflight', 'acquire')]]
    frame_map = {f['function']: f['bytes'] for f in result['dispatcher']['frames']}
    path = ['vdplet_entry', 'rally_render_prepare_v1', 'preflight', 'acquire']
    minimum = sum(frame_map[n] for n in path)
    result['dispatcher']['stack_path'] = {'functions': path, 'module_frame_bytes': minimum,
        'excludes': 'firmware caller and service callback frames; no runtime high-water measurement',
        'within_declared_stack': minimum <= LIMITS['declared_stack'],
        'evidence': 'Xtensa entry instructions, call8/callx8 and resolved literal targets; preserved disassembly',
        'call_edges': edges}
    root = HERE.parents[2]
    for path, digest in pin['golem_source_inputs'].items():
        assert SHA(root / path) == digest, path
    compiler = BUILD / 'golem-fit-compiler'
    run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-O2',
         root / 'docs/tasks/GOLEM-036/right-shift/compiler.cpp', '-o', compiler])
    run([compiler, HERE / 'fit_source.golem', BUILD / 'generated'])
    generated = (BUILD / 'generated/native.c').read_text()
    assert generated.count('visibility("default")') == 1
    (BUILD / 'fit-golem.c').write_text(generated.replace('visibility("default")', 'visibility("hidden")'))
    result['combined'] = linked_build(module, HERE / 'fit_combined.c', 'fit-combined')
    result['combined'].update({'source_sha256': SHA(HERE / 'fit_source.golem'),
        'composition_source_sha256': SHA(HERE / 'fit_combined.c'),
        'generated_native_sha256': SHA(BUILD / 'generated/native.c'),
        'internalized_native_sha256': SHA(BUILD / 'fit-golem.c'),
        'internalization': 'only generated entry visibility changes default to hidden; one public SDK dispatcher',
        'generated_root': json.loads((BUILD / 'generated/root.json').read_text()),
        'omits': result['dispatcher']['omits'][1:],
        'scope': 'real generated C plus helper; measurement-only sequence, not accepted execution/publication contract'})
    combined_disasm = (EVIDENCE / 'fit-combined.disassembly.txt').read_text()
    combined_path = ['vdplet_entry', 'helper_fit_entry', 'rally_render_prepare_v1', 'preflight', 'acquire']
    combined_frames = {f['function']: f['bytes'] for f in result['combined']['frames']}
    result['combined']['stack_path'] = {'functions': combined_path,
        'module_frame_bytes': sum(combined_frames[f] for f in combined_path),
        'excludes': 'firmware caller and service callback frames; Golem returns before helper in this measurement fixture',
        'call_edges': [{'caller': a, 'callee': b,
                        'instructions': call_evidence(combined_disasm, a, b, BUILD / 'fit-combined.elf')}
                       for a, b in zip(combined_path, combined_path[1:])]}
    for name in ('native.c', 'root.json', 'interface.json'):
        (EVIDENCE / ('generated-' + name)).write_bytes((BUILD / 'generated' / name).read_bytes())
    assert not result['combined']['sdk_admitted'] or not result['dispatcher']['stack_path']['within_declared_stack']
    result['outcome'] = 'resource failure; return to Author before implementing bindings'
    (EVIDENCE / 'fit.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: result['dispatcher'][k] for k in ('resident_text_bytes', 'resident_data_bytes',
          'would_be_image_bytes', 'sdk_package_error')}, indent=2))
    print('Minimum measured dispatcher/helper stack path:', minimum)
    print('Combined text/data/image:', result['combined']['resident_text_bytes'],
          result['combined']['resident_data_bytes'], result['combined']['would_be_image_bytes'])


if __name__ == '__main__':
    main()
