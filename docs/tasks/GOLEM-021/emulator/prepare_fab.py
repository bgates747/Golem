#!/usr/bin/env python3
"""Owned Fab1.2.5 frontend with optional native-frame pacing and clean shutdown."""
from __future__ import annotations
import argparse
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tarfile
PIN='c22876d1f7ad903537c308cf65afd6553b37cdb8'
here=Path(__file__).resolve().parent
p=argparse.ArgumentParser()
p.add_argument('--reference',type=Path,default=Path('/Agon/fab-agon-emulator'))
p.add_argument('--source',type=Path,default=here/'build/fab-source')
a=p.parse_args()
marker=a.source/'.golem-fab-source.json'
if not marker.exists():
    if a.source.exists() and any(a.source.iterdir()):raise SystemExit('Refusing nonempty unmarked source export')
    a.source.mkdir(parents=True,exist_ok=True)
    data=subprocess.check_output(['git','-C',str(a.reference),'archive',PIN])
    with tarfile.open(fileobj=io.BytesIO(data)) as tar:tar.extractall(a.source,filter='data')
    marker.write_text(json.dumps({'upstream_commit':PIN,'patch_revision':0},indent=2)+'\n')
identity=json.loads(marker.read_text())
if identity['upstream_commit']!=PIN:raise SystemExit('Unexpected Fab source identity')
if identity['patch_revision']==0:
    def replace(file,old,new):
        path=a.source/file;text=path.read_text()
        if text.count(old)!=1:raise SystemExit(f'Patch context differs: {file}')
        path.write_text(text.replace(old,new))
    replace('src/vdp_interface.rs',
'''pub struct VdpInterface {
''',
'''pub struct VdpInterface {
    pub flexe_vdp_frames: Option<libloading::Symbol<'static, unsafe extern "C" fn() -> u64>>,
    pub flexe_vdp_failed: Option<libloading::Symbol<'static, unsafe extern "C" fn() -> bool>>,
''')
    replace('src/vdp_interface.rs',
'''            return VdpInterface {
''',
'''            return VdpInterface {
                flexe_vdp_frames: lib.get(b"flexe_vdp_frames").ok(),
                flexe_vdp_failed: lib.get(b"flexe_vdp_failed").ok(),
''')
    replace('src/main.rs',
'''const PREFIX: Option<&'static str> = option_env!("PREFIX");''',
'''const PREFIX: Option<&'static str> = option_env!("PREFIX");

static HOST_SHUTDOWN: std::sync::atomic::AtomicBool =
    std::sync::atomic::AtomicBool::new(false);
extern "C" fn request_host_shutdown(_: i32) {
    HOST_SHUTDOWN.store(true, std::sync::atomic::Ordering::Relaxed);
}
#[cfg(unix)]
extern "C" {
    fn signal(number: i32, handler: extern "C" fn(i32)) -> usize;
}''')
    replace('src/main.rs',
'''pub fn main_loop() -> i32 {
''',
'''pub fn main_loop() -> i32 {
    #[cfg(unix)] unsafe {
        signal(2, request_host_shutdown);
        signal(15, request_host_shutdown);
    }
''')
    replace('src/main.rs',
'''    let _vdp_thread = thread::Builder::new()''',
'''    let hybrid_backend = vdp_interface.flexe_vdp_frames.is_some();
    let mut last_native_frame = 0u64;
    let vdp_thread = thread::Builder::new()''')
    replace('src/main.rs',
'''&format!("Fab Agon Emulator {}", env!("CARGO_PKG_VERSION")),''',
'''&format!("{} {}", if hybrid_backend { "Fab/Flexe Agon Emulator" } else { "Fab Agon Emulator" }, env!("CARGO_PKG_VERSION")),''')
    replace('src/main.rs',
'''            // signal vsync to ez80 via GPIO (pin 1 (from 0) of GPIO port B)
            {
''',
'''            // Hybrid ESP32 backends own physical scanout timing. Presenting
            // an unchanged frame must not generate another eZ80 VSync.
            let native_frame_changed = if let Some(ref frames) = vdp_interface.flexe_vdp_frames {
                let now = unsafe { (**frames)() };
                let changed = now != last_native_frame;
                last_native_frame = now;
                changed
            } else { true };
            // signal vsync to ez80 via GPIO (pin 1 (from 0) of GPIO port B)
            if native_frame_changed {
''')
    replace('src/main.rs',
'''            if emulator_shutdown.load(std::sync::atomic::Ordering::Relaxed) {
''',
'''            if HOST_SHUTDOWN.load(std::sync::atomic::Ordering::Relaxed)
                || emulator_shutdown.load(std::sync::atomic::Ordering::Relaxed) {
''')
    replace('src/main.rs',
'''            for event in event_pump.poll_iter() {
''',
'''            if let Some(ref failed) = vdp_interface.flexe_vdp_failed {
                if unsafe { (**failed)() } {
                    exit_status.store(2, std::sync::atomic::Ordering::Relaxed);
                    break 'running;
                }
            }
            for event in event_pump.poll_iter() {
''')
    replace('src/main.rs',
'''    std::thread::sleep(std::time::Duration::from_millis(200));''',
'''    if hybrid_backend {
        if let Ok(worker) = vdp_thread { let _ = worker.join(); }
    } else {
        std::thread::sleep(std::time::Duration::from_millis(200));
    }''')
    identity['patch_revision']=1
    identity['patch_files']={f:hashlib.sha256((a.source/f).read_bytes()).hexdigest() for f in ['src/main.rs','src/vdp_interface.rs']}
    marker.write_text(json.dumps(identity,indent=2)+'\n')
elif identity['patch_revision']!=1:raise SystemExit('Unknown frontend patch revision')
for f,digest in identity['patch_files'].items():
    if hashlib.sha256((a.source/f).read_bytes()).hexdigest()!=digest:raise SystemExit(f'Frontend source changed: {f}')
print(json.dumps(identity,indent=2))
