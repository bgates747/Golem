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
if(r->operation!=VDPL_RUN||r->entry_id>=5)return VDPL_E_UNSUPPORTED;
if(r->argument_bytes!=2)return VDPL_E_ARGUMENT;
vdplet_view_v1 args={sizeof(args),0,0,0,0},view={sizeof(view),0,0,0,0};
int32_t rc=h->buffer_acquire(h->context,r->argument_buffer,VDPL_READ,0,2,&args);if(rc)return rc;
uint16_t id=get(args.data);rc=h->buffer_acquire(h->context,id,VDPL_WRITE,0,VDPL_WHOLE_BUFFER,&view);if(rc){h->buffer_release(h->context,args.lease);return rc;}
if(view.bytes!=126){h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return VDPL_E_BOUNDS;}
uint8_t*state=view.data;put(state+12,0);
static const uint8_t offsets[] __attribute__((aligned(4)))={0,1,2,3,4,5,6,7,26,27,30,31,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,};
static const uint8_t values[] __attribute__((aligned(4)))={82,71,1,0,112,0,126,0,4,0,0,0,5,0,14,0,0,0,0,0,48,53,50,82,1,0,32,78,2,0,0,0,16,0,0,0,1,0,0,0,2,0,33,78,0,0,0,0,8,0,0,0,3,0,0,0,3,0,34,78,4,0,0,0,18,0,0,0,1,0,0,0,4,0,35,78,0,0,0,0,4,0,0,0,1,0,0,0,};
static const uint8_t errors[] __attribute__((aligned(4)))={1,1,2,2,3,3,3,3,3,3,6,6,3,3,3,3,6,6,6,6,6,6,6,6,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,};
for(unsigned i=0;i<sizeof(offsets);++i)if(state[offsets[i]]!=values[i])ROOT_FAIL(errors[i],VDPL_E_ARGUMENT);
if(!get32(state+8))ROOT_FAIL(4,VDPL_E_ARGUMENT);
if(get(state+28)>=5u||get(state+28)!=r->entry_id)ROOT_FAIL(7,VDPL_E_ARGUMENT);
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,20000,1,2u,16u,&bound);
if(rc){ROOT_FAIL(8,rc);}
rc=h->buffer_release(h->context,bound.lease);if(rc){ROOT_FAIL(8,rc);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,20001,2,0u,8u,&bound);
if(rc){ROOT_FAIL(8,rc);}
rc=h->buffer_release(h->context,bound.lease);if(rc){ROOT_FAIL(8,rc);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,20002,1,4u,18u,&bound);
if(rc){ROOT_FAIL(8,rc);}
rc=h->buffer_release(h->context,bound.lease);if(rc){ROOT_FAIL(8,rc);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,20003,1,0u,4u,&bound);
if(rc){ROOT_FAIL(8,rc);}
rc=h->buffer_release(h->context,bound.lease);if(rc){ROOT_FAIL(8,rc);}}
put(state+14,0);
if(get(state+112)<32u)FAIL(32);
if(get(state+112)>224u)FAIL(32);
if(get(state+114)>1u)FAIL(32);
if(get(state+116)<32u)FAIL(32);
if(get(state+116)>224u)FAIL(32);
if(get(state+118)<48u)FAIL(32);
if(get(state+118)>240u)FAIL(32);
if(get(state+122)>63u)FAIL(32);
if(get(state+124)>1u)FAIL(32);
switch(r->entry_id){
case 0:{
if(32u<32u)FAIL(32);
if(32u>224u)FAIL(32);
put(state+112,32u);
if(1u>1u)FAIL(32);
put(state+114,1u);
if(32u<32u)FAIL(32);
if(32u>224u)FAIL(32);
put(state+116,32u);
if(48u<48u)FAIL(32);
if(48u>240u)FAIL(32);
put(state+118,48u);
put(state+120,0u);
if(12u>63u)FAIL(32);
put(state+122,12u);
if(1u>1u)FAIL(32);
put(state+124,1u);
break;}
case 1:{
uint32_t v0=get(state+124);
if(v0){
uint32_t v1=get(state+112);
uint32_t v2=(v1==224u)?1u:0u;
if(v2){
if(0u>1u)FAIL(32);
put(state+114,0u);
}else{
uint32_t v3=get(state+112);
uint32_t v4=(v3==32u)?1u:0u;
if(v4){
if(1u>1u)FAIL(32);
put(state+114,1u);
}else{
}
}
uint32_t v5=get(state+114);
uint32_t v6=(v5==1u)?1u:0u;
if(v6){
uint32_t v7=get(state+112);
uint32_t v8=(uint32_t)(v7)+(uint32_t)(2u);
if(v8>65535u)FAIL(1);
if(v8<32u)FAIL(32);
if(v8>224u)FAIL(32);
put(state+112,v8);
}else{
uint32_t v9=get(state+112);
if((uint32_t)(v9)<(uint32_t)(2u))FAIL(2);
uint32_t v10=(uint32_t)(v9)-(uint32_t)(2u);
if(v10<32u)FAIL(32);
if(v10>224u)FAIL(32);
put(state+112,v10);
}
uint32_t v11=get(state+120);
uint32_t v12=(uint32_t)(v11)+(uint32_t)(1u);
if(v12>65535u)FAIL(1);
put(state+120,v12);
}else{
}
break;}
case 2:{
uint32_t v13=get(state+112);
if(v13<32u)FAIL(32);
if(v13>224u)FAIL(32);
put(state+116,v13);
uint32_t v14=get(state+116);
uint32_t v15=(uint32_t)(v14)+(uint32_t)(16u);
if(v15>65535u)FAIL(1);
if(v15<48u)FAIL(32);
if(v15>240u)FAIL(32);
put(state+118,v15);
uint32_t v16=get(state+114);
uint32_t v17=(v16==1u)?1u:0u;
if(v17){
if(12u>63u)FAIL(32);
put(state+122,12u);
}else{
if(48u>63u)FAIL(32);
put(state+122,48u);
}
break;}
case 3:{
if(128u<32u)FAIL(32);
if(128u>224u)FAIL(32);
put(state+112,128u);
if(0u>1u)FAIL(32);
put(state+114,0u);
if(128u<32u)FAIL(32);
if(128u>224u)FAIL(32);
put(state+116,128u);
if(144u<48u)FAIL(32);
if(144u>240u)FAIL(32);
put(state+118,144u);
put(state+120,0u);
if(48u>63u)FAIL(32);
put(state+122,48u);
if(1u>1u)FAIL(32);
put(state+124,1u);
break;}
case 4:{
if(0u>1u)FAIL(32);
put(state+124,0u);
break;}
}
put32(state+20,get32(state+20)+1u);put(state+24,r->entry_id);put32(state+32,get32(state+8));
h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return 0;
}
