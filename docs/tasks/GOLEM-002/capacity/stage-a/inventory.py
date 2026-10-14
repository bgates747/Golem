"""Inspect existing pinned artifacts. No firmware build, target or capacity run."""
from pathlib import Path
import hashlib,importlib.util,json,re,struct,subprocess

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[4]
SDK=ROOT.parent/'agon-vdplet'
FW=SDK/'build/firmware'
SOURCE=FW/'firmware-src'
EVIDENCE=HERE/'evidence'
PREFIX=str(Path.home()/'.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-')
SHA=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    artifacts=json.loads((FW/'firmware-artifacts.json').read_text())
    for row in artifacts.values():assert SHA(Path(row['path']))==row['sha256']
    receipt=json.loads((FW/'firmware-source.json').read_text())
    for name,digest in receipt['headers'].items():assert SHA(SOURCE/'video'/name)==digest
    for key,name in [('dispatch_sha256','video/vdu_sys.h'),('processor_sha256','video/vdu_stream_processor.h'),('configuration_sha256','platformio.ini')]:
        assert SHA(SOURCE/name)==receipt[key]
    elf=Path(artifacts['firmware.elf']['path'])
    disasm=subprocess.check_output([PREFIX+'objdump','-d','-C',str(elf)],text=True)
    functions={m[2]:m[3] for m in re.finditer(r'^([0-9a-f]+) <([^\n]+)>:\n(.*?)(?=\n[0-9a-f]+ <|\Z)',disasm,re.M|re.S)}
    wanted=['processLoop(void*)','VDUStreamProcessor::processNext()',
            'VDUStreamProcessor::vdu(unsigned char, bool)','VDUStreamProcessor::vdu_sys()',
            'VDUStreamProcessor::vdu_sys_buffered()','VDUStreamProcessor::bufferCall(unsigned short, AdvancedOffset)',
            'VDUStreamProcessor::processAllAvailable()','VDUStreamProcessor::vdu_sys_vdplet(bool)',
            'vdpl_runtime::call(unsigned short, unsigned short, unsigned short, unsigned int)',
            'vdpl_runtime::acquire(void*, unsigned short, unsigned short, unsigned int, unsigned int, vdplet_view_v1*)',
            'vdpl_runtime::relinquish(void*, unsigned int)']
    frames={}
    for name in wanted:
        body=functions[name];m=re.search(r'entry\s+a1, (0x[0-9a-f]+|\d+)',body)
        assert m,name
        frames[name]=int(m[1],0)
    (EVIDENCE/'firmware-selected-disassembly.txt').write_text('\n'.join(name+'\n'+functions[name] for name in wanted))
    video=(SOURCE/'video/video.ino').read_text()
    task=re.search(r'xTaskCreatePinnedToCore\(\s*processLoop,\s*"processLoop",\s*(\d+)',video)
    assert task and int(task[1])==4096
    header=Path.home()/'.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32/include/freertos/include/freertos/task.h'
    assert 'Stack size in bytes' in header.read_text()
    runtime=SDK/'src/vdplet_runtime.h'
    module=json.loads((EVIDENCE/'resources.json').read_text())
    spec=importlib.util.spec_from_file_location('fit',ROOT/'docs/tasks/GOLEM-037/measure_fit.py')
    fit=importlib.util.module_from_spec(spec);spec.loader.exec_module(fit)
    module_paths={}
    for name,row in module['layouts'].items():
        path=['vdplet_entry','rally_render_prepare_v1','preflight','acquire']
        fm={f['function']:f['bytes'] for f in row['combined']['frames']}
        dump=(EVIDENCE/(name+'-combined.disassembly.txt')).read_text()
        module_paths[name]={'path':path,'module_frame_bytes':sum(fm[n] for n in path),
            'conservative_all_module_static_frames':sum(fm.values()),
            'scope':'no recursion in supplied source/helper; excludes external service/library/firmware frames; not a runtime measurement',
            'call_edges':[{'caller':a,'callee':b,'instructions':fit.call_evidence(dump,a,b,HERE/'build'/(name+'-combined.elf'))}
                          for a,b in zip(path,path[1:])]}
    # Source-derived one resident wrapper scenario, not a whole-firmware bound.
    path=[wanted[i] for i in (0,1,2,3,4,5,6,1,2,3,7,8)]
    prefix_bytes=sum(frames[n] for n in path)
    size=subprocess.check_output([PREFIX+'size','-A',str(elf)],text=True)
    section_sizes={m[1]:int(m[2]) for m in re.finditer(r'^([.\w]+)\s+(\d+)\s+\d+\s*$',size,re.M)}
    framework=Path.home()/'.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32'
    archive=framework/'lib/libheap.a'
    obj=HERE/'build/memory_layout.o'
    obj.write_bytes(subprocess.check_output([PREFIX+'ar','p',str(archive),'memory_layout.c.obj']))
    binary=obj.read_bytes();shoff=struct.unpack_from('<I',binary,32)[0]
    count,names_index=struct.unpack_from('<HH',binary,48)
    no,nb=struct.unpack_from('<II',binary,shoff+names_index*40+16);names=binary[no:no+nb]
    parts={}
    for i in range(count):
        f=struct.unpack_from('<10I',binary,shoff+i*40)
        name=names[f[0]:names.index(0,f[0])].decode()
        if name in ('.rodata.soc_memory_types','.rodata.soc_memory_regions'):parts[name]=binary[f[4]:f[4]+f[5]]
    types=parts['.rodata.soc_memory_types'];assert len(types)%20==0
    # Struct layout is confirmed by the pinned heap_memory_layout.h (pointer,
    # three u32 caps, two bools, alignment padding). Type1 has executable alias.
    caps=struct.unpack_from('<III',types,20+4)
    assert 3 in caps and types[20+16]==1
    regions=parts['.rodata.soc_memory_regions'];assert len(regions)%16==0
    aliases=[dict(zip(('start','bytes','type','iram_alias'),struct.unpack_from('<4I',regions,i)))
             for i in range(0,len(regions),16) if struct.unpack_from('<I',regions,i+8)[0]==1]
    heap_dump=subprocess.check_output([PREFIX+'objdump','-sr',str(obj)],text=True)
    (EVIDENCE/'heap-memory-layout.txt').write_text(heap_dump)
    result={'schema':1,'scope':'existing build/source inventory; no new firmware or target runs',
        'inventory_script_sha256':SHA(HERE/'inventory.py'),
        'firmware_artifact_receipt_sha256':SHA(FW/'firmware-artifacts.json'),
        'firmware_source_receipt_sha256':SHA(FW/'firmware-source.json'),
        'firmware_artifacts':artifacts,'source_receipt':receipt,
        'source_inputs':{str(p):SHA(p) for p in (SOURCE/'video/video.ino',SOURCE/'video/vdu_buffered.h',SOURCE/'video/buffers.h',header,runtime)},
        'firmware_selected_frames':frames,'command_task_stack_bytes':4096,
        'task_stack_units':'bytes, confirmed by pinned ESP-IDF FreeRTOS header; upstream source comment about words is not a capacity measurement',
        'one_wrapper_source_path':{'functions':path,'selected_live_frames':prefix_bytes,
           'scope':'source-derived scenario only; not full callgraph, unlimited nested buffer-call bound or observed high-water'},
        'firmware_allocated_sections':{k:v for k,v in section_sizes.items() if k in ('.iram0.vectors','.iram0.text','.dram0.data','.dram0.bss','.flash.text','.flash.rodata')},
        'heap_layout':{'archive_sha256':SHA(archive),'object_sha256':SHA(obj),
            'type_descriptor_header_sha256':SHA(framework/'include/heap/include/heap_memory_layout.h'),
            'soc_header_sha256':SHA(framework/'include/soc/esp32/include/soc/soc.h'),
            'd_iram_type_caps':list(caps),'aliased_iram':True,'raw_alias_regions':aliases,
            'interpretation':'executable allocations may consume aliased internal data RAM as well as dedicated IRAM; do not add alias sizes twice or infer runtime availability from the linker IRAM remainder'},
        'module_stack_paths':module_paths,
        'unmeasured':['current free/largest executable block','current free/largest internal byte-accessible block',
                      'actual command-task high-water under composition','full service/library/interrupt stack maxima',
                      'video/audio/asset allocation contention and fragmentation','physical capacity/cache/timing'],
        'transport':'stock write chunks are u16; consolidate command14 can form one larger uint32-sized block; LOAD length u32; runtime still requires exactly one block',
        'peak_load':'consolidation retains old chunks while allocating/copying final block; load holds upload while allocating separate text/data; no second module slot',
        'proposal_not_implementation':{'runtime_ceilings':{'text':65536,'data':16384,'image':131072,'declared_module_stack':2048},
            'recommended_initial_budgets':{'text':32768,'data':4096,'image':65536,'declared_module_stack':2048},
            'current_scaffold_declarations_rounded':{'text':28672,'data':2048,'image_requirement':28184,'declared_module_stack':2048},
            'task_stack':4096,'firmware_service_reserve':2048,
            'qualification_gate':'real D2 static paths must fit declared module stack, full firmware/services must fit reserve; confirm >=1024 bytes command-task high-water in selected configurations before expanding stress; unknown paths remain unqualified'}}
    (EVIDENCE/'inventory.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'module_paths':module_paths,'known_one_wrapper_prefix_bytes':prefix_bytes,
                      'firmware_selected_frames':frames},indent=2))

if __name__=='__main__':main()
