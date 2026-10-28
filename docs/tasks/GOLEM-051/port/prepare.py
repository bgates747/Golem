#!/usr/bin/env python3
"""Snapshot current maintained compiler; supply only a platform library adapter."""
from pathlib import Path
import hashlib
import json
import shutil
import re

HERE = Path(__file__).resolve().parent
TASK = HERE.parent
ROOT = TASK.parents[2]

HEADERS = 'algorithm array filesystem fstream functional iomanip iostream iterator limits map memory optional set sstream stdexcept string vector cmath cerrno cctype cstdint cstdlib cstring utility'.split()

TOKEN = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|\b[A-Za-z_]\w*\b|\s+|.',re.S)

def adapt(text):
    """Mechanical recovery lowering only; parser/backend logic is retained."""
    tokens=[m for m in TOKEN.finditer(text) if not m.group().isspace() and not m.group().startswith(('//','/*'))]
    changes=[]
    for i,t in enumerate(tokens):
        if t.group()=='double':changes.append((t.start(),t.end(),'ag::Real'))
        if t.group() in ('cout','cerr'):changes.append((t.start(),t.end(),'out_stream()' if t.group()=='cout' else 'err_stream()'))
        if t.group()=='throw':
            j=i+1;depth=0
            while j<len(tokens):
                s=tokens[j].group()
                if s==';' and depth==0:break
                if s in ('(','{','['):depth+=1
                elif s in (')','}',']'):depth-=1
                j+=1
            expression=text[t.end():tokens[j].start()].strip()
            changes.append((t.start(),tokens[j].start(),'ag::raise('+expression+')' if expression else 'ag::rethrow()'))
        if t.group()!='try':continue
        j=i+1;assert tokens[j].group()=='{';depth=1;j+=1
        while depth:
            if tokens[j].group()=='{':depth+=1
            elif tokens[j].group()=='}':depth-=1
            j+=1
        assert tokens[j].group()=='catch';first=j;j+=2;start=j
        while tokens[j].group()!=')':j+=1
        declaration=text[tokens[start].start():tokens[j].start()].strip()
        match=re.fullmatch(r'(?:const\s+)?([\w:]+)\s*(?:&\s*)?(\w+)?',declaration)
        assert match,declaration
        kind,var=match.groups()
        changes.append((t.start(),t.end(),f'if (ag::Frame _ag_frame(ag::filter<{kind}>()); setjmp(_ag_frame.env)==0)'))
        changes.append((tokens[first].start(),tokens[j].end(),'else'))
        if var:
            brace=tokens[j+1];assert brace.group()=='{'
            changes.append((brace.end(),brace.end(),f' auto &{var}=ag::caught<{kind}>(); '))
    for start,end,value in sorted(changes,reverse=True):text=text[:start]+value+text[end:]
    return '#include "recovery.hpp"\n'+text

def worker(text,profile):
    """Select an existing CLI branch to leave RAM for its working data."""
    for flag in ('--stock-matrix','--resident'):
        if profile==flag[2:]:continue
        marker='        if (argc >= 2 && std::string(argv[1]) == "'+flag+'") {'
        start=text.index(marker);opening=text.index('{',start)
        ts=[m for m in TOKEN.finditer(text,opening) if not m.group().isspace() and not m.group().startswith(('//','/*'))]
        depth=0
        for token in ts:
            if token.group()=='{':depth+=1
            elif token.group()=='}':
                depth-=1
                if not depth:
                    text=text[:start]+text[token.end():];break
    if profile!='portable':
        text=text.replace('        if (argc >= 2 && std::string(argv[1]) == "--portable") return golem::portable_cli::run(argc, argv);','')
    if profile!='raw':
        start=text.index('        auto bytes = compile(readFile(argv[1]));')
        end=text.index('    } catch',start)
        text=text[:start]+'        return 2;\n'+text[end:]
    return text

