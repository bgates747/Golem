#!/usr/bin/env python3
"""Capture real source inputs exercised by maintained tests, without editing them."""
from pathlib import Path
import json
import shutil
import subprocess

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
OUT=HERE/'build/capture'

TRACE=r'''
#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>
namespace capture051 {
inline void record(const std::string&mode,const std::string&source,const std::vector<std::pair<unsigned,unsigned>>&ranges={},const std::string&table="",const std::vector<unsigned char>&data={}){
 std::string key=mode+"|"+source+"|"+table;
 for(auto r:ranges)key+="|"+std::to_string(r.first)+":"+std::to_string(r.second);
 key.append(data.begin(),data.end());uint64_t h=1469598103934665603ull;for(unsigned char c:key){h^=c;h*=1099511628211ull;}
 char id[24];snprintf(id,sizeof id,"%016llx",(unsigned long long)h);
 auto dir=std::filesystem::path(CAPTURE_ROOT)/id;std::filesystem::create_directories(dir);
 std::ofstream(dir/"source.golem",std::ios::binary)<<source;
 std::ofstream opts(dir/"options.txt");if(mode!="raw")opts<<"--"<<mode<<'\n';
 for(auto r:ranges)opts<<"--reserve\n"<<r.first<<':'<<r.second<<'\n';
 if(!table.empty()){opts<<"--table-data\n"<<table<<"=@data.bin\n";std::ofstream f(dir/"data.bin",std::ios::binary);f.write((const char*)data.data(),data.size());}
}
template<class R>auto ranges(const R&rs){std::vector<std::pair<unsigned,unsigned>>o;for(auto r:rs)o.push_back({r.first,r.last});return o;}
}
'''

def capture():
    OUT.mkdir(parents=True,exist_ok=True);cases=OUT/'cases';cases.mkdir(exist_ok=True)
    for folder in ('src','tests'):
        (OUT/folder).mkdir(exist_ok=True)
    for p in (ROOT/'src').rglob('*.hpp'):
        dest=OUT/'src'/p.relative_to(ROOT/'src');dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,dest)
    (OUT/'src/capture.hpp').write_text(TRACE)
    for filename,needle,hook in [
        ('ordinary.hpp','    Compiler compiler;','    capture051::record("raw",src);\n'),
        ('resident.hpp','    golem::BufferAllocator allocation;','    capture051::record("resident",source,capture051::ranges(reservations));\n'),
        ('resident_drawing.hpp','    Result r; r.spec = parse(source);','    capture051::record("resident",source,capture051::ranges(reservations));\n'),
        ('resident_table.hpp','    Result result; result.spec = parse(source);','    capture051::record("resident",source,capture051::ranges(reservations),tableName,data);\n'),
        ('stock_matrix.hpp','    syntax::Reader r(source);','    capture051::record("stock-matrix",source,capture051::ranges(reservations));\n'),
    ]:
        p=OUT/'src'/filename;text=p.read_text()
        if needle not in text:
            # Inspect the maintained function rather than guessing its body.
            if filename=='resident.hpp':needle='    BufferAllocator allocation;'
            elif filename=='resident_drawing.hpp':needle='    Result r;'
        assert needle in text,(filename,needle)
        text=text.replace(needle,hook+needle,1)
        p.write_text('#include "capture.hpp"\n'+text)
    p=OUT/'src/portable/portable.hpp';text=p.read_text();text=text.replace('explicit Parser(const std::string &source) : reader_(source) {}','explicit Parser(const std::string &source) : reader_(source) { capture051::record("portable",source); }');p.write_text('#include "../capture.hpp"\n'+text)
    reports=[]
    for name in ('test_compiler','test_portable','test_stock_matrix','test_stock_loops','test_load_plan'):
        src=OUT/'tests'/(name+'.cpp');shutil.copyfile(ROOT/'tests'/(name+'.cpp'),src)
        binary=OUT/name
        subprocess.run(['g++','-std=c++17','-O1','-ffp-contract=off','-DCAPTURE_ROOT="'+str(cases)+'"',str(src),'-o',str(binary)],check=True)
        args=[str(binary)]
        if name=='test_portable':args += [str(ROOT/'tests/portable/fixtures'),str(OUT/'portable-results')]
        result=subprocess.run(args,capture_output=True,text=True)
        (OUT/(name+'.log')).write_text(result.stdout+result.stderr)
        if result.returncode:raise RuntimeError(name+' failed; inspect capture log')
        reports.append({'test':name,'result':result.stdout.strip()})
    report={'maintained_tests':reports,'captured_sources':len(list(cases.iterdir()))}
    (HERE/'captured-tests.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
    return cases

if __name__=='__main__':capture()
