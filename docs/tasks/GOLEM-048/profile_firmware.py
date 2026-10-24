#!/usr/bin/env python3
"""Task-only counters in an owned firmware copy; no production ABI changes."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess

HERE = Path(__file__).resolve().parent
SDK = HERE.parents[2].parent / "agon-vdplet"
NAMES = ["queue_calls", "queue_cycles", "line_calls", "line_cycles",
         "state_calls", "state_cycles", "trace_calls", "trace_cycles",
         "wait_calls", "wait_cycles", "submit_calls", "submit_cycles",
         "entry_cycles", "entry_id"]


def patch(path, old, new):
    text = path.read_text()
    assert text.count(old) == 1, (path, old)
    path.write_text(text.replace(old, new))


def main(trace, tag):
    prefix = (tag + "-" if tag else "") + ("profile-on" if trace else "profile-off")
    source = HERE / "build" / prefix
    original = SDK / "build/firmware/firmware-src"
    if source.exists():
        raise ValueError("Choose a fresh diagnostic output; existing evidence is preserved")
    subprocess.run(["cp", "-a", "--reflink=auto", str(original), str(source)], check=True)
    video = source / "video"
    header = video / "vdpl_bench.h"
    header.write_text('''#pragma once
#include <stdint.h>
extern "C" {
extern volatile uint32_t vp_data[14];
extern bool vp_active;
}
static inline uint32_t vp_cycle() {
    uint32_t n; __asm__ __volatile__("rsr %0,ccount" : "=a"(n)); return n;
}
''')
    include = '#include "' + str(header) + '"\n'
    ino = video / "video.ino"
    ino.write_text(include + 'extern "C" { volatile uint32_t vp_data[14]={}; bool vp_active=false; }\n' + ino.read_text())
    display = source / ".pio/libdeps/esp32dev/vdp-gl/src/displaycontroller.cpp"
    display.write_text(include + display.read_text())
    patch(display, '    xQueueSendToBack(m_execQueue, &primitive, portMAX_DELAY);',
          '''    auto vp_start=vp_cycle();
    xQueueSendToBack(m_execQueue, &primitive, portMAX_DELAY);
    if(vp_active){++vp_data[0];vp_data[1]+=vp_cycle()-vp_start;}''')
    canvas = source / ".pio/libdeps/esp32dev/vdp-gl/src/canvas.cpp"
    canvas.write_text(include + canvas.read_text())
    patch(canvas, 'void Canvas::waitCompletion(bool waitVSync)\n{',
          'void Canvas::waitCompletion(bool waitVSync)\n{\n  auto vp_start=vp_cycle();')
    patch(canvas, '    m_displayController->processPrimitives();         // process right now!\n  }\n}',
          '''    m_displayController->processPrimitives();         // process right now!
  }
  if(vp_active){++vp_data[8];vp_data[9]+=vp_cycle()-vp_start;}
}''')
    vga = source / ".pio/libdeps/esp32dev/vdp-gl/src/dispdrivers/vgabasecontroller.cpp"
    vga.write_text(include + vga.read_text())
    patch(vga, '        ctrl->execPrimitive(prim, updateRect, false);',
          '''        auto vp_start=vp_cycle();
        ctrl->execPrimitive(prim, updateRect, false);
        if(vp_active){unsigned at=prim.cmd==PrimitiveCmd::LineTo?2:4;
          ++vp_data[at];vp_data[at+1]+=vp_cycle()-vp_start;}''')
    vdu = video / "vdu.h"
    vdu.write_text(include + vdu.read_text())
    patch(vdu, 'if (consoleMode) {\n\t\tDBGSerial.write(c);\n\t}',
          '''if (consoleMode) {
        auto vp_start=vp_cycle();DBGSerial.write(c);
        if(vp_active){++vp_data[6];vp_data[7]+=vp_cycle()-vp_start;}
    }''')
    runtime = video / "vdplet_runtime.h"
    runtime.write_text(include + runtime.read_text())
    call = '    int32_t result=function(services,&request);' if 'function(services,&request)' in runtime.read_text() else '    int32_t result=function(state.image.abi_minor ? &host_vdu.base : &host,&request);'
    patch(runtime, call,
          f'''    if(operation==VDPL_RUN && (entry==1 || entry==2)){{
        canvas->waitCompletion(true);vp_active=false;
        for(auto &n:vp_data)n=0;vp_data[13]=entry;
        consoleMode={'true' if trace else 'false'};vp_active=true;
    }}
    auto vp_start=vp_cycle();
{call}
    if(operation==VDPL_RUN && (entry==1 || entry==2))vp_data[12]=vp_cycle()-vp_start;''')
    patch(runtime, '    while (span.available()) vdu(readByte(),false);',
          '''    auto vp_start=vp_cycle();
    while (span.available()) vdu(readByte(),false);
    if(vp_active){++vp_data[10];vp_data[11]+=vp_cycle()-vp_start;}''')
    if '    context->vdpletLines(lines,count);' in runtime.read_text():
        patch(runtime, '    context->vdpletLines(lines,count);',
              '''    auto vp_start=vp_cycle();
    context->vdpletLines(lines,count);
    if(vp_active){++vp_data[10];vp_data[11]+=vp_cycle()-vp_start;}''')
        # Deliberately leave Context's path pending, without vdu_plot lookahead,
        # for a differential move/line versus native-batch transition test.
        patch(runtime, 'else { rc=vdpl_runtime::info(vdpl::u16(payload+6),value); kind=VDPL_REPLY_VALUE; }',
              '''else {
                if(vdpl::u16(payload+6)==0x7f00){
                    context->plot(30,30,4);context->plot(90,30,0xdd);
                    context->plot(90,90,0xdd);context->plot(30,90,0xdd);value=0;
                }else rc=vdpl_runtime::info(vdpl::u16(payload+6),value);
                kind=VDPL_REPLY_VALUE;
            }''')
    patch(runtime, '    if (flags&VDPL_REPLY) reply(kind,token,value);',
          '''    if(operation==VDPL_INFO && length==8 && vdpl::u16(payload+6)==3 && vp_active){
        vp_active=false;
        force_debug_log("VP %u %u %u %u %u %u %u %u %u %u %u %u %u %u\\n",
          vp_data[0],vp_data[1],vp_data[2],vp_data[3],vp_data[4],vp_data[5],
          vp_data[6],vp_data[7],vp_data[8],vp_data[9],vp_data[10],vp_data[11],vp_data[12],vp_data[13]);
        consoleMode=true;
    }
    if (flags&VDPL_REPLY) reply(kind,token,value);''')
    out = HERE / "build" / (prefix + "-build.log")
    with out.open("w") as log:
        subprocess.run([str(SDK / ".venv/bin/python"), "-m", "platformio", "run", "-d", str(source),
                        "-e", "esp32dev", "-j", "2"], stdout=log, stderr=subprocess.STDOUT, check=True)
    artifacts = source / ".pio/build/esp32dev"
    receipt = {"diagnostic_only": True, "trace_during_native_draw": trace,
               "counter_names": NAMES, "cycle_units": "local-core CCOUNT; nominal240MHz; emulation only",
               "overlap": "queue/submit/entry intervals overlap raster on the other core; do not add",
               "firmware": {n: {"bytes": (artifacts/n).stat().st_size,
                                 "sha256": hashlib.sha256((artifacts/n).read_bytes()).hexdigest()}
                            for n in ("firmware.bin", "firmware.elf")}}
    (HERE / (prefix + "-firmware.json")).write_text(json.dumps(receipt, indent=2)+"\n")
    print("Task-only profiling firmware built:", source)


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--trace", choices=("on", "off"), default="on")
    p.add_argument("--tag", default="", choices=("", "lines"))
    a=p.parse_args();main(a.trace == "on",a.tag)
