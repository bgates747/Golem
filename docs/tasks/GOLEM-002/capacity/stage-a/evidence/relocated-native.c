/* Generated from the shared Golem model; module state stays in VDP buffers. */
#include "vdplet.h"
static volatile uint32_t identity=0x474c3332u;
static uint16_t get(const uint8_t*p){return (uint16_t)p[0]|(uint16_t)p[1]<<8;}
static void put(uint8_t*p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static inline int32_t signed16(uint32_t v){return (int32_t)v-((v&32768u)?65536:0); }
static inline int32_t signed32(uint32_t v){return v>2147483647u?-1-(int32_t)(4294967295u-v):(int32_t)v;}
#define FAIL(n) do {put(state+14,n);h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return VDPL_E_ARGUMENT;}while(0)
#define ROOT_FAIL(n,rc) do{put(state+12,n);h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return (rc);}while(0)
static inline uint32_t get32(const uint8_t*p){return (uint32_t)get(p)|(uint32_t)get(p+2)<<16;}
static inline void put32(uint8_t*p,uint32_t v){put(p,(uint16_t)v);put(p+2,(uint16_t)(v>>16));}
static inline float fvalue(uint32_t u){union{uint32_t u;float f;}v={.u=u};return v.f;}
static inline uint32_t fbits(float f){union{uint32_t u;float f;}v={.f=f};return v.u;}
static inline uint32_t ordered(uint32_t v){return (v&0x80000000u)?~v:(v^0x80000000u);}
#define FP_RESULT(v) do{if(((v)&0x7f800000u)==0x7f800000u)FAIL(33);if(!((v)&0x7f800000u))(v)=0;}while(0)
#define FP_INPUT(v) do{if(!((v)&0x7f800000u)&&((v)&0x007fffffu))FAIL(33);FP_RESULT(v);}while(0)
__attribute__((visibility("default")))
int32_t vdplet_entry(const vdplet_host_v1*h,const vdplet_request_v1*r){
if(!h||!r||h->struct_bytes<sizeof(*h)||r->struct_bytes<sizeof(*r)||h->abi_major!=1||!(h->capabilities&VDPL_CAP_BUFFERS)||!h->buffer_acquire||!h->buffer_release)return VDPL_E_ABI;
if(r->operation==VDPL_INIT)return identity==0x474c3332u?0:VDPL_E_INIT;
if(r->operation==VDPL_FINI)return 0;
if(r->operation!=VDPL_RUN||r->entry_id>=15)return VDPL_E_UNSUPPORTED;
if(r->argument_bytes!=2)return VDPL_E_ARGUMENT;
vdplet_view_v1 args={sizeof(args),0,0,0,0},view={sizeof(view),0,0,0,0};
int32_t rc=h->buffer_acquire(h->context,r->argument_buffer,VDPL_READ,0,2,&args);if(rc)return rc;
uint16_t id=get(args.data);rc=h->buffer_acquire(h->context,id,VDPL_WRITE,0,VDPL_WHOLE_BUFFER,&view);if(rc){h->buffer_release(h->context,args.lease);return rc;}
if(view.bytes!=292){h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return VDPL_E_BOUNDS;}
uint8_t*state=view.data;put(state+12,0);
static const uint8_t offsets[] __attribute__((aligned(4)))={0,1,2,3,4,5,6,7,26,27,30,31,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,};
static const uint8_t values[] __attribute__((aligned(4)))={82,71,1,0,112,0,36,1,4,0,0,0,15,0,180,0,0,0,0,0,48,53,50,82,1,0,24,121,3,0,0,0,16,0,0,0,1,0,0,0,2,0,25,121,2,0,0,0,8,0,0,0,3,0,0,0,3,0,26,121,8,0,0,0,12,0,0,0,1,0,0,0,4,0,27,121,1,0,0,0,16,0,0,0,1,0,0,0,};
static const uint8_t errors[] __attribute__((aligned(4)))={1,1,2,2,3,3,3,3,3,3,6,6,3,3,3,3,6,6,6,6,6,6,6,6,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,};
for(unsigned i=0;i<sizeof(offsets);++i)if(state[offsets[i]]!=values[i])ROOT_FAIL(errors[i],VDPL_E_ARGUMENT);
if(!get32(state+8))ROOT_FAIL(4,VDPL_E_ARGUMENT);
if(get(state+28)>=15u||get(state+28)!=r->entry_id)ROOT_FAIL(7,VDPL_E_ARGUMENT);
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,31000,1,3u,16u,&bound);
if(rc){ROOT_FAIL(8,rc);}
rc=h->buffer_release(h->context,bound.lease);if(rc){ROOT_FAIL(8,rc);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,31001,2,2u,8u,&bound);
if(rc){ROOT_FAIL(8,rc);}
rc=h->buffer_release(h->context,bound.lease);if(rc){ROOT_FAIL(8,rc);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,31002,1,8u,12u,&bound);
if(rc){ROOT_FAIL(8,rc);}
rc=h->buffer_release(h->context,bound.lease);if(rc){ROOT_FAIL(8,rc);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,31003,1,1u,16u,&bound);
if(rc){ROOT_FAIL(8,rc);}
rc=h->buffer_release(h->context,bound.lease);if(rc){ROOT_FAIL(8,rc);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,502,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=12u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,503,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=24u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,504,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=24u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,505,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=24u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,506,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=12u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,507,1,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=48u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,508,1,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=514u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
put(state+14,0);
if((int64_t)(signed32(get32(state+144)))<-32768ll)FAIL(32);
if((int64_t)(signed32(get32(state+144)))>32767ll)FAIL(32);
if((int64_t)(signed32(get32(state+186)))<1ll)FAIL(32);
if((int64_t)(signed32(get32(state+186)))>3276800ll)FAIL(32);
if((int64_t)(signed32(get32(state+202)))<-38400ll)FAIL(32);
if((int64_t)(signed32(get32(state+202)))>38400ll)FAIL(32);
if((int64_t)(signed32(get32(state+210)))<-14260ll)FAIL(32);
if((int64_t)(signed32(get32(state+210)))>14260ll)FAIL(32);
if(get(state+226)>1u)FAIL(32);
if(get(state+228)>1u)FAIL(32);
if((int64_t)(signed32(get32(state+234)))<-14260ll)FAIL(32);
if((int64_t)(signed32(get32(state+234)))>14260ll)FAIL(32);
if(get(state+238)>1u)FAIL(32);
if((int64_t)(signed32(get32(state+272)))<-10ll)FAIL(32);
if((int64_t)(signed32(get32(state+272)))>10ll)FAIL(32);
if((int64_t)(signed32(get32(state+276)))<-3ll)FAIL(32);
if((int64_t)(signed32(get32(state+276)))>3ll)FAIL(32);
switch(r->entry_id){
case 0:{
uint32_t v0=get32(state+140);
if((int64_t)(signed32(v0))<-32768ll)FAIL(32);
if((int64_t)(signed32(v0))>32767ll)FAIL(32);
put32(state+144,v0);
uint32_t v1=get32(state+144);
put32(state+148,v1);
uint32_t v2=get32(state+148);
uint32_t v3=(signed32(v2)<signed32(0u))?1u:0u;
if(v3){
uint32_t v4=get32(state+148);
int64_t nv5=(int64_t)signed32(0u)-(int64_t)signed32(v4);
if(nv5 < -2147483648ll || nv5 >2147483647ll)FAIL(2);
uint32_t v5=(uint32_t)nv5;
put32(state+148,v5);
}else{
}
uint32_t v6=get32(state+148);
uint32_t v7=(signed32(v6)>signed32(16384u))?1u:0u;
if(v7){
uint32_t v8=get32(state+148);
int64_t nv9=(int64_t)signed32(32768u)-(int64_t)signed32(v8);
if(nv9 < -2147483648ll || nv9 >2147483647ll)FAIL(2);
uint32_t v9=(uint32_t)nv9;
put32(state+148,v9);
}else{
}
uint32_t v10=get32(state+148);
uint32_t v11=64u;
if(!v11)FAIL(4);
if(v10==2147483648u && v11==4294967295u)FAIL(6);
uint32_t v12=(uint32_t)(signed32(v10)/signed32(v11)),v13=(uint32_t)(signed32(v10)%signed32(v11));
put32(state+152,v12);
put32(state+156,v13);
uint32_t v14=get32(state+152);
int64_t nv15=(int64_t)(signed32(v14));
if(nv15<0ll||nv15>65535ll)FAIL(32);
uint32_t v15=(uint32_t)nv15&65535u;
put(state+160,v15);
uint32_t v16=get(state+160);
int32_t index17=(int32_t)(v16);
if(index17<0||index17>256)FAIL(5);
uint32_t v18;{vdplet_view_v1 av18={sizeof(av18),0,0,0,0};
rc=h->buffer_acquire(h->context,508,VDPL_READ,(uint32_t)(index17-(0))*2u,2,&av18);
if(rc){ROOT_FAIL(8,rc);}
v18=get(av18.data);
rc=h->buffer_release(h->context,av18.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv19=(int64_t)(signed16(v18));
if(nv19<-2147483648ll||nv19>2147483647ll)FAIL(32);
uint32_t v19=(uint32_t)nv19;
put32(state+162,v19);
uint32_t v20=get(state+160);
uint32_t v21=(v20<256u)?1u:0u;
if(v21){
uint32_t v22=get(state+160);
uint32_t v23=(uint32_t)(v22)+(uint32_t)(1u);
if(v23>65535u)FAIL(1);
int32_t index24=(int32_t)(v23);
if(index24<0||index24>256)FAIL(5);
uint32_t v25;{vdplet_view_v1 av25={sizeof(av25),0,0,0,0};
rc=h->buffer_acquire(h->context,508,VDPL_READ,(uint32_t)(index24-(0))*2u,2,&av25);
if(rc){ROOT_FAIL(8,rc);}
v25=get(av25.data);
rc=h->buffer_release(h->context,av25.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv26=(int64_t)(signed16(v25));
if(nv26<-2147483648ll||nv26>2147483647ll)FAIL(32);
uint32_t v26=(uint32_t)nv26;
uint32_t v27=get32(state+162);
int64_t nv28=(int64_t)signed32(v26)-(int64_t)signed32(v27);
if(nv28 < -2147483648ll || nv28 >2147483647ll)FAIL(2);
uint32_t v28=(uint32_t)nv28;
uint32_t v29=get32(state+156);
int64_t nv30=(int64_t)signed32(v28)*(int64_t)signed32(v29);
if(nv30 < -2147483648ll || nv30 >2147483647ll)FAIL(3);
uint32_t v30=(uint32_t)nv30;
put32(state+166,v30);
uint32_t v31=get32(state+166);
uint32_t v32=64u;
if(!v32)FAIL(4);
if(v31==2147483648u && v32==4294967295u)FAIL(6);
uint32_t v33=(uint32_t)(signed32(v31)/signed32(v32)),v34=(uint32_t)(signed32(v31)%signed32(v32));
put32(state+170,v33);
put32(state+174,v34);
uint32_t v35=get32(state+162);
uint32_t v36=get32(state+170);
int64_t nv37=(int64_t)signed32(v35)+(int64_t)signed32(v36);
if(nv37 < -2147483648ll || nv37 >2147483647ll)FAIL(1);
uint32_t v37=(uint32_t)nv37;
put32(state+162,v37);
}else{
}
uint32_t v38=get32(state+144);
uint32_t v39=(signed32(v38)<signed32(0u))?1u:0u;
if(v39){
uint32_t v40=get32(state+162);
int64_t nv41=(int64_t)signed32(0u)-(int64_t)signed32(v40);
if(nv41 < -2147483648ll || nv41 >2147483647ll)FAIL(2);
uint32_t v41=(uint32_t)nv41;
put32(state+162,v41);
}else{
}
break;}
case 1:{
uint32_t v42=get(state+116);
uint32_t v43=(v42==0u)?1u:0u;
if(v43){
uint32_t v44=get(state+114);
uint32_t v45=(uint32_t)(v44)+(uint32_t)(1u);
if(v45>65535u)FAIL(1);
put(state+114,v45);
uint32_t v46=get(state+114);
uint32_t v47=(v46>=6u)?1u:0u;
if(v47){
put(state+114,0u);
}else{
}
uint32_t v48=get(state+114);
uint32_t v49=(uint32_t)(v48)*(uint32_t)(4u);
if(v49>65535u)FAIL(3);
put(state+118,v49);
uint32_t v50=get(state+118);
int32_t index51=(int32_t)(v50);
if(index51<0||index51>23)FAIL(5);
uint32_t v52;{vdplet_view_v1 av52={sizeof(av52),0,0,0,0};
rc=h->buffer_acquire(h->context,507,VDPL_READ,(uint32_t)(index51-(0))*2u,2,&av52);
if(rc){ROOT_FAIL(8,rc);}
v52=get(av52.data);
rc=h->buffer_release(h->context,av52.lease);if(rc){ROOT_FAIL(8,rc);}}
if(v52>32767u)FAIL(32);
put(state+116,v52);
uint32_t v53=get(state+118);
uint32_t v54=(uint32_t)(v53)+(uint32_t)(2u);
if(v54>65535u)FAIL(1);
int32_t index55=(int32_t)(v54);
if(index55<0||index55>23)FAIL(5);
uint32_t v56;{vdplet_view_v1 av56={sizeof(av56),0,0,0,0};
rc=h->buffer_acquire(h->context,507,VDPL_READ,(uint32_t)(index55-(0))*2u,2,&av56);
if(rc){ROOT_FAIL(8,rc);}
v56=get(av56.data);
rc=h->buffer_release(h->context,av56.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv57=(int64_t)(signed16(v56));
if(nv57<-2147483648ll||nv57>2147483647ll)FAIL(32);
uint32_t v57=(uint32_t)nv57;
put32(state+120,v57);
}else{
}
uint32_t v58=get(state+114);
uint32_t v59=(uint32_t)(v58)*(uint32_t)(4u);
if(v59>65535u)FAIL(3);
put(state+118,v59);
uint32_t v60=get32(state+120);
uint32_t v61=get(state+118);
uint32_t v62=(uint32_t)(v61)+(uint32_t)(3u);
if(v62>65535u)FAIL(1);
int32_t index63=(int32_t)(v62);
if(index63<0||index63>23)FAIL(5);
uint32_t v64;{vdplet_view_v1 av64={sizeof(av64),0,0,0,0};
rc=h->buffer_acquire(h->context,507,VDPL_READ,(uint32_t)(index63-(0))*2u,2,&av64);
if(rc){ROOT_FAIL(8,rc);}
v64=get(av64.data);
rc=h->buffer_release(h->context,av64.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv65=(int64_t)(signed16(v64));
if(nv65<-2147483648ll||nv65>2147483647ll)FAIL(32);
uint32_t v65=(uint32_t)nv65;
int64_t nv66=(int64_t)signed32(v60)+(int64_t)signed32(v65);
if(nv66 < -2147483648ll || nv66 >2147483647ll)FAIL(1);
uint32_t v66=(uint32_t)nv66;
put32(state+120,v66);
uint32_t v67=get32(state+120);
put32(state+140,v67);
uint32_t v68=get32(state+140);
if((int64_t)(signed32(v68))<-32768ll)FAIL(32);
if((int64_t)(signed32(v68))>32767ll)FAIL(32);
put32(state+144,v68);
uint32_t v69=get32(state+144);
put32(state+148,v69);
uint32_t v70=get32(state+148);
uint32_t v71=(signed32(v70)<signed32(0u))?1u:0u;
if(v71){
uint32_t v72=get32(state+148);
int64_t nv73=(int64_t)signed32(0u)-(int64_t)signed32(v72);
if(nv73 < -2147483648ll || nv73 >2147483647ll)FAIL(2);
uint32_t v73=(uint32_t)nv73;
put32(state+148,v73);
}else{
}
uint32_t v74=get32(state+148);
uint32_t v75=(signed32(v74)>signed32(16384u))?1u:0u;
if(v75){
uint32_t v76=get32(state+148);
int64_t nv77=(int64_t)signed32(32768u)-(int64_t)signed32(v76);
if(nv77 < -2147483648ll || nv77 >2147483647ll)FAIL(2);
uint32_t v77=(uint32_t)nv77;
put32(state+148,v77);
}else{
}
uint32_t v78=get32(state+148);
uint32_t v79=64u;
if(!v79)FAIL(4);
if(v78==2147483648u && v79==4294967295u)FAIL(6);
uint32_t v80=(uint32_t)(signed32(v78)/signed32(v79)),v81=(uint32_t)(signed32(v78)%signed32(v79));
put32(state+152,v80);
put32(state+156,v81);
uint32_t v82=get32(state+152);
int64_t nv83=(int64_t)(signed32(v82));
if(nv83<0ll||nv83>65535ll)FAIL(32);
uint32_t v83=(uint32_t)nv83&65535u;
put(state+160,v83);
uint32_t v84=get(state+160);
int32_t index85=(int32_t)(v84);
if(index85<0||index85>256)FAIL(5);
uint32_t v86;{vdplet_view_v1 av86={sizeof(av86),0,0,0,0};
rc=h->buffer_acquire(h->context,508,VDPL_READ,(uint32_t)(index85-(0))*2u,2,&av86);
if(rc){ROOT_FAIL(8,rc);}
v86=get(av86.data);
rc=h->buffer_release(h->context,av86.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv87=(int64_t)(signed16(v86));
if(nv87<-2147483648ll||nv87>2147483647ll)FAIL(32);
uint32_t v87=(uint32_t)nv87;
put32(state+162,v87);
uint32_t v88=get(state+160);
uint32_t v89=(v88<256u)?1u:0u;
if(v89){
uint32_t v90=get(state+160);
uint32_t v91=(uint32_t)(v90)+(uint32_t)(1u);
if(v91>65535u)FAIL(1);
int32_t index92=(int32_t)(v91);
if(index92<0||index92>256)FAIL(5);
uint32_t v93;{vdplet_view_v1 av93={sizeof(av93),0,0,0,0};
rc=h->buffer_acquire(h->context,508,VDPL_READ,(uint32_t)(index92-(0))*2u,2,&av93);
if(rc){ROOT_FAIL(8,rc);}
v93=get(av93.data);
rc=h->buffer_release(h->context,av93.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv94=(int64_t)(signed16(v93));
if(nv94<-2147483648ll||nv94>2147483647ll)FAIL(32);
uint32_t v94=(uint32_t)nv94;
uint32_t v95=get32(state+162);
int64_t nv96=(int64_t)signed32(v94)-(int64_t)signed32(v95);
if(nv96 < -2147483648ll || nv96 >2147483647ll)FAIL(2);
uint32_t v96=(uint32_t)nv96;
uint32_t v97=get32(state+156);
int64_t nv98=(int64_t)signed32(v96)*(int64_t)signed32(v97);
if(nv98 < -2147483648ll || nv98 >2147483647ll)FAIL(3);
uint32_t v98=(uint32_t)nv98;
put32(state+166,v98);
uint32_t v99=get32(state+166);
uint32_t v100=64u;
if(!v100)FAIL(4);
if(v99==2147483648u && v100==4294967295u)FAIL(6);
uint32_t v101=(uint32_t)(signed32(v99)/signed32(v100)),v102=(uint32_t)(signed32(v99)%signed32(v100));
put32(state+170,v101);
put32(state+174,v102);
uint32_t v103=get32(state+162);
uint32_t v104=get32(state+170);
int64_t nv105=(int64_t)signed32(v103)+(int64_t)signed32(v104);
if(nv105 < -2147483648ll || nv105 >2147483647ll)FAIL(1);
uint32_t v105=(uint32_t)nv105;
put32(state+162,v105);
}else{
}
uint32_t v106=get32(state+144);
uint32_t v107=(signed32(v106)<signed32(0u))?1u:0u;
if(v107){
uint32_t v108=get32(state+162);
int64_t nv109=(int64_t)signed32(0u)-(int64_t)signed32(v108);
if(nv109 < -2147483648ll || nv109 >2147483647ll)FAIL(2);
uint32_t v109=(uint32_t)nv109;
put32(state+162,v109);
}else{
}
uint32_t v110=get(state+118);
uint32_t v111=(uint32_t)(v110)+(uint32_t)(1u);
if(v111>65535u)FAIL(1);
int32_t index112=(int32_t)(v111);
if(index112<0||index112>23)FAIL(5);
uint32_t v113;{vdplet_view_v1 av113={sizeof(av113),0,0,0,0};
rc=h->buffer_acquire(h->context,507,VDPL_READ,(uint32_t)(index112-(0))*2u,2,&av113);
if(rc){ROOT_FAIL(8,rc);}
v113=get(av113.data);
rc=h->buffer_release(h->context,av113.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv114=(int64_t)(signed16(v113));
if(nv114<-2147483648ll||nv114>2147483647ll)FAIL(32);
uint32_t v114=(uint32_t)nv114;
put32(state+178,v114);
uint32_t v115=get32(state+178);
int64_t nv116=(int64_t)signed32(v115)*(int64_t)signed32(2u);
if(nv116 < -2147483648ll || nv116 >2147483647ll)FAIL(3);
uint32_t v116=(uint32_t)nv116;
uint32_t v117=get32(state+162);
int64_t nv118=(int64_t)signed32(v116)*(int64_t)signed32(v117);
if(nv118 < -2147483648ll || nv118 >2147483647ll)FAIL(3);
uint32_t v118=(uint32_t)nv118;
put32(state+166,v118);
uint32_t v119=get32(state+166);
uint32_t v120=12800u;
if(!v120)FAIL(4);
if(v119==2147483648u && v120==4294967295u)FAIL(6);
uint32_t v121=(uint32_t)(signed32(v119)/signed32(v120)),v122=(uint32_t)(signed32(v119)%signed32(v120));
put32(state+170,v121);
put32(state+174,v122);
uint32_t v123=get32(state+124);
uint32_t v124=get32(state+170);
int64_t nv125=(int64_t)signed32(v123)+(int64_t)signed32(v124);
if(nv125 < -2147483648ll || nv125 >2147483647ll)FAIL(1);
uint32_t v125=(uint32_t)nv125;
put32(state+124,v125);
uint32_t v126=get32(state+178);
int64_t nv127=(int64_t)signed32(v126)*(int64_t)signed32(2u);
if(nv127 < -2147483648ll || nv127 >2147483647ll)FAIL(3);
uint32_t v127=(uint32_t)nv127;
int64_t nv128=(int64_t)(signed32(v127));
if(nv128<0ll||nv128>4294967295ll)FAIL(32);
uint32_t v128=(uint32_t)nv128;
put32(state+132,v128);
uint32_t v129=get32(state+128);
uint32_t v130=get32(state+132);
uint64_t nv131=(uint64_t)(v129)+(uint64_t)(v130);
if(nv131>4294967295ull)FAIL(1);
uint32_t v131=(uint32_t)nv131;
put32(state+128,v131);
uint32_t v132=get(state+116);
if((uint32_t)(v132)<(uint32_t)(1u))FAIL(2);
uint32_t v133=(uint32_t)(v132)-(uint32_t)(1u);
put(state+116,v133);
break;}
case 2:{
uint32_t v134=get32(state+182);
if((int64_t)(signed32(v134))<1ll)FAIL(32);
if((int64_t)(signed32(v134))>3276800ll)FAIL(32);
put32(state+186,v134);
uint32_t v135=get32(state+206);
int64_t nv136=(int64_t)(v135);
if(nv136<-2147483648ll||nv136>2147483647ll)FAIL(32);
uint32_t v136=(uint32_t)nv136;
uint32_t v137=get32(state+198);
int64_t nv138=(int64_t)(v137);
if(nv138<-2147483648ll||nv138>2147483647ll)FAIL(32);
uint32_t v138=(uint32_t)nv138;
int64_t nv139=(int64_t)signed32(v136)-(int64_t)signed32(v138);
if(nv139 < -2147483648ll || nv139 >2147483647ll)FAIL(2);
uint32_t v139=(uint32_t)nv139;
put32(state+166,v139);
uint32_t v140=get32(state+166);
uint32_t v141=get32(state+186);
uint32_t v142=v141;
if(!v142)FAIL(4);
if(v140==2147483648u && v142==4294967295u)FAIL(6);
uint32_t v143=(uint32_t)(signed32(v140)/signed32(v142)),v144=(uint32_t)(signed32(v140)%signed32(v142));
put32(state+170,v143);
put32(state+174,v144);
uint32_t v145=get32(state+174);
put32(state+214,v145);
uint32_t v146=get32(state+214);
uint32_t v147=(signed32(v146)<signed32(0u))?1u:0u;
if(v147){
uint32_t v148=get32(state+214);
uint32_t v149=get32(state+186);
int64_t nv150=(int64_t)signed32(v148)+(int64_t)signed32(v149);
if(nv150 < -2147483648ll || nv150 >2147483647ll)FAIL(1);
uint32_t v150=(uint32_t)nv150;
put32(state+214,v150);
}else{
}
uint32_t v151=get32(state+186);
uint32_t v152=2u;
if(!v152)FAIL(4);
if(v151==2147483648u && v152==4294967295u)FAIL(6);
uint32_t v153=(uint32_t)(signed32(v151)/signed32(v152)),v154=(uint32_t)(signed32(v151)%signed32(v152));
put32(state+222,v153);
put32(state+174,v154);
uint32_t v155=get32(state+214);
uint32_t v156=get32(state+222);
uint32_t v157=(signed32(v155)>signed32(v156))?1u:0u;
if(v157){
uint32_t v158=get32(state+214);
uint32_t v159=get32(state+186);
int64_t nv160=(int64_t)signed32(v158)-(int64_t)signed32(v159);
if(nv160 < -2147483648ll || nv160 >2147483647ll)FAIL(2);
uint32_t v160=(uint32_t)nv160;
put32(state+214,v160);
}else{
}
uint32_t v161=get32(state+214);
put32(state+166,v161);
uint32_t v162=get32(state+166);
uint32_t v163=(signed32(v162)<signed32(0u))?1u:0u;
if(v163){
uint32_t v164=get32(state+166);
int64_t nv165=(int64_t)signed32(0u)-(int64_t)signed32(v164);
if(nv165 < -2147483648ll || nv165 >2147483647ll)FAIL(2);
uint32_t v165=(uint32_t)nv165;
put32(state+166,v165);
}else{
}
uint32_t v166=get32(state+210);
uint32_t v167=get32(state+202);
int64_t nv168=(int64_t)signed32(v166)-(int64_t)signed32(v167);
if(nv168 < -2147483648ll || nv168 >2147483647ll)FAIL(2);
uint32_t v168=(uint32_t)nv168;
put32(state+218,v168);
uint32_t v169=get32(state+218);
uint32_t v170=(signed32(v169)<signed32(0u))?1u:0u;
if(v170){
uint32_t v171=get32(state+218);
int64_t nv172=(int64_t)signed32(0u)-(int64_t)signed32(v171);
if(nv172 < -2147483648ll || nv172 >2147483647ll)FAIL(2);
uint32_t v172=(uint32_t)nv172;
put32(state+218,v172);
}else{
}
uint32_t v173=get32(state+166);
uint32_t v174=(signed32(v173)<signed32(3200u))?1u:0u;
uint32_t v175=v174;
if(v175){
uint32_t v176=get32(state+218);
uint32_t v177=(signed32(v176)<signed32(6144u))?1u:0u;
v175=v177;
}
if(v175>1u)FAIL(32);
put(state+226,v175);
break;}
case 3:{
uint32_t v178=get(state+116);
uint32_t v179=(v178==0u)?1u:0u;
if(v179){
uint32_t v180=get(state+114);
uint32_t v181=(uint32_t)(v180)+(uint32_t)(1u);
if(v181>65535u)FAIL(1);
put(state+114,v181);
uint32_t v182=get(state+114);
uint32_t v183=(v182>=6u)?1u:0u;
if(v183){
put(state+114,0u);
}else{
}
uint32_t v184=get(state+114);
uint32_t v185=(uint32_t)(v184)*(uint32_t)(4u);
if(v185>65535u)FAIL(3);
put(state+118,v185);
uint32_t v186=get(state+118);
int32_t index187=(int32_t)(v186);
if(index187<0||index187>23)FAIL(5);
uint32_t v188;{vdplet_view_v1 av188={sizeof(av188),0,0,0,0};
rc=h->buffer_acquire(h->context,507,VDPL_READ,(uint32_t)(index187-(0))*2u,2,&av188);
if(rc){ROOT_FAIL(8,rc);}
v188=get(av188.data);
rc=h->buffer_release(h->context,av188.lease);if(rc){ROOT_FAIL(8,rc);}}
if(v188>32767u)FAIL(32);
put(state+116,v188);
uint32_t v189=get(state+118);
uint32_t v190=(uint32_t)(v189)+(uint32_t)(2u);
if(v190>65535u)FAIL(1);
int32_t index191=(int32_t)(v190);
if(index191<0||index191>23)FAIL(5);
uint32_t v192;{vdplet_view_v1 av192={sizeof(av192),0,0,0,0};
rc=h->buffer_acquire(h->context,507,VDPL_READ,(uint32_t)(index191-(0))*2u,2,&av192);
if(rc){ROOT_FAIL(8,rc);}
v192=get(av192.data);
rc=h->buffer_release(h->context,av192.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv193=(int64_t)(signed16(v192));
if(nv193<-2147483648ll||nv193>2147483647ll)FAIL(32);
uint32_t v193=(uint32_t)nv193;
put32(state+120,v193);
}else{
}
uint32_t v194=get(state+114);
uint32_t v195=(uint32_t)(v194)*(uint32_t)(4u);
if(v195>65535u)FAIL(3);
put(state+118,v195);
uint32_t v196=get32(state+120);
uint32_t v197=get(state+118);
uint32_t v198=(uint32_t)(v197)+(uint32_t)(3u);
if(v198>65535u)FAIL(1);
int32_t index199=(int32_t)(v198);
if(index199<0||index199>23)FAIL(5);
uint32_t v200;{vdplet_view_v1 av200={sizeof(av200),0,0,0,0};
rc=h->buffer_acquire(h->context,507,VDPL_READ,(uint32_t)(index199-(0))*2u,2,&av200);
if(rc){ROOT_FAIL(8,rc);}
v200=get(av200.data);
rc=h->buffer_release(h->context,av200.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv201=(int64_t)(signed16(v200));
if(nv201<-2147483648ll||nv201>2147483647ll)FAIL(32);
uint32_t v201=(uint32_t)nv201;
int64_t nv202=(int64_t)signed32(v196)+(int64_t)signed32(v201);
if(nv202 < -2147483648ll || nv202 >2147483647ll)FAIL(1);
uint32_t v202=(uint32_t)nv202;
put32(state+120,v202);
uint32_t v203=get32(state+120);
put32(state+140,v203);
uint32_t v204=get32(state+140);
if((int64_t)(signed32(v204))<-32768ll)FAIL(32);
if((int64_t)(signed32(v204))>32767ll)FAIL(32);
put32(state+144,v204);
uint32_t v205=get32(state+144);
put32(state+148,v205);
uint32_t v206=get32(state+148);
uint32_t v207=(signed32(v206)<signed32(0u))?1u:0u;
if(v207){
uint32_t v208=get32(state+148);
int64_t nv209=(int64_t)signed32(0u)-(int64_t)signed32(v208);
if(nv209 < -2147483648ll || nv209 >2147483647ll)FAIL(2);
uint32_t v209=(uint32_t)nv209;
put32(state+148,v209);
}else{
}
uint32_t v210=get32(state+148);
uint32_t v211=(signed32(v210)>signed32(16384u))?1u:0u;
if(v211){
uint32_t v212=get32(state+148);
int64_t nv213=(int64_t)signed32(32768u)-(int64_t)signed32(v212);
if(nv213 < -2147483648ll || nv213 >2147483647ll)FAIL(2);
uint32_t v213=(uint32_t)nv213;
put32(state+148,v213);
}else{
}
uint32_t v214=get32(state+148);
uint32_t v215=64u;
if(!v215)FAIL(4);
if(v214==2147483648u && v215==4294967295u)FAIL(6);
uint32_t v216=(uint32_t)(signed32(v214)/signed32(v215)),v217=(uint32_t)(signed32(v214)%signed32(v215));
put32(state+152,v216);
put32(state+156,v217);
uint32_t v218=get32(state+152);
int64_t nv219=(int64_t)(signed32(v218));
if(nv219<0ll||nv219>65535ll)FAIL(32);
uint32_t v219=(uint32_t)nv219&65535u;
put(state+160,v219);
uint32_t v220=get(state+160);
int32_t index221=(int32_t)(v220);
if(index221<0||index221>256)FAIL(5);
uint32_t v222;{vdplet_view_v1 av222={sizeof(av222),0,0,0,0};
rc=h->buffer_acquire(h->context,508,VDPL_READ,(uint32_t)(index221-(0))*2u,2,&av222);
if(rc){ROOT_FAIL(8,rc);}
v222=get(av222.data);
rc=h->buffer_release(h->context,av222.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv223=(int64_t)(signed16(v222));
if(nv223<-2147483648ll||nv223>2147483647ll)FAIL(32);
uint32_t v223=(uint32_t)nv223;
put32(state+162,v223);
uint32_t v224=get(state+160);
uint32_t v225=(v224<256u)?1u:0u;
if(v225){
uint32_t v226=get(state+160);
uint32_t v227=(uint32_t)(v226)+(uint32_t)(1u);
if(v227>65535u)FAIL(1);
int32_t index228=(int32_t)(v227);
if(index228<0||index228>256)FAIL(5);
uint32_t v229;{vdplet_view_v1 av229={sizeof(av229),0,0,0,0};
rc=h->buffer_acquire(h->context,508,VDPL_READ,(uint32_t)(index228-(0))*2u,2,&av229);
if(rc){ROOT_FAIL(8,rc);}
v229=get(av229.data);
rc=h->buffer_release(h->context,av229.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv230=(int64_t)(signed16(v229));
if(nv230<-2147483648ll||nv230>2147483647ll)FAIL(32);
uint32_t v230=(uint32_t)nv230;
uint32_t v231=get32(state+162);
int64_t nv232=(int64_t)signed32(v230)-(int64_t)signed32(v231);
if(nv232 < -2147483648ll || nv232 >2147483647ll)FAIL(2);
uint32_t v232=(uint32_t)nv232;
uint32_t v233=get32(state+156);
int64_t nv234=(int64_t)signed32(v232)*(int64_t)signed32(v233);
if(nv234 < -2147483648ll || nv234 >2147483647ll)FAIL(3);
uint32_t v234=(uint32_t)nv234;
put32(state+166,v234);
uint32_t v235=get32(state+166);
uint32_t v236=64u;
if(!v236)FAIL(4);
if(v235==2147483648u && v236==4294967295u)FAIL(6);
uint32_t v237=(uint32_t)(signed32(v235)/signed32(v236)),v238=(uint32_t)(signed32(v235)%signed32(v236));
put32(state+170,v237);
put32(state+174,v238);
uint32_t v239=get32(state+162);
uint32_t v240=get32(state+170);
int64_t nv241=(int64_t)signed32(v239)+(int64_t)signed32(v240);
if(nv241 < -2147483648ll || nv241 >2147483647ll)FAIL(1);
uint32_t v241=(uint32_t)nv241;
put32(state+162,v241);
}else{
}
uint32_t v242=get32(state+144);
uint32_t v243=(signed32(v242)<signed32(0u))?1u:0u;
if(v243){
uint32_t v244=get32(state+162);
int64_t nv245=(int64_t)signed32(0u)-(int64_t)signed32(v244);
if(nv245 < -2147483648ll || nv245 >2147483647ll)FAIL(2);
uint32_t v245=(uint32_t)nv245;
put32(state+162,v245);
}else{
}
uint32_t v246=get(state+118);
uint32_t v247=(uint32_t)(v246)+(uint32_t)(1u);
if(v247>65535u)FAIL(1);
int32_t index248=(int32_t)(v247);
if(index248<0||index248>23)FAIL(5);
uint32_t v249;{vdplet_view_v1 av249={sizeof(av249),0,0,0,0};
rc=h->buffer_acquire(h->context,507,VDPL_READ,(uint32_t)(index248-(0))*2u,2,&av249);
if(rc){ROOT_FAIL(8,rc);}
v249=get(av249.data);
rc=h->buffer_release(h->context,av249.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv250=(int64_t)(signed16(v249));
if(nv250<-2147483648ll||nv250>2147483647ll)FAIL(32);
uint32_t v250=(uint32_t)nv250;
put32(state+178,v250);
uint32_t v251=get32(state+178);
int64_t nv252=(int64_t)signed32(v251)*(int64_t)signed32(2u);
if(nv252 < -2147483648ll || nv252 >2147483647ll)FAIL(3);
uint32_t v252=(uint32_t)nv252;
uint32_t v253=get32(state+162);
int64_t nv254=(int64_t)signed32(v252)*(int64_t)signed32(v253);
if(nv254 < -2147483648ll || nv254 >2147483647ll)FAIL(3);
uint32_t v254=(uint32_t)nv254;
put32(state+166,v254);
uint32_t v255=get32(state+166);
uint32_t v256=12800u;
if(!v256)FAIL(4);
if(v255==2147483648u && v256==4294967295u)FAIL(6);
uint32_t v257=(uint32_t)(signed32(v255)/signed32(v256)),v258=(uint32_t)(signed32(v255)%signed32(v256));
put32(state+170,v257);
put32(state+174,v258);
uint32_t v259=get32(state+124);
uint32_t v260=get32(state+170);
int64_t nv261=(int64_t)signed32(v259)+(int64_t)signed32(v260);
if(nv261 < -2147483648ll || nv261 >2147483647ll)FAIL(1);
uint32_t v261=(uint32_t)nv261;
put32(state+124,v261);
uint32_t v262=get32(state+178);
int64_t nv263=(int64_t)signed32(v262)*(int64_t)signed32(2u);
if(nv263 < -2147483648ll || nv263 >2147483647ll)FAIL(3);
uint32_t v263=(uint32_t)nv263;
int64_t nv264=(int64_t)(signed32(v263));
if(nv264<0ll||nv264>4294967295ll)FAIL(32);
uint32_t v264=(uint32_t)nv264;
put32(state+132,v264);
uint32_t v265=get32(state+128);
uint32_t v266=get32(state+132);
uint64_t nv267=(uint64_t)(v265)+(uint64_t)(v266);
if(nv267>4294967295ull)FAIL(1);
uint32_t v267=(uint32_t)nv267;
put32(state+128,v267);
uint32_t v268=get(state+116);
if((uint32_t)(v268)<(uint32_t)(1u))FAIL(2);
uint32_t v269=(uint32_t)(v268)-(uint32_t)(1u);
put(state+116,v269);
uint32_t v270=get32(state+182);
if((int64_t)(signed32(v270))<1ll)FAIL(32);
if((int64_t)(signed32(v270))>3276800ll)FAIL(32);
put32(state+186,v270);
if(0u>1u)FAIL(32);
put(state+228,0u);
for(uint32_t loop271=0;loop271<6u;++loop271){
put(state+112,(uint16_t)(0+(int32_t)loop271*(1)));
uint32_t v272=get(state+112);
int32_t index273=(int32_t)(v272);
if(index273<0||index273>5)FAIL(5);
uint32_t v274;{vdplet_view_v1 av274={sizeof(av274),0,0,0,0};
rc=h->buffer_acquire(h->context,502,VDPL_READ,(uint32_t)(index273-(0))*2u,2,&av274);
if(rc){ROOT_FAIL(8,rc);}
v274=get(av274.data);
rc=h->buffer_release(h->context,av274.lease);if(rc){ROOT_FAIL(8,rc);}}
if(v274>1u)FAIL(32);
put(state+238,v274);
uint32_t v275=get(state+112);
int32_t index276=(int32_t)(v275);
if(index276<0||index276>5)FAIL(5);
{vdplet_view_v1 store277={sizeof(store277),0,0,0,0};
rc=h->buffer_acquire(h->context,506,VDPL_WRITE,(uint32_t)(index276-(0))*2u,2,&store277);
if(rc){ROOT_FAIL(8,rc);}
put(store277.data,0u);
rc=h->buffer_release(h->context,store277.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v278=get(state+238);
uint32_t v279=(v278==1u)?1u:0u;
if(v279){
uint32_t v280=get(state+112);
int32_t index281=(int32_t)(v280);
if(index281<0||index281>5)FAIL(5);
uint32_t v283=get(state+112);
int32_t index284=(int32_t)(v283);
if(index284<0||index284>5)FAIL(5);
uint32_t v285;{vdplet_view_v1 av285={sizeof(av285),0,0,0,0};
rc=h->buffer_acquire(h->context,503,VDPL_READ,(uint32_t)(index284-(0))*4u,4,&av285);
if(rc){ROOT_FAIL(8,rc);}
v285=get32(av285.data);
rc=h->buffer_release(h->context,av285.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v286=get32(state+132);
int64_t nv287=(int64_t)(v286);
if(nv287<-2147483648ll||nv287>2147483647ll)FAIL(32);
uint32_t v287=(uint32_t)nv287;
int64_t nv288=(int64_t)signed32(v285)+(int64_t)signed32(v287);
if(nv288 < -2147483648ll || nv288 >2147483647ll)FAIL(1);
uint32_t v288=(uint32_t)nv288;
{vdplet_view_v1 store282={sizeof(store282),0,0,0,0};
rc=h->buffer_acquire(h->context,503,VDPL_WRITE,(uint32_t)(index281-(0))*4u,4,&store282);
if(rc){ROOT_FAIL(8,rc);}
put32(store282.data,v288);
rc=h->buffer_release(h->context,store282.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v289=get(state+112);
int32_t index290=(int32_t)(v289);
if(index290<0||index290>5)FAIL(5);
uint32_t v291;{vdplet_view_v1 av291={sizeof(av291),0,0,0,0};
rc=h->buffer_acquire(h->context,504,VDPL_READ,(uint32_t)(index290-(0))*4u,4,&av291);
if(rc){ROOT_FAIL(8,rc);}
v291=get32(av291.data);
rc=h->buffer_release(h->context,av291.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv292=(int64_t)(v291);
if(nv292<-2147483648ll||nv292>2147483647ll)FAIL(32);
uint32_t v292=(uint32_t)nv292;
uint32_t v293=get32(state+132);
int64_t nv294=(int64_t)(v293);
if(nv294<-2147483648ll||nv294>2147483647ll)FAIL(32);
uint32_t v294=(uint32_t)nv294;
int64_t nv295=(int64_t)signed32(v292)+(int64_t)signed32(v294);
if(nv295 < -2147483648ll || nv295 >2147483647ll)FAIL(1);
uint32_t v295=(uint32_t)nv295;
put32(state+166,v295);
uint32_t v296=get32(state+166);
uint32_t v297=get32(state+186);
uint32_t v298=v297;
if(!v298)FAIL(4);
if(v296==2147483648u && v298==4294967295u)FAIL(6);
uint32_t v299=(uint32_t)(signed32(v296)/signed32(v298)),v300=(uint32_t)(signed32(v296)%signed32(v298));
put32(state+170,v299);
put32(state+174,v300);
uint32_t v301=get32(state+174);
uint32_t v302=(signed32(v301)<signed32(0u))?1u:0u;
if(v302){
uint32_t v303=get32(state+174);
uint32_t v304=get32(state+186);
int64_t nv305=(int64_t)signed32(v303)+(int64_t)signed32(v304);
if(nv305 < -2147483648ll || nv305 >2147483647ll)FAIL(1);
uint32_t v305=(uint32_t)nv305;
put32(state+174,v305);
}else{
}
uint32_t v306=get(state+112);
int32_t index307=(int32_t)(v306);
if(index307<0||index307>5)FAIL(5);
uint32_t v309=get32(state+174);
int64_t nv310=(int64_t)(signed32(v309));
if(nv310<0ll||nv310>4294967295ll)FAIL(32);
uint32_t v310=(uint32_t)nv310;
{vdplet_view_v1 store308={sizeof(store308),0,0,0,0};
rc=h->buffer_acquire(h->context,504,VDPL_WRITE,(uint32_t)(index307-(0))*4u,4,&store308);
if(rc){ROOT_FAIL(8,rc);}
put32(store308.data,v310);
rc=h->buffer_release(h->context,store308.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v311=get(state+112);
uint32_t v312=3u;
if(!v312)FAIL(4);
uint32_t v313=v311/v312,v314=v311%v312;
put(state+230,v313);
put(state+232,v314);
uint32_t v315=get(state+232);
int64_t nv316=(int64_t)(v315);
if(nv316<-2147483648ll||nv316>2147483647ll)FAIL(32);
uint32_t v316=(uint32_t)nv316;
int64_t nv317=(int64_t)signed32(v316)-(int64_t)signed32(1u);
if(nv317 < -2147483648ll || nv317 >2147483647ll)FAIL(2);
uint32_t v317=(uint32_t)nv317;
int64_t nv318=(int64_t)signed32(v317)*(int64_t)signed32(9216u);
if(nv318 < -2147483648ll || nv318 >2147483647ll)FAIL(3);
uint32_t v318=(uint32_t)nv318;
if((int64_t)(signed32(v318))<-14260ll)FAIL(32);
if((int64_t)(signed32(v318))>14260ll)FAIL(32);
put32(state+234,v318);
uint32_t v319=get(state+112);
uint32_t v320=2u;
if(!v320)FAIL(4);
uint32_t v321=v319/v320,v322=v319%v320;
put(state+230,v321);
put(state+232,v322);
uint32_t v323=get(state+232);
uint32_t v324=(v323==0u)?1u:0u;
if(v324){
uint32_t v325=get32(state+234);
uint32_t v326=get32(state+124);
int64_t nv327=(int64_t)signed32(v325)+(int64_t)signed32(v326);
if(nv327 < -2147483648ll || nv327 >2147483647ll)FAIL(1);
uint32_t v327=(uint32_t)nv327;
int64_t nv328=(int64_t)signed32(v327)-(int64_t)signed32(5044u);
if(nv328 < -2147483648ll || nv328 >2147483647ll)FAIL(2);
uint32_t v328=(uint32_t)nv328;
if((int64_t)(signed32(v328))<-14260ll)FAIL(32);
if((int64_t)(signed32(v328))>14260ll)FAIL(32);
put32(state+234,v328);
}else{
uint32_t v329=get32(state+234);
uint32_t v330=get32(state+124);
int64_t nv331=(int64_t)signed32(v329)-(int64_t)signed32(v330);
if(nv331 < -2147483648ll || nv331 >2147483647ll)FAIL(2);
uint32_t v331=(uint32_t)nv331;
int64_t nv332=(int64_t)signed32(v331)+(int64_t)signed32(5044u);
if(nv332 < -2147483648ll || nv332 >2147483647ll)FAIL(1);
uint32_t v332=(uint32_t)nv332;
if((int64_t)(signed32(v332))<-14260ll)FAIL(32);
if((int64_t)(signed32(v332))>14260ll)FAIL(32);
put32(state+234,v332);
}
uint32_t v333=get(state+112);
int32_t index334=(int32_t)(v333);
if(index334<0||index334>5)FAIL(5);
uint32_t v336=get32(state+234);
{vdplet_view_v1 store335={sizeof(store335),0,0,0,0};
rc=h->buffer_acquire(h->context,505,VDPL_WRITE,(uint32_t)(index334-(0))*4u,4,&store335);
if(rc){ROOT_FAIL(8,rc);}
put32(store335.data,v336);
rc=h->buffer_release(h->context,store335.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v337=get(state+112);
int32_t index338=(int32_t)(v337);
if(index338<0||index338>5)FAIL(5);
uint32_t v339;{vdplet_view_v1 av339={sizeof(av339),0,0,0,0};
rc=h->buffer_acquire(h->context,504,VDPL_READ,(uint32_t)(index338-(0))*4u,4,&av339);
if(rc){ROOT_FAIL(8,rc);}
v339=get32(av339.data);
rc=h->buffer_release(h->context,av339.lease);if(rc){ROOT_FAIL(8,rc);}}
put32(state+206,v339);
uint32_t v340=get(state+112);
int32_t index341=(int32_t)(v340);
if(index341<0||index341>5)FAIL(5);
uint32_t v342;{vdplet_view_v1 av342={sizeof(av342),0,0,0,0};
rc=h->buffer_acquire(h->context,505,VDPL_READ,(uint32_t)(index341-(0))*4u,4,&av342);
if(rc){ROOT_FAIL(8,rc);}
v342=get32(av342.data);
rc=h->buffer_release(h->context,av342.lease);if(rc){ROOT_FAIL(8,rc);}}
if((int64_t)(signed32(v342))<-14260ll)FAIL(32);
if((int64_t)(signed32(v342))>14260ll)FAIL(32);
put32(state+210,v342);
uint32_t v343=get32(state+182);
if((int64_t)(signed32(v343))<1ll)FAIL(32);
if((int64_t)(signed32(v343))>3276800ll)FAIL(32);
put32(state+186,v343);
uint32_t v344=get32(state+206);
int64_t nv345=(int64_t)(v344);
if(nv345<-2147483648ll||nv345>2147483647ll)FAIL(32);
uint32_t v345=(uint32_t)nv345;
uint32_t v346=get32(state+198);
int64_t nv347=(int64_t)(v346);
if(nv347<-2147483648ll||nv347>2147483647ll)FAIL(32);
uint32_t v347=(uint32_t)nv347;
int64_t nv348=(int64_t)signed32(v345)-(int64_t)signed32(v347);
if(nv348 < -2147483648ll || nv348 >2147483647ll)FAIL(2);
uint32_t v348=(uint32_t)nv348;
put32(state+166,v348);
uint32_t v349=get32(state+166);
uint32_t v350=get32(state+186);
uint32_t v351=v350;
if(!v351)FAIL(4);
if(v349==2147483648u && v351==4294967295u)FAIL(6);
uint32_t v352=(uint32_t)(signed32(v349)/signed32(v351)),v353=(uint32_t)(signed32(v349)%signed32(v351));
put32(state+170,v352);
put32(state+174,v353);
uint32_t v354=get32(state+174);
put32(state+214,v354);
uint32_t v355=get32(state+214);
uint32_t v356=(signed32(v355)<signed32(0u))?1u:0u;
if(v356){
uint32_t v357=get32(state+214);
uint32_t v358=get32(state+186);
int64_t nv359=(int64_t)signed32(v357)+(int64_t)signed32(v358);
if(nv359 < -2147483648ll || nv359 >2147483647ll)FAIL(1);
uint32_t v359=(uint32_t)nv359;
put32(state+214,v359);
}else{
}
uint32_t v360=get32(state+186);
uint32_t v361=2u;
if(!v361)FAIL(4);
if(v360==2147483648u && v361==4294967295u)FAIL(6);
uint32_t v362=(uint32_t)(signed32(v360)/signed32(v361)),v363=(uint32_t)(signed32(v360)%signed32(v361));
put32(state+222,v362);
put32(state+174,v363);
uint32_t v364=get32(state+214);
uint32_t v365=get32(state+222);
uint32_t v366=(signed32(v364)>signed32(v365))?1u:0u;
if(v366){
uint32_t v367=get32(state+214);
uint32_t v368=get32(state+186);
int64_t nv369=(int64_t)signed32(v367)-(int64_t)signed32(v368);
if(nv369 < -2147483648ll || nv369 >2147483647ll)FAIL(2);
uint32_t v369=(uint32_t)nv369;
put32(state+214,v369);
}else{
}
uint32_t v370=get32(state+214);
put32(state+166,v370);
uint32_t v371=get32(state+166);
uint32_t v372=(signed32(v371)<signed32(0u))?1u:0u;
if(v372){
uint32_t v373=get32(state+166);
int64_t nv374=(int64_t)signed32(0u)-(int64_t)signed32(v373);
if(nv374 < -2147483648ll || nv374 >2147483647ll)FAIL(2);
uint32_t v374=(uint32_t)nv374;
put32(state+166,v374);
}else{
}
uint32_t v375=get32(state+210);
uint32_t v376=get32(state+202);
int64_t nv377=(int64_t)signed32(v375)-(int64_t)signed32(v376);
if(nv377 < -2147483648ll || nv377 >2147483647ll)FAIL(2);
uint32_t v377=(uint32_t)nv377;
put32(state+218,v377);
uint32_t v378=get32(state+218);
uint32_t v379=(signed32(v378)<signed32(0u))?1u:0u;
if(v379){
uint32_t v380=get32(state+218);
int64_t nv381=(int64_t)signed32(0u)-(int64_t)signed32(v380);
if(nv381 < -2147483648ll || nv381 >2147483647ll)FAIL(2);
uint32_t v381=(uint32_t)nv381;
put32(state+218,v381);
}else{
}
uint32_t v382=get32(state+166);
uint32_t v383=(signed32(v382)<signed32(3200u))?1u:0u;
uint32_t v384=v383;
if(v384){
uint32_t v385=get32(state+218);
uint32_t v386=(signed32(v385)<signed32(6144u))?1u:0u;
v384=v386;
}
if(v384>1u)FAIL(32);
put(state+226,v384);
uint32_t v387=get(state+226);
if(v387){
uint32_t v388=get(state+112);
int32_t index389=(int32_t)(v388);
if(index389<0||index389>5)FAIL(5);
{vdplet_view_v1 store390={sizeof(store390),0,0,0,0};
rc=h->buffer_acquire(h->context,506,VDPL_WRITE,(uint32_t)(index389-(0))*2u,2,&store390);
if(rc){ROOT_FAIL(8,rc);}
put(store390.data,1u);
rc=h->buffer_release(h->context,store390.lease);if(rc){ROOT_FAIL(8,rc);}}
if(1u>1u)FAIL(32);
put(state+228,1u);
}else{
}
}else{
}
}
uint32_t v391=get32(state+136);
uint64_t nv392=(uint64_t)(v391)+(uint64_t)(1u);
if(nv392>4294967295ull)FAIL(1);
uint32_t v392=(uint32_t)nv392;
put32(state+136,v392);
break;}
case 4:{
uint32_t v393=get32(state+182);
if((int64_t)(signed32(v393))<1ll)FAIL(32);
if((int64_t)(signed32(v393))>3276800ll)FAIL(32);
put32(state+186,v393);
uint32_t v394=get32(state+190);
uint32_t v395=get32(state+186);
uint32_t v396=v395;
if(!v396)FAIL(4);
if(v394==2147483648u && v396==4294967295u)FAIL(6);
uint32_t v397=(uint32_t)(signed32(v394)/signed32(v396)),v398=(uint32_t)(signed32(v394)%signed32(v396));
put32(state+170,v397);
put32(state+174,v398);
uint32_t v399=get32(state+174);
uint32_t v400=(signed32(v399)<signed32(0u))?1u:0u;
if(v400){
uint32_t v401=get32(state+174);
uint32_t v402=get32(state+186);
int64_t nv403=(int64_t)signed32(v401)+(int64_t)signed32(v402);
if(nv403 < -2147483648ll || nv403 >2147483647ll)FAIL(1);
uint32_t v403=(uint32_t)nv403;
put32(state+174,v403);
}else{
}
uint32_t v404=get32(state+174);
int64_t nv405=(int64_t)(signed32(v404));
if(nv405<0ll||nv405>4294967295ll)FAIL(32);
uint32_t v405=(uint32_t)nv405;
put32(state+194,v405);
break;}
case 5:{
uint32_t v406=get32(state+240);
uint32_t v407=get32(state+244);
int64_t nv408=(int64_t)signed32(v406)*(int64_t)signed32(v407);
if(nv408 < -2147483648ll || nv408 >2147483647ll)FAIL(3);
uint32_t v408=(uint32_t)nv408;
put32(state+248,v408);
break;}
case 6:{
uint32_t v409=get32(state+256);
uint32_t v410=get32(state+260);
uint64_t nv411=(uint64_t)(v409)*(uint64_t)(v410);
if(nv411>4294967295ull)FAIL(3);
uint32_t v411=(uint32_t)nv411;
put32(state+264,v411);
break;}
case 7:{
uint32_t v412=get(state+280);
uint32_t v413=(uint32_t)(v412)+(uint32_t)(1u);
if(v413>65535u)FAIL(1);
put(state+280,v413);
uint32_t v414=get32(state+240);
uint32_t v415=get32(state+244);
uint32_t v416=v415;
if(!v416)FAIL(4);
if(v414==2147483648u && v416==4294967295u)FAIL(6);
uint32_t v417=(uint32_t)(signed32(v414)/signed32(v416)),v418=(uint32_t)(signed32(v414)%signed32(v416));
put32(state+248,v417);
put32(state+252,v418);
break;}
case 8:{
uint32_t v419=get(state+280);
uint32_t v420=(uint32_t)(v419)+(uint32_t)(1u);
if(v420>65535u)FAIL(1);
put(state+280,v420);
uint32_t v421=get32(state+256);
uint32_t v422=get32(state+260);
uint32_t v423=v422;
if(!v423)FAIL(4);
uint32_t v424=v421/v423,v425=v421%v423;
put32(state+264,v424);
put32(state+268,v425);
break;}
case 9:{
uint32_t v426=get(state+280);
uint32_t v427=(uint32_t)(v426)+(uint32_t)(1u);
if(v427>65535u)FAIL(1);
put(state+280,v427);
uint32_t v428=get32(state+240);
uint32_t v429=get32(state+244);
uint32_t v430=v429;
if(!v430)FAIL(4);
if(v428==2147483648u && v430==4294967295u)FAIL(6);
uint32_t v431=(uint32_t)(signed32(v428)/signed32(v430)),v432=(uint32_t)(signed32(v428)%signed32(v430));
if((int64_t)(signed32(v431))<-10ll)FAIL(32);
if((int64_t)(signed32(v431))>10ll)FAIL(32);
if((int64_t)(signed32(v432))<-3ll)FAIL(32);
if((int64_t)(signed32(v432))>3ll)FAIL(32);
put32(state+272,v431);
put32(state+276,v432);
break;}
case 10:{
uint32_t v433=get32(state+240);
uint32_t v434=get32(state+244);
uint32_t v435=v434;
if(!v435)FAIL(4);
if(v433==2147483648u && v435==4294967295u)FAIL(6);
uint32_t v436=(uint32_t)(signed32(v433)/signed32(v435)),v437=(uint32_t)(signed32(v433)%signed32(v435));
put32(state+240,v436);
put32(state+244,v437);
break;}
case 11:{
uint32_t v438=get32(state+240);
uint32_t v439=get(state+282);
if(v439>31u)FAIL(32);
uint32_t v440=v438>>v439;
if((v438&2147483648u)&&v439)v440|=4294967295u<<(32u-v439);
put32(state+284,v440);
break;}
case 12:{
uint32_t v441=get32(state+256);
uint32_t v442=get(state+282);
if(v442>31u)FAIL(32);
uint32_t v443=v441>>v442;
put32(state+288,v443);
break;}
case 13:{
put(state+114,0u);
put(state+116,60u);
put32(state+120,0u);
put32(state+124,0u);
put32(state+128,0u);
put32(state+136,0u);
break;}
case 14:{
if(0u>1u)FAIL(32);
put(state+228,0u);
break;}
}
put32(state+20,get32(state+20)+1u);put(state+24,r->entry_id);put32(state+32,get32(state+8));
h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return 0;
}
