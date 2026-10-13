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
if(r->operation!=VDPL_RUN||r->entry_id>=14)return VDPL_E_UNSUPPORTED;
if(r->argument_bytes!=2)return VDPL_E_ARGUMENT;
vdplet_view_v1 args={sizeof(args),0,0,0,0},view={sizeof(view),0,0,0,0};
int32_t rc=h->buffer_acquire(h->context,r->argument_buffer,VDPL_READ,0,2,&args);if(rc)return rc;
uint16_t id=get(args.data);rc=h->buffer_acquire(h->context,id,VDPL_WRITE,0,VDPL_WHOLE_BUFFER,&view);if(rc){h->buffer_release(h->context,args.lease);return rc;}
if(view.bytes!=156){h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return VDPL_E_BOUNDS;}
uint8_t*state=view.data;put(state+12,0);
static const uint8_t offsets[] __attribute__((aligned(4)))={0,1,2,3,4,5,6,7,26,27,30,31,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,};
static const uint8_t values[] __attribute__((aligned(4)))={82,71,1,0,112,0,156,0,4,0,0,0,14,0,44,0,0,0,0,0,48,53,50,82,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
static const uint8_t errors[] __attribute__((aligned(4)))={1,1,2,2,3,3,3,3,3,3,6,6,3,3,3,3,6,6,6,6,6,6,6,6,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,};
for(unsigned i=0;i<sizeof(offsets);++i)if(state[offsets[i]]!=values[i])ROOT_FAIL(errors[i],VDPL_E_ARGUMENT);
if(!get32(state+8))ROOT_FAIL(4,VDPL_E_ARGUMENT);
if(get(state+28)>=14u||get(state+28)!=r->entry_id)ROOT_FAIL(7,VDPL_E_ARGUMENT);
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,256,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=8u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,257,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=8u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
put(state+14,0);
if(get(state+112)>1u)FAIL(32);
if((int64_t)(get32(state+150))>11796120ll)FAIL(32);
switch(r->entry_id){
case 0:{
uint32_t v0=get32(state+114);
uint32_t v1=get32(state+118);
uint64_t nv2=(uint64_t)(v0)*(uint64_t)(v1);
if(nv2>4294967295ull)FAIL(3);
uint32_t v2=(uint32_t)nv2;
put32(state+126,v2);
break;}
case 1:{
uint32_t v3=get32(state+130);
uint32_t v4=get32(state+134);
int64_t nv5=(int64_t)signed32(v3)*(int64_t)signed32(v4);
if(nv5 < -2147483648ll || nv5 >2147483647ll)FAIL(3);
uint32_t v5=(uint32_t)nv5;
put32(state+142,v5);
break;}
case 2:{
uint32_t v6=get32(state+114);
uint32_t v7=get32(state+118);
uint64_t nv8=(uint64_t)(v6)*(uint64_t)(v7);
if(nv8>4294967295ull)FAIL(3);
uint32_t v8=(uint32_t)nv8;
uint32_t v9=get32(state+122);
uint64_t nv10=(uint64_t)(v9)*(uint64_t)(2u);
if(nv10>4294967295ull)FAIL(3);
uint32_t v10=(uint32_t)nv10;
uint64_t nv11=(uint64_t)(v8)*(uint64_t)(v10);
if(nv11>4294967295ull)FAIL(3);
uint32_t v11=(uint32_t)nv11;
put32(state+126,v11);
break;}
case 3:{
uint32_t v12=get32(state+130);
uint32_t v13=get32(state+134);
int64_t nv14=(int64_t)signed32(v12)*(int64_t)signed32(v13);
if(nv14 < -2147483648ll || nv14 >2147483647ll)FAIL(3);
uint32_t v14=(uint32_t)nv14;
uint32_t v15=get32(state+138);
int64_t nv16=(int64_t)signed32(v15)*(int64_t)signed32(4294967294u);
if(nv16 < -2147483648ll || nv16 >2147483647ll)FAIL(3);
uint32_t v16=(uint32_t)nv16;
int64_t nv17=(int64_t)signed32(v14)*(int64_t)signed32(v16);
if(nv17 < -2147483648ll || nv17 >2147483647ll)FAIL(3);
uint32_t v17=(uint32_t)nv17;
put32(state+142,v17);
break;}
case 4:{
uint32_t v18=get32(state+114);
uint32_t v19=get32(state+118);
uint32_t v20=get32(state+122);
uint64_t nv21=(uint64_t)(v19)*(uint64_t)(v20);
if(nv21>4294967295ull)FAIL(3);
uint32_t v21=(uint32_t)nv21;
uint64_t nv22=(uint64_t)(v18)+(uint64_t)(v21);
if(nv22>4294967295ull)FAIL(1);
uint32_t v22=(uint32_t)nv22;
put32(state+126,v22);
break;}
case 5:{
uint32_t v23=get32(state+114);
uint64_t nv24=(uint64_t)(v23)+(uint64_t)(1u);
if(nv24>4294967295ull)FAIL(1);
uint32_t v24=(uint32_t)nv24;
put32(state+126,v24);
uint32_t v25=get(state+148);
int32_t index26=(int32_t)(v25);
if(index26<0||index26>1)FAIL(5);
uint32_t v28=get32(state+114);
uint32_t v29=get32(state+118);
uint64_t nv30=(uint64_t)(v28)*(uint64_t)(v29);
if(nv30>4294967295ull)FAIL(3);
uint32_t v30=(uint32_t)nv30;
{vdplet_view_v1 store27={sizeof(store27),0,0,0,0};
rc=h->buffer_acquire(h->context,256,VDPL_WRITE,(uint32_t)(index26-(0))*4u,4,&store27);
if(rc){ROOT_FAIL(8,rc);}
put32(store27.data,v30);
rc=h->buffer_release(h->context,store27.lease);if(rc){ROOT_FAIL(8,rc);}}
break;}
case 6:{
uint32_t v31=get(state+148);
int32_t index32=(int32_t)(v31);
if(index32<0||index32>1)FAIL(5);
uint32_t v34=get32(state+114);
uint32_t v35=get32(state+118);
uint64_t nv36=(uint64_t)(v34)*(uint64_t)(v35);
if(nv36>4294967295ull)FAIL(3);
uint32_t v36=(uint32_t)nv36;
{vdplet_view_v1 store33={sizeof(store33),0,0,0,0};
rc=h->buffer_acquire(h->context,256,VDPL_WRITE,(uint32_t)(index32-(0))*4u,4,&store33);
if(rc){ROOT_FAIL(8,rc);}
put32(store33.data,v36);
rc=h->buffer_release(h->context,store33.lease);if(rc){ROOT_FAIL(8,rc);}}
break;}
case 7:{
uint32_t v37=get(state+154);
int32_t index38=signed16(v37);
if(index38<-1||index38>0)FAIL(5);
uint32_t v40=get32(state+130);
uint32_t v41=get32(state+134);
int64_t nv42=(int64_t)signed32(v40)*(int64_t)signed32(v41);
if(nv42 < -2147483648ll || nv42 >2147483647ll)FAIL(3);
uint32_t v42=(uint32_t)nv42;
{vdplet_view_v1 store39={sizeof(store39),0,0,0,0};
rc=h->buffer_acquire(h->context,257,VDPL_WRITE,(uint32_t)(index38-(-1))*4u,4,&store39);
if(rc){ROOT_FAIL(8,rc);}
put32(store39.data,v42);
rc=h->buffer_release(h->context,store39.lease);if(rc){ROOT_FAIL(8,rc);}}
break;}
case 8:{
uint32_t v43=0u;
if(v43){
uint32_t v44=get32(state+114);
uint32_t v45=get32(state+118);
uint64_t nv46=(uint64_t)(v44)*(uint64_t)(v45);
if(nv46>4294967295ull)FAIL(3);
uint32_t v46=(uint32_t)nv46;
uint32_t v47=(v46==0u)?1u:0u;
v43=v47;
}
if(v43>1u)FAIL(32);
put(state+112,v43);
break;}
case 9:{
uint32_t v48=get(state+146);
uint32_t v49=(uint32_t)(v48)*(uint32_t)(2u);
if(v49>65535u)FAIL(3);
int64_t nv50=(int64_t)(v49);
if(nv50<0ll||nv50>4294967295ll)FAIL(32);
uint32_t v50=(uint32_t)nv50;
put32(state+126,v50);
break;}
case 10:{
uint32_t v51=get(state+146);
int64_t nv52=(int64_t)(v51);
if(nv52<0ll||nv52>4294967295ll)FAIL(32);
uint32_t v52=(uint32_t)nv52;
uint64_t nv53=(uint64_t)(v52)*(uint64_t)(2u);
if(nv53>4294967295ull)FAIL(3);
uint32_t v53=(uint32_t)nv53;
put32(state+126,v53);
break;}
case 11:{
uint32_t v54=get32(state+114);
uint32_t v55=get32(state+118);
uint64_t nv56=(uint64_t)(v54)*(uint64_t)(v55);
if(nv56>4294967295ull)FAIL(3);
uint32_t v56=(uint32_t)nv56;
if((int64_t)(v56)>11796120ll)FAIL(32);
put32(state+150,v56);
break;}
case 12:{
put32(state+126,4294967295u);
put32(state+142,2147483648u);
break;}
case 13:{
uint32_t v57=get32(state+114);
uint32_t v58=get32(state+118);
uint64_t nv59=(uint64_t)(v57)*(uint64_t)(v58);
if(nv59>4294967295ull)FAIL(3);
uint32_t v59=(uint32_t)nv59;
uint32_t v60=get32(state+130);
int64_t nv61=(int64_t)(signed32(v60));
if(nv61<-32768ll||nv61>32767ll)FAIL(32);
uint32_t v61=(uint32_t)nv61&65535u;
int64_t nv62=(int64_t)(signed16(v61));
if(nv62<0ll||nv62>4294967295ll)FAIL(32);
uint32_t v62=(uint32_t)nv62;
uint64_t nv63=(uint64_t)(v59)*(uint64_t)(v62);
if(nv63>4294967295ull)FAIL(3);
uint32_t v63=(uint32_t)nv63;
put32(state+126,v63);
break;}
}
put32(state+20,get32(state+20)+1u);put(state+24,r->entry_id);put32(state+32,get32(state+8));
h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return 0;
}