def prepare(profile=None):
    out = TASK / 'build/port'
    source = out / (profile or 'full') / 'source'
    source.mkdir(parents=True, exist_ok=True)
    pins = {}
    for path in sorted((ROOT/'src').rglob('*')):
        if path.suffix not in ('.hpp','.cpp'):
            continue
        rel = path.relative_to(ROOT/'src')
        target = source / rel
        target.parent.mkdir(parents=True,exist_ok=True)
        text=path.read_text()
        # MOS has 24-bit unsigned int. Descriptor extents are explicitly 32-bit
        # on the wire; preserve the desktop parser's range and serialization.
        if str(rel)=='portable/root.hpp':
            text=text.replace('struct Binding {unsigned role=0,id=0,offset=0,bytes=0,access=0;};',
                              'struct Binding {unsigned role=0,id=0;uint32_t offset=0,bytes=0;unsigned access=0;};')
        if str(rel)=='portable/driver.hpp':
            text=text.replace('writeFile(path, std::string(bytes.begin(), bytes.end()));','''std::ofstream out(path,std::ios::binary);
    if(!out || !out.write((const char*)bytes.data(),bytes.size()))throw std::runtime_error("could not write "+path.string());
    out.close();if(!out)throw std::runtime_error("could not close "+path.string());''')

            text=text.replace('inline unsigned number(', 'inline uint32_t number(')
            text=text.replace('return static_cast<unsigned>(value);','return static_cast<uint32_t>(value);')
            for variable in ('first','last','role','id'):
                text=text.replace('const auto '+variable+'=number(', 'const unsigned '+variable+'=number(')
            for anchor,phase in [('auto program = portable::Parser(source).parse();','parsing'),
                                 ('buffered=BufferEmitter(program, layout, allocator).emit();','emitting stock commands'),
                                 ('envelope=root_v1::envelope(buffered,program,layout,allocator);','building stock wrappers'),
                                 ('const auto native = root_v1::native(program, layout,bindings);','emitting native C'),
                                 ('std::ostringstream meta;','writing metadata')]:
                text=text.replace(anchor,'ag::phase="'+phase+'";'+anchor)
            text=text.replace('        std::ostringstream calls;', '        allocator=golem::BufferAllocator();\n        std::ostringstream calls;')
            text=text.replace('std::ostringstream meta;', 'std::spoolstream meta;')
            text=text.replace('golem::loading::Plan loadPlan;','ag::phase="load plan";golem::loading::Plan loadPlan;')
            text=text.replace('std::filesystem::create_directories(output);','buffered.programs.clear();buffered.data.clear();buffered.ordinaryData.clear();ag::phase="creating output";std::filesystem::create_directories(output);')
            text=text.replace('auto interface = portable::interfaceJson(program, layout);','ag::phase="interface JSON";auto interface = portable::interfaceJson(program, layout);')
            text=text.replace('writeFile(output/"program.json", portable::programJson(program));','ag::phase="program JSON";writeFile(output/"program.json", portable::programJson(program));')

            text=text.replace('numbers(meta, std::vector<unsigned>(item.second.begin(), item.second.end()));',
                '''meta << '[';size_t byteIndex=0;
            for(auto byte:item.second){if(byteIndex++)meta << ',';meta << unsigned(byte);}meta << ']';''')

            text=text.replace('writeFile(output/"backends.json", meta.str());','meta.save(output/"backends.json");')
            # Finish and release stock data before the native C formatter's
            # temporary strings. Both outputs retain the existing bytes.
            text=text.replace('if(!bufferedOnly)native=root_v1::native(program, layout,bindings);','')
            text=text.replace('writeFile(output/"native.c", native);','')
            text=text.replace('if(emitLoadPlan)loadPlan.write(output);','''if(emitLoadPlan)loadPlan.write(output);
        const auto publishedInstallBytes=buffered.install.size();
        buffered=Buffered{};nativeRecords.clear();loadPlan=golem::loading::Plan{};
        ag::phase="emitting native C";
        if(!bufferedOnly)writeFile(output/"native.c",root_v1::native(program,layout,bindings));''')
            text=text.replace('buffered.install.size() << " bytes; "','publishedInstallBytes << " bytes; "')

        if str(rel)=='portable/backends.hpp':
            text=text.replace('return output_;','return std::move(output_);')
        if str(rel)=='stock_matrix.hpp':
            text=text.replace('syntax::Reader r(source);','ag::phase="parsing matrices";syntax::Reader r(source);')
            text=text.replace('    for(auto &p:out.entries)out.blocks.emplace', '    ag::phase="emitting matrices";for(auto &p:out.entries)out.blocks.emplace')
            text=text.replace('    // Matrix IDs have no command blocks:',
                              '    for(auto &entry:out.entries)entry.ops=std::vector<Operation>();\n    // Matrix IDs have no command blocks:')
        if profile and rel.name=='golemc.cpp':text=worker(text,profile)
        if rel.name=='golemc.cpp' and profile in (None,'raw'):
            text=text.replace('auto bytes = compile(readFile(argv[1]));', '''golem::BufferAllocator rawAllocation;
        Compiler rawCompiler(rawAllocation);
        Reader rawReader(readFile(argv[1]));
        compileBlock(rawCompiler,rawReader,rawCompiler.out);
        if(!rawReader.end()) rawReader.error("unexpected Next with no matching For");
        auto &bytes = rawCompiler.out;''')
            text=text.replace('write(argv[2], bytes);','''write(argv[2], bytes);
        golem::resident_scalar::Bytes rawCleanup;
        for(const auto &claim:rawAllocation.claims()) if(!claim.second.external) {
            emitBufferHeader(rawCleanup,claim.first);rawCleanup.push_back(2);
        }
        write(std::string(argv[2])+".cleanup.bin",rawCleanup);''')
        text=adapt(text)
        if rel.name=='golemc.cpp':text=text.replace('int main(', 'int golemc_entry(')
        target.write_text(text)
        pins[str(rel)] = hashlib.sha256(path.read_bytes()).hexdigest()
    shim=out/'include';shim.mkdir(exist_ok=True)
    for name in HEADERS:
        (shim/name).write_text('#pragma once\n#include "mini.hpp"\n')
    shutil.copyfile(HERE/'mini.hpp',shim/'mini.hpp')
    shutil.copyfile(HERE/'basics.hpp',shim/'basics.hpp')
    shutil.copyfile(HERE/'recovery.hpp',shim/'recovery.hpp')
    shutil.copyfile(HERE/'real.hpp',shim/'real.hpp')
    (out/'source-pins.json').write_text(json.dumps(pins,indent=2)+'\n')
    return out,source

if __name__=='__main__':
    print(prepare())
