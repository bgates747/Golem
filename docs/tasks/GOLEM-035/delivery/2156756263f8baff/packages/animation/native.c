/* Generated from the shared Golem model; module state stays in VDP buffers. */
#include "vdplet.h"
static volatile uint32_t identity=0x474c3332u;
static uint16_t get(const uint8_t*p){return (uint16_t)p[0]|(uint16_t)p[1]<<8;}
static void put(uint8_t*p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static inline int32_t signed16(uint32_t v){return (int32_t)v-((v&32768u)?65536:0); }
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
if(r->operation!=VDPL_RUN||r->entry_id>=4)return VDPL_E_UNSUPPORTED;
if(r->argument_bytes!=2)return VDPL_E_ARGUMENT;
vdplet_view_v1 args={sizeof(args),0,0,0,0},view={sizeof(view),0,0,0,0};
int32_t rc=h->buffer_acquire(h->context,r->argument_buffer,VDPL_READ,0,2,&args);if(rc)return rc;
uint16_t id=get(args.data);rc=h->buffer_acquire(h->context,id,VDPL_WRITE,0,VDPL_WHOLE_BUFFER,&view);if(rc){h->buffer_release(h->context,args.lease);return rc;}
if(view.bytes!=142){h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return VDPL_E_BOUNDS;}
uint8_t*state=view.data;put(state+12,0);
static const uint8_t offsets[] __attribute__((aligned(4)))={0,1,2,3,4,5,6,7,26,27,30,31,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,};
static const uint8_t values[] __attribute__((aligned(4)))={82,71,1,0,112,0,142,0,4,0,0,0,4,0,30,0,0,0,0,0,48,53,50,82,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
static const uint8_t errors[] __attribute__((aligned(4)))={1,1,2,2,3,3,3,3,3,3,6,6,3,3,3,3,6,6,6,6,6,6,6,6,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,};
for(unsigned i=0;i<sizeof(offsets);++i)if(state[offsets[i]]!=values[i])ROOT_FAIL(errors[i],VDPL_E_ARGUMENT);
if(!get32(state+8))ROOT_FAIL(4,VDPL_E_ARGUMENT);
if(get(state+28)>=4u||get(state+28)!=r->entry_id)ROOT_FAIL(7,VDPL_E_ARGUMENT);
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,2,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=12u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,3,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=12u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,4,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=12u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,5,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=12u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,6,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=12u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,7,1,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=12u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,8,1,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=12u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,9,1,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=12u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,10,1,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=12u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,11,1,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=12u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,12,1,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=514u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
put(state+14,0);
switch(r->entry_id){
case 0:{
for(uint32_t loop0=0;loop0<6u;++loop0){
put(state+112,(uint16_t)(0+(int32_t)loop0*(1)));
uint32_t v1=get(state+112);
int32_t index2=(int32_t)(v1);
if(index2<0||index2>5)FAIL(5);
uint32_t v3;{vdplet_view_v1 av3={sizeof(av3),0,0,0,0};
rc=h->buffer_acquire(h->context,9,VDPL_READ,(uint32_t)(index2-(0))*2u,2,&av3);
if(rc){ROOT_FAIL(8,rc);}
v3=get(av3.data);
rc=h->buffer_release(h->context,av3.lease);if(rc){ROOT_FAIL(8,rc);}}
if(v3>32767u)FAIL(32);
uint32_t v4=60u;
if(!v4)FAIL(4);
uint32_t v5=v3/v4,v6=v3%v4;
put(state+124,v5);
put(state+122,v6);
uint32_t v7=get(state+112);
int32_t index8=(int32_t)(v7);
if(index8<0||index8>5)FAIL(5);
uint32_t v10=get(state+124);
if(v10>32767u)FAIL(32);
{vdplet_view_v1 store9={sizeof(store9),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index8-(0))*2u,2,&store9);
if(rc){ROOT_FAIL(8,rc);}
put(store9.data,v10);
rc=h->buffer_release(h->context,store9.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v11=get(state+112);
uint32_t v12=(v11<3u)?1u:0u;
if(v12){
uint32_t v13=get(state+112);
int32_t index14=(int32_t)(v13);
if(index14<0||index14>5)FAIL(5);
uint32_t v16=get(state+112);
int32_t index17=(int32_t)(v16);
if(index17<0||index17>5)FAIL(5);
uint32_t v18;{vdplet_view_v1 av18={sizeof(av18),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_READ,(uint32_t)(index17-(0))*2u,2,&av18);
if(rc){ROOT_FAIL(8,rc);}
v18=get(av18.data);
rc=h->buffer_release(h->context,av18.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t sv19=signed16(0u)-signed16(v18);
if(sv19 < -32768 || sv19 >32767)FAIL(2);
uint32_t v19=(uint32_t)sv19&65535u;
{vdplet_view_v1 store15={sizeof(store15),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index14-(0))*2u,2,&store15);
if(rc){ROOT_FAIL(8,rc);}
put(store15.data,v19);
rc=h->buffer_release(h->context,store15.lease);if(rc){ROOT_FAIL(8,rc);}}
}else{
}
}
break;}
case 1:{
for(uint32_t loop20=0;loop20<6u;++loop20){
put(state+112,(uint16_t)(0+(int32_t)loop20*(1)));
uint32_t v21=get(state+112);
int32_t index22=(int32_t)(v21);
if(index22<0||index22>5)FAIL(5);
uint32_t v23;{vdplet_view_v1 av23={sizeof(av23),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_READ,(uint32_t)(index22-(0))*2u,2,&av23);
if(rc){ROOT_FAIL(8,rc);}
v23=get(av23.data);
rc=h->buffer_release(h->context,av23.lease);if(rc){ROOT_FAIL(8,rc);}}
if(v23>32767u)FAIL(32);
uint32_t v24=get(state+112);
int32_t index25=(int32_t)(v24);
if(index25<0||index25>5)FAIL(5);
uint32_t v26;{vdplet_view_v1 av26={sizeof(av26),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_READ,(uint32_t)(index25-(0))*2u,2,&av26);
if(rc){ROOT_FAIL(8,rc);}
v26=get(av26.data);
rc=h->buffer_release(h->context,av26.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t sv27=signed16(v23)+signed16(v26);
if(sv27 < -32768 || sv27 >32767)FAIL(1);
uint32_t v27=(uint32_t)sv27&65535u;
put(state+130,v27);
uint32_t v28=get(state+130);
uint32_t v29=(signed16(v28)<signed16(16u))?1u:0u;
uint32_t v30=v29;
if(!v30){
uint32_t v31=get(state+130);
uint32_t v32=(signed16(v31)>signed16(288u))?1u:0u;
v30=v32;
}
if(v30){
uint32_t v33=get(state+112);
int32_t index34=(int32_t)(v33);
if(index34<0||index34>5)FAIL(5);
uint32_t v36=get(state+112);
int32_t index37=(int32_t)(v36);
if(index37<0||index37>5)FAIL(5);
uint32_t v38;{vdplet_view_v1 av38={sizeof(av38),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_READ,(uint32_t)(index37-(0))*2u,2,&av38);
if(rc){ROOT_FAIL(8,rc);}
v38=get(av38.data);
rc=h->buffer_release(h->context,av38.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t sv39=signed16(0u)-signed16(v38);
if(sv39 < -32768 || sv39 >32767)FAIL(2);
uint32_t v39=(uint32_t)sv39&65535u;
{vdplet_view_v1 store35={sizeof(store35),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index34-(0))*2u,2,&store35);
if(rc){ROOT_FAIL(8,rc);}
put(store35.data,v39);
rc=h->buffer_release(h->context,store35.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v40=get(state+112);
int32_t index41=(int32_t)(v40);
if(index41<0||index41>5)FAIL(5);
uint32_t v42;{vdplet_view_v1 av42={sizeof(av42),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_READ,(uint32_t)(index41-(0))*2u,2,&av42);
if(rc){ROOT_FAIL(8,rc);}
v42=get(av42.data);
rc=h->buffer_release(h->context,av42.lease);if(rc){ROOT_FAIL(8,rc);}}
if(v42>32767u)FAIL(32);
uint32_t v43=get(state+112);
int32_t index44=(int32_t)(v43);
if(index44<0||index44>5)FAIL(5);
uint32_t v45;{vdplet_view_v1 av45={sizeof(av45),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_READ,(uint32_t)(index44-(0))*2u,2,&av45);
if(rc){ROOT_FAIL(8,rc);}
v45=get(av45.data);
rc=h->buffer_release(h->context,av45.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t sv46=signed16(v42)+signed16(v45);
if(sv46 < -32768 || sv46 >32767)FAIL(1);
uint32_t v46=(uint32_t)sv46&65535u;
put(state+130,v46);
}else{
}
uint32_t v47=get(state+112);
int32_t index48=(int32_t)(v47);
if(index48<0||index48>5)FAIL(5);
uint32_t v50=get(state+130);
if(v50>32767u)FAIL(32);
{vdplet_view_v1 store49={sizeof(store49),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_WRITE,(uint32_t)(index48-(0))*2u,2,&store49);
if(rc){ROOT_FAIL(8,rc);}
put(store49.data,v50);
rc=h->buffer_release(h->context,store49.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v51=get(state+112);
int32_t index52=(int32_t)(v51);
if(index52<0||index52>5)FAIL(5);
uint32_t v53;{vdplet_view_v1 av53={sizeof(av53),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_READ,(uint32_t)(index52-(0))*2u,2,&av53);
if(rc){ROOT_FAIL(8,rc);}
v53=get(av53.data);
rc=h->buffer_release(h->context,av53.lease);if(rc){ROOT_FAIL(8,rc);}}
put(state+116,v53);
uint32_t v54=get(state+116);
uint32_t v55=(v54==511u)?1u:0u;
if(v55){
put(state+116,0u);
}else{
uint32_t v56=get(state+116);
uint32_t v57=(uint32_t)(v56)+(uint32_t)(1u);
if(v57>65535u)FAIL(1);
put(state+116,v57);
}
uint32_t v58=get(state+112);
int32_t index59=(int32_t)(v58);
if(index59<0||index59>5)FAIL(5);
uint32_t v61=get(state+116);
{vdplet_view_v1 store60={sizeof(store60),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_WRITE,(uint32_t)(index59-(0))*2u,2,&store60);
if(rc){ROOT_FAIL(8,rc);}
put(store60.data,v61);
rc=h->buffer_release(h->context,store60.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v62=get(state+116);
uint32_t v63=(v62>256u)?1u:0u;
if(v63){
uint32_t v64=get(state+116);
if((uint32_t)(512u)<(uint32_t)(v64))FAIL(2);
uint32_t v65=(uint32_t)(512u)-(uint32_t)(v64);
put(state+114,v65);
}else{
uint32_t v66=get(state+116);
put(state+114,v66);
}
uint32_t v67=get(state+114);
int32_t index68=(int32_t)(v67);
if(index68<0||index68>256)FAIL(5);
uint32_t v69;{vdplet_view_v1 av69={sizeof(av69),0,0,0,0};
rc=h->buffer_acquire(h->context,12,VDPL_READ,(uint32_t)(index68-(0))*2u,2,&av69);
if(rc){ROOT_FAIL(8,rc);}
v69=get(av69.data);
rc=h->buffer_release(h->context,av69.lease);if(rc){ROOT_FAIL(8,rc);}}
if(v69>32767u)FAIL(32);
put(state+118,v69);
uint32_t v70=get(state+118);
uint32_t v71=4096u;
if(!v71)FAIL(4);
uint32_t v72=v70/v71,v73=v70%v71;
put(state+120,v72);
put(state+122,v73);
uint32_t v74=get(state+112);
int32_t index75=(int32_t)(v74);
if(index75<0||index75>5)FAIL(5);
uint32_t v77=get(state+112);
int32_t index78=(int32_t)(v77);
if(index78<0||index78>5)FAIL(5);
uint32_t v79;{vdplet_view_v1 av79={sizeof(av79),0,0,0,0};
rc=h->buffer_acquire(h->context,7,VDPL_READ,(uint32_t)(index78-(0))*2u,2,&av79);
if(rc){ROOT_FAIL(8,rc);}
v79=get(av79.data);
rc=h->buffer_release(h->context,av79.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v80=get(state+120);
uint32_t v81=(uint32_t)(v79)+(uint32_t)(v80);
if(v81>65535u)FAIL(1);
{vdplet_view_v1 store76={sizeof(store76),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_WRITE,(uint32_t)(index75-(0))*2u,2,&store76);
if(rc){ROOT_FAIL(8,rc);}
put(store76.data,v81);
rc=h->buffer_release(h->context,store76.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v82=get(state+112);
int32_t index83=(int32_t)(v82);
if(index83<0||index83>5)FAIL(5);
{vdplet_view_v1 store84={sizeof(store84),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index83-(0))*2u,2,&store84);
if(rc){ROOT_FAIL(8,rc);}
put(store84.data,12u);
rc=h->buffer_release(h->context,store84.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v85=get(state+112);
int32_t index86=(int32_t)(v85);
if(index86<0||index86>5)FAIL(5);
uint32_t v87;{vdplet_view_v1 av87={sizeof(av87),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_READ,(uint32_t)(index86-(0))*2u,2,&av87);
if(rc){ROOT_FAIL(8,rc);}
v87=get(av87.data);
rc=h->buffer_release(h->context,av87.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v88=(signed16(v87)<signed16(0u))?1u:0u;
if(v88){
uint32_t v89=get(state+112);
int32_t index90=(int32_t)(v89);
if(index90<0||index90>5)FAIL(5);
{vdplet_view_v1 store91={sizeof(store91),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index90-(0))*2u,2,&store91);
if(rc){ROOT_FAIL(8,rc);}
put(store91.data,48u);
rc=h->buffer_release(h->context,store91.lease);if(rc){ROOT_FAIL(8,rc);}}
}else{
}
}
uint32_t v92=get(state+126);
uint32_t v93=(uint32_t)(v92)+(uint32_t)(1u);
if(v93>65535u)FAIL(1);
put(state+126,v93);
break;}
case 2:{
uint32_t v94=get(state+128);
int32_t index95=(int32_t)(v94);
if(index95<0||index95>5)FAIL(5);
uint32_t v96;{vdplet_view_v1 av96={sizeof(av96),0,0,0,0};
rc=h->buffer_acquire(h->context,8,VDPL_READ,(uint32_t)(index95-(0))*2u,2,&av96);
if(rc){ROOT_FAIL(8,rc);}
v96=get(av96.data);
rc=h->buffer_release(h->context,av96.lease);if(rc){ROOT_FAIL(8,rc);}}
put(state+132,v96);
uint32_t v97=get(state+128);
int32_t index98=(int32_t)(v97);
if(index98<0||index98>5)FAIL(5);
uint32_t v99;{vdplet_view_v1 av99={sizeof(av99),0,0,0,0};
rc=h->buffer_acquire(h->context,9,VDPL_READ,(uint32_t)(index98-(0))*2u,2,&av99);
if(rc){ROOT_FAIL(8,rc);}
v99=get(av99.data);
rc=h->buffer_release(h->context,av99.lease);if(rc){ROOT_FAIL(8,rc);}}
put(state+134,v99);
uint32_t v100=get(state+128);
int32_t index101=(int32_t)(v100);
if(index101<0||index101>5)FAIL(5);
uint32_t v102;{vdplet_view_v1 av102={sizeof(av102),0,0,0,0};
rc=h->buffer_acquire(h->context,10,VDPL_READ,(uint32_t)(index101-(0))*2u,2,&av102);
if(rc){ROOT_FAIL(8,rc);}
v102=get(av102.data);
rc=h->buffer_release(h->context,av102.lease);if(rc){ROOT_FAIL(8,rc);}}
put(state+136,v102);
uint32_t v103=get(state+128);
int32_t index104=(int32_t)(v103);
if(index104<0||index104>5)FAIL(5);
uint32_t v105;{vdplet_view_v1 av105={sizeof(av105),0,0,0,0};
rc=h->buffer_acquire(h->context,11,VDPL_READ,(uint32_t)(index104-(0))*2u,2,&av105);
if(rc){ROOT_FAIL(8,rc);}
v105=get(av105.data);
rc=h->buffer_release(h->context,av105.lease);if(rc){ROOT_FAIL(8,rc);}}
put(state+138,v105);
break;}
case 3:{
uint32_t v106=get(state+114);
int32_t index107=(int32_t)(v106);
if(index107<0||index107>256)FAIL(5);
uint32_t v108;{vdplet_view_v1 av108={sizeof(av108),0,0,0,0};
rc=h->buffer_acquire(h->context,12,VDPL_READ,(uint32_t)(index107-(0))*2u,2,&av108);
if(rc){ROOT_FAIL(8,rc);}
v108=get(av108.data);
rc=h->buffer_release(h->context,av108.lease);if(rc){ROOT_FAIL(8,rc);}}
put(state+140,v108);
break;}
}
put32(state+20,get32(state+20)+1u);put(state+24,r->entry_id);put32(state+32,get32(state+8));
h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return 0;
}
