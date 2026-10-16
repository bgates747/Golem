#!/usr/bin/env python3
"""Check retained text, editing, numeric admission and generated C calls."""
from pathlib import Path
import os
import subprocess
import sys
import json

HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parent));import generate_header

TEST=r'''
#include "parameters.hpp"
#include "generated/flower_golem.h"
#include <cassert>
#include <cstdio>
static uint16_t submitted;
extern "C" void golem_host_call(uint16_t id){submitted=id;}
int main(){
 flower::Parameters p;p.defaults();float n=0;
 assert(!strcmp(p.text[0],"3.03")&&!strcmp(p.text[5],"512"));
 assert(flower::accept(p,0,"3.0300")&&!strcmp(p.text[0],"3.0300"));
 assert(flower::accept(p,0,"")&&!strcmp(p.text[0],"3.0300"));
 const char *bad[]={" ","nan","inf","1x",".","+","1e","1e+","1 2","1e999999999999999999","1e-100","1e100","0","-1","0.000000000000000000000000000001e-10"};
 for(auto s:bad){assert(!flower::accept(p,0,s));assert(!strcmp(p.text[0],"3.0300"));}
 assert(!flower::parse(2,"-1",n)&&!flower::parse(2,"-0.9999999999999999",n));
 assert(flower::parse(2,"-.5",n)&&n==-0.5f);
 assert(flower::parse(4,"-0.25",n)&&n==-0.25f);
 assert(flower::parse(5,"0",n)&&n==0.0f);
 assert(flower::parse(5,"-0e99999999999999999999",n)&&n==0.0f);
 assert(flower::parse(6,"360.000",n)&&n==360.0f);
 assert(!flower::parse(6,"360.1",n)&&!flower::parse(6,"-0.01",n));
 assert(flower::parse(6,"2.25e1",n)&&n==22.5f);
 flower::Editor e;e.reset();assert(!e.key(27));
 for(unsigned i=0;i<31;++i)assert(e.key('1'));
 assert(!e.key('2')&&e.size==31);assert(e.key(8)&&e.size==30);assert(e.key(127)&&e.size==29);
 while(e.size){e.key(8);}
 assert(e.key(8)&&e.size==0);
 uint8_t state[GOLEM_STATE_BYTES];memset(state,0xa5,sizeof state);golem_set_theta_init(state,22.5f);
 const uint8_t *v=state+GOLEM_theta_init_OFFSET;assert(v[0]==0&&v[1]==0&&v[2]==0xb4&&v[3]==0x41&&v[-1]==0xa5&&v[4]==0xa5);
 golem_prepare();assert(submitted==GOLEM_prepare_NATIVE);golem_batch();assert(submitted==GOLEM_batch_NATIVE);
 puts("Retained strings, lexical/domain checks, editing and native C interface pass.");
}
'''

def main():
    out=HERE/'build/host';out.mkdir(parents=True,exist_ok=True)
    (out/'test.cpp').write_text(TEST)
    args=['c++','-std=c++17','-Wall','-Wextra','-Werror','-g','-fsanitize=address,undefined',
          '-I'+str(HERE/'include'),str(out/'test.cpp'),'-o',str(out/'test')]
    subprocess.run(args,check=True);subprocess.run([str(out/'test')],check=True)
    parity=0
    for name in ('default','relocated'):
        package=HERE.parent/'build/abi'/name
        dest=out/(name+'.h');generate_header.generate(package,dest)
        assert dest.read_bytes()==(HERE.parent/'evidence'/(name+'-flower_golem.h')).read_bytes()
        parity+=1
    (HERE/'evidence/host.json').write_text(json.dumps({'passed':True,'ASan_UBSan':True,'earlier_dual_headers_unchanged':parity},indent=2)+'\n')

if __name__=='__main__':main()
