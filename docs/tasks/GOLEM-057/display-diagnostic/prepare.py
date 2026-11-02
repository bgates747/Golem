#!/usr/bin/env python3
"""Create isolated original010 control/diagnostic copies; no build or run."""
from pathlib import Path
import difflib
import gzip
import hashlib
import json
import shutil

HERE = Path(__file__).resolve().parent
TASK = HERE.parent
SDK = Path('/Agon/mystuff/agon-vdplet')
ORIGINAL = SDK / 'build/vdpl-010/pixels/firmware-src'
BUILD = TASK / 'build/display-origin-diagnostic'
GL = Path('.pio/libdeps/esp32dev/vdp-gl/src')
FILES = [Path('video/vdu.h'), Path('video/video.ino'), GL / 'dispdrivers/vgapalettedcontroller.cpp']
PINS = {
    Path('video/vdu.h'): '24923b53a83d77ebf729795d7ecb7343e6f2fed0e9d748d48fc83c8b75e805cb',
    GL / 'dispdrivers/vgapalettedcontroller.cpp': '5c8f9b6b673d917ed7052a39aaef1f9671b669563f379c40337e3d1a829c2f49',
    Path('platformio.ini'): '7947cbd2e5fcb38e12ca3b8dee44b97700da0ca1d05c19914b100d47885f0bb1',
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def replace_once(text, old, new):
    assert text.count(old) == 1, (old[:120], text.count(old))
    return text.replace(old, new, 1)


def patches():
    text = (ORIGINAL / FILES[0]).read_text()
    text = replace_once(text, '#include <HardwareSerial.h>', '#include <HardwareSerial.h>\n#include <g057_diag.h>')
    text = replace_once(text, '\tauto errVal = changeMode(mode);', '''#if G057_DIAG_ENABLED
    g057_begin(mode, videoMode);
    g057_attempt(mode, G057_REQUESTED);
#endif
\tauto errVal = changeMode(mode);''')
    text = replace_once(text, '\t\terrVal = changeMode(videoMode);', '''#if G057_DIAG_ENABLED
        g057_attempt(videoMode, G057_RESTORE_PREVIOUS);
#endif
\t\terrVal = changeMode(videoMode);''')
    text = replace_once(text, '\t\t\tchangeMode(1);', '''#if G057_DIAG_ENABLED
            g057_attempt(1, G057_FINAL_FALLBACK);
#endif
\t\t\tchangeMode(1);''')
    text = replace_once(text, '\t// reset our context, and clear the context stack', '''#if G057_DIAG_ENABLED
    g057_finish();
#endif
\t// reset our context, and clear the context stack''')
    result = {FILES[0]: text}
    text = (ORIGINAL / FILES[1]).read_text()
    text = replace_once(text, '#include <fabgl.h>', '#include <fabgl.h>\n#include <g057_diag.h>')
    text = replace_once(text, 'HardwareSerial\tDBGSerial(0);', '''HardwareSerial\tDBGSerial(0);

#if G057_DIAG_ENABLED
extern "C" void g057_uart_emit(const uint8_t * bytes, size_t length) {
    DBGSerial.write(bytes, length);
    DBGSerial.flush();
}
#endif''')
    result[FILES[1]] = text
    text = (ORIGINAL / FILES[2]).read_text()
    text = replace_once(text, '#include "vgapalettedcontroller.h"', '''#include "vgapalettedcontroller.h"
#define G057_DIAG_IMPLEMENTATION
#include "g057_diag.h"''')
    text = replace_once(text, '  if (m_signalTableSize == 0) {\n    return false;\n  }\n  if (m_signalMaps.find(paletteId) == m_signalMaps.end()) {\n    m_signalMaps[paletteId] = heap_caps_malloc(m_signalTableSize, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);', '''#if G057_DIAG_ENABLED
  const bool diagLookup = m_nativePixelFormat == NativePixelFormat::PALETTE2 ||
      m_nativePixelFormat == NativePixelFormat::PALETTE4 ||
      m_nativePixelFormat == NativePixelFormat::PALETTE8 ||
      m_nativePixelFormat == NativePixelFormat::PALETTE16;
  const auto diagFound = m_signalMaps.find(0);
  const bool diagPresent = diagFound != m_signalMaps.end();
  const void * diagExisting = diagPresent ? diagFound->second : nullptr;
  if (paletteId == 0 && (m_signalTableSize == 0 || diagPresent)) {
    auto * r = g057_record(G057_DEFAULT_PLANE, this, uint32_t(m_nativePixelFormat),
        m_signalTableSize, g057_plane_flags(diagLookup, m_signalTableSize == 0,
                                            diagPresent, diagExisting, false));
    if (r) r->w[10] = g057_ptr(diagExisting);
  }
#endif
  if (m_signalTableSize == 0) {
    return false;
  }
  if (m_signalMaps.find(paletteId) == m_signalMaps.end()) {
#if G057_DIAG_ENABLED
    void * diagAllocated = heap_caps_malloc(m_signalTableSize, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    if (paletteId == 0) {
      auto * r = g057_record(G057_DEFAULT_PLANE, this, uint32_t(m_nativePixelFormat),
          m_signalTableSize, g057_plane_flags(diagLookup, false, diagPresent,
                                              diagExisting, true));
      if (r) {
        r->w[7] = m_signalTableSize;
        r->w[8] = MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL;
        r->w[9] = g057_ptr(diagAllocated);
        r->w[10] = g057_ptr(diagExisting);
      }
    }
    m_signalMaps[paletteId] = diagAllocated;
#else
    m_signalMaps[paletteId] = heap_caps_malloc(m_signalTableSize, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
#endif''')
    text = replace_once(text, '''  auto paletteID = rawList[1];

  item->endRow''', '''  auto paletteID = rawList[1];

#if G057_DIAG_ENABLED
  if (entries == 1 && row == 0 && paletteID == 0 && g057_initial(this)) {
    const auto found = m_signalMaps.find(0);
    const bool present = found != m_signalMaps.end();
    const void * selected = present ? found->second : nullptr;
    const bool lookup = m_nativePixelFormat == NativePixelFormat::PALETTE2 ||
        m_nativePixelFormat == NativePixelFormat::PALETTE4 ||
        m_nativePixelFormat == NativePixelFormat::PALETTE8 ||
        m_nativePixelFormat == NativePixelFormat::PALETTE16;
    auto * r = g057_record(G057_INITIAL_LIST, this, uint32_t(m_nativePixelFormat),
        m_signalTableSize, G057_ALLOC_CALLED | G057_SELECTION_PEEK |
        (lookup ? G057_LOOKUP_REQUIRED : 0u) | (present ? G057_MAP_PRESENT : 0u));
    if (r) {
      r->w[7] = sizeof(PaletteListItem);
      r->w[8] = MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL;
      r->w[9] = r->w[11] = g057_ptr(item);
      r->w[10] = r->w[13] = g057_ptr(selected);
      r->w[14] = uint32_t(paletteID) | uint32_t(entries & 255) << 16 | uint32_t(row & 255) << 24;
    }
  }
#endif

  item->endRow''')
    text = replace_once(text, '''  // must be started before interrupt alloc
  startGPIOStream();''', '''#if G057_DIAG_ENABLED
  {
    const auto found = m_signalMaps.find(0);
    const bool present = found != m_signalMaps.end();
    const bool lookup = m_nativePixelFormat == NativePixelFormat::PALETTE2 ||
        m_nativePixelFormat == NativePixelFormat::PALETTE4 ||
        m_nativePixelFormat == NativePixelFormat::PALETTE8 ||
        m_nativePixelFormat == NativePixelFormat::PALETTE16;
    auto * r = g057_record(G057_PREACTIVATE, this, uint32_t(m_nativePixelFormat),
        m_signalTableSize, (lookup ? G057_LOOKUP_REQUIRED : 0u) |
                          (present ? G057_MAP_PRESENT : 0u));
    if (r) {
      r->w[10] = present ? g057_ptr(found->second) : 0;
      r->w[11] = g057_ptr(m_signalList);
      r->w[12] = g057_ptr(m_currentSignalItem);
      r->w[13] = m_currentSignalItem ? g057_ptr(m_currentSignalItem->signals) : 0;
      r->w[15] = uint32_t(m_viewPortWidth & 65535) | uint32_t(m_viewPortHeight & 65535) << 16;
    }
    g057_flush();
  }
#endif

  // must be started before interrupt alloc
  startGPIOStream();''')
    result[FILES[2]] = text
    return result


def inventory(root):
    return {str(p.relative_to(root)): sha(p) for p in sorted(root.rglob('*'))
            if p.is_file() and '.git' not in p.relative_to(root).parts}


def ignore(directory, names):
    # The original cached VCS library has no .piopm receipt: PlatformIO needs its
    # copied git metadata to recognize the installed pinned dependency offline.
    return [n for n in names if (n == '.git' and Path(directory).name != 'vdp-gl') or
            (n == 'build' and Path(directory).name == '.pio') or
            Path(n).suffix in ('.o', '.a', '.elf', '.bin', '.map')]


def main():
    assert hashlib.sha256((TASK / 'display-origin-diagnostic-proposal.md').read_bytes()).hexdigest() == json.loads((TASK / 'display-origin-diagnostic-freeze.json').read_text())['plan_sha256']
    for path, expected in PINS.items():
        assert sha(ORIGINAL / path) == expected, str(path)
    assert not BUILD.exists(), 'Refuse to overwrite a prior preparation/build'
    BUILD.mkdir(parents=True)
    changes = patches()
    manifest = {'schema': 'g057-display-diag-preparation-v1', 'original': str(ORIGINAL), 'pins': {str(p): s for p, s in PINS.items()}, 'variants': {}}
    diff = []
    header = (HERE / 'g057_diag.h').read_text()
    for relative, text in changes.items():
        diff.extend(difflib.unified_diff((ORIGINAL / relative).read_text().splitlines(True), text.splitlines(True), fromfile='a/' + str(relative), tofile='b/' + str(relative)))
    diff.extend(difflib.unified_diff([], header.splitlines(True), fromfile='/dev/null', tofile='b/' + str(GL / 'g057_diag.h')))
    (HERE / 'source.patch').write_text(''.join(diff))
    for variant, enabled in [('control', 0), ('instrumented', 1)]:
        target = BUILD / variant
        shutil.copytree(ORIGINAL, target, ignore=ignore, symlinks=True)
        original_manifest = inventory(target)
        for relative, text in changes.items():
            (target / relative).write_text(text)
        (target / GL / 'g057_diag.h').write_text(header.replace('#define G057_DIAG_ENABLED 1', f'#define G057_DIAG_ENABLED {enabled}', 1))
        copied_manifest = inventory(target)
        manifest['variants'][variant] = {'root': str(target), 'macro': enabled, 'copied_original_files': original_manifest, 'prepared_source_files': copied_manifest}
    manifest['patch_sha256'] = sha(HERE / 'source.patch')
    manifest['offline_dependency_metadata'] = 'Retain a copy of original cached vdp-gl/.git; exclude it from source-file inventory. No upstream checkout is used or changed.'
    save_manifest(manifest)
    print('Prepared original010 control and instrumented source copies; no build/run performed.')


def save_manifest(manifest):
    inventory_path = HERE / 'source-manifests.json.gz'
    inventory_path.write_bytes(gzip.compress(json.dumps(manifest, sort_keys=True).encode(), mtime=0))
    compact = {k: v for k, v in manifest.items() if k != 'variants'}
    compact['full_manifest'] = {'path': inventory_path.name, 'bytes': inventory_path.stat().st_size,
                                'sha256': sha(inventory_path)}
    compact['variants'] = {}
    for name, data in manifest['variants'].items():
        before, after = data['copied_original_files'], data['prepared_source_files']
        compact['variants'][name] = {'root': data['root'], 'macro': data['macro'],
            'original_file_count': len(before), 'prepared_file_count': len(after),
            'changes': {p: {'before': before.get(p), 'after': h} for p, h in after.items()
                        if before.get(p) != h}}
    (HERE / 'preparation.json').write_text(json.dumps(compact, indent=2) + '\n')


if __name__ == '__main__':
    main()
