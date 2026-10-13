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
if(r->operation!=VDPL_RUN||r->entry_id>=10)return VDPL_E_UNSUPPORTED;
if(r->argument_bytes!=2)return VDPL_E_ARGUMENT;
vdplet_view_v1 args={sizeof(args),0,0,0,0},view={sizeof(view),0,0,0,0};
int32_t rc=h->buffer_acquire(h->context,r->argument_buffer,VDPL_READ,0,2,&args);if(rc)return rc;
uint16_t id=get(args.data);rc=h->buffer_acquire(h->context,id,VDPL_WRITE,0,VDPL_WHOLE_BUFFER,&view);if(rc){h->buffer_release(h->context,args.lease);return rc;}
if(view.bytes!=130){h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return VDPL_E_BOUNDS;}
uint8_t*state=view.data;put(state+12,0);
static const uint8_t offsets[] __attribute__((aligned(4)))={0,1,2,3,4,5,6,7,26,27,30,31,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,};
static const uint8_t values[] __attribute__((aligned(4)))={82,71,1,0,112,0,130,0,4,0,0,0,10,0,18,0,0,0,0,0,48,53,50,82,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
static const uint8_t errors[] __attribute__((aligned(4)))={1,1,2,2,3,3,3,3,3,3,6,6,3,3,3,3,6,6,6,6,6,6,6,6,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,};
for(unsigned i=0;i<sizeof(offsets);++i)if(state[offsets[i]]!=values[i])ROOT_FAIL(errors[i],VDPL_E_ARGUMENT);
if(!get32(state+8))ROOT_FAIL(4,VDPL_E_ARGUMENT);
if(get(state+28)>=10u||get(state+28)!=r->entry_id)ROOT_FAIL(7,VDPL_E_ARGUMENT);
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
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,258,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=12u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,259,1,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=514u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
put(state+14,0);
if(get(state+128)>1u)FAIL(32);
switch(r->entry_id){
case 0:{
uint32_t v0=get(state+114);
int32_t index1=signed16(v0);
if(index1<-2||index1>1)FAIL(5);
uint32_t v2;{vdplet_view_v1 av2={sizeof(av2),0,0,0,0};
rc=h->buffer_acquire(h->context,256,VDPL_READ,(uint32_t)(index1-(-2))*2u,2,&av2);
if(rc){ROOT_FAIL(8,rc);}
v2=get(av2.data);
rc=h->buffer_release(h->context,av2.lease);if(rc){ROOT_FAIL(8,rc);}}
put(state+118,v2);
break;}
case 1:{
uint32_t v3=get(state+112);
int32_t index4=(int32_t)(v3);
if(index4<65530||index4>65533)FAIL(5);
uint32_t v5;{vdplet_view_v1 av5={sizeof(av5),0,0,0,0};
rc=h->buffer_acquire(h->context,257,VDPL_READ,(uint32_t)(index4-(65530))*2u,2,&av5);
if(rc){ROOT_FAIL(8,rc);}
v5=get(av5.data);
rc=h->buffer_release(h->context,av5.lease);if(rc){ROOT_FAIL(8,rc);}}
put(state+120,v5);
break;}
case 2:{
uint32_t v6=get(state+122);
uint32_t v7=(uint32_t)(v6)+(uint32_t)(1u);
if(v7>65535u)FAIL(1);
put(state+122,v7);
uint32_t v8=get(state+112);
int32_t index9=(int32_t)(v8);
if(index9<0||index9>5)FAIL(5);
uint32_t v11=get(state+116);
{vdplet_view_v1 store10={sizeof(store10),0,0,0,0};
rc=h->buffer_acquire(h->context,258,VDPL_WRITE,(uint32_t)(index9-(0))*2u,2,&store10);
if(rc){ROOT_FAIL(8,rc);}
put(store10.data,v11);
rc=h->buffer_release(h->context,store10.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v12=get(state+112);
int32_t index13=(int32_t)(v12);
if(index13<0||index13>5)FAIL(5);
uint32_t v14;{vdplet_view_v1 av14={sizeof(av14),0,0,0,0};
rc=h->buffer_acquire(h->context,258,VDPL_READ,(uint32_t)(index13-(0))*2u,2,&av14);
if(rc){ROOT_FAIL(8,rc);}
v14=get(av14.data);
rc=h->buffer_release(h->context,av14.lease);if(rc){ROOT_FAIL(8,rc);}}
put(state+120,v14);
break;}
case 3:{
uint32_t v15=get(state+112);
int32_t index16=(int32_t)(v15);
if(index16<0||index16>5)FAIL(5);
uint32_t v18=get(state+112);
int32_t index19=(int32_t)(v18);
if(index19<0||index19>5)FAIL(5);
uint32_t v20;{vdplet_view_v1 av20={sizeof(av20),0,0,0,0};
rc=h->buffer_acquire(h->context,258,VDPL_READ,(uint32_t)(index19-(0))*2u,2,&av20);
if(rc){ROOT_FAIL(8,rc);}
v20=get(av20.data);
rc=h->buffer_release(h->context,av20.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v21=get(state+116);
uint32_t v22=(uint32_t)(v20)+(uint32_t)(v21);
if(v22>65535u)FAIL(1);
{vdplet_view_v1 store17={sizeof(store17),0,0,0,0};
rc=h->buffer_acquire(h->context,258,VDPL_WRITE,(uint32_t)(index16-(0))*2u,2,&store17);
if(rc){ROOT_FAIL(8,rc);}
put(store17.data,v22);
rc=h->buffer_release(h->context,store17.lease);if(rc){ROOT_FAIL(8,rc);}}
break;}
case 4:{
uint32_t v23=get(state+114);
int32_t index24=signed16(v23);
if(index24<-2||index24>1)FAIL(5);
uint32_t v26=get(state+112);
int32_t index27=(int32_t)(v26);
if(index27<0||index27>256)FAIL(5);
uint32_t v28;{vdplet_view_v1 av28={sizeof(av28),0,0,0,0};
rc=h->buffer_acquire(h->context,259,VDPL_READ,(uint32_t)(index27-(0))*2u,2,&av28);
if(rc){ROOT_FAIL(8,rc);}
v28=get(av28.data);
rc=h->buffer_release(h->context,av28.lease);if(rc){ROOT_FAIL(8,rc);}}
{vdplet_view_v1 store25={sizeof(store25),0,0,0,0};
rc=h->buffer_acquire(h->context,256,VDPL_WRITE,(uint32_t)(index24-(-2))*2u,2,&store25);
if(rc){ROOT_FAIL(8,rc);}
put(store25.data,v28);
rc=h->buffer_release(h->context,store25.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v29=get(state+114);
int32_t index30=signed16(v29);
if(index30<-2||index30>1)FAIL(5);
uint32_t v31;{vdplet_view_v1 av31={sizeof(av31),0,0,0,0};
rc=h->buffer_acquire(h->context,256,VDPL_READ,(uint32_t)(index30-(-2))*2u,2,&av31);
if(rc){ROOT_FAIL(8,rc);}
v31=get(av31.data);
rc=h->buffer_release(h->context,av31.lease);if(rc){ROOT_FAIL(8,rc);}}
put(state+118,v31);
break;}
case 5:{
uint32_t v32=get(state+112);
int32_t index33=(int32_t)(v32);
if(index33<0||index33>256)FAIL(5);
uint32_t v34;{vdplet_view_v1 av34={sizeof(av34),0,0,0,0};
rc=h->buffer_acquire(h->context,259,VDPL_READ,(uint32_t)(index33-(0))*2u,2,&av34);
if(rc){ROOT_FAIL(8,rc);}
v34=get(av34.data);
rc=h->buffer_release(h->context,av34.lease);if(rc){ROOT_FAIL(8,rc);}}
put(state+118,v34);
break;}
case 6:{
uint32_t v35=0u;
if(v35){
uint32_t v36=get(state+112);
int32_t index37=(int32_t)(v36);
if(index37<0||index37>256)FAIL(5);
uint32_t v38;{vdplet_view_v1 av38={sizeof(av38),0,0,0,0};
rc=h->buffer_acquire(h->context,259,VDPL_READ,(uint32_t)(index37-(0))*2u,2,&av38);
if(rc){ROOT_FAIL(8,rc);}
v38=get(av38.data);
rc=h->buffer_release(h->context,av38.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v39=(signed16(v38)==signed16(0u))?1u:0u;
v35=v39;
}
if(v35>1u)FAIL(32);
put(state+128,v35);
uint32_t v40=get(state+122);
uint32_t v41=(uint32_t)(v40)+(uint32_t)(1u);
if(v41>65535u)FAIL(1);
put(state+122,v41);
break;}
case 7:{
uint32_t v42=get(state+112);
int32_t index43=(int32_t)(v42);
if(index43<0||index43>5)FAIL(5);
uint32_t v45=get(state+116);
uint32_t v46=(uint32_t)(v45)+(uint32_t)(1u);
if(v46>65535u)FAIL(1);
{vdplet_view_v1 store44={sizeof(store44),0,0,0,0};
rc=h->buffer_acquire(h->context,258,VDPL_WRITE,(uint32_t)(index43-(0))*2u,2,&store44);
if(rc){ROOT_FAIL(8,rc);}
put(store44.data,v46);
rc=h->buffer_release(h->context,store44.lease);if(rc){ROOT_FAIL(8,rc);}}
break;}
case 8:{
uint32_t v47=get(state+112);
uint32_t v48=get(state+116);
uint32_t v49=(uint32_t)(v47)+(uint32_t)(v48);
if(v49>65535u)FAIL(1);
int32_t index50=(int32_t)(v49);
if(index50<0||index50>5)FAIL(5);
uint32_t v51;{vdplet_view_v1 av51={sizeof(av51),0,0,0,0};
rc=h->buffer_acquire(h->context,258,VDPL_READ,(uint32_t)(index50-(0))*2u,2,&av51);
if(rc){ROOT_FAIL(8,rc);}
v51=get(av51.data);
rc=h->buffer_release(h->context,av51.lease);if(rc){ROOT_FAIL(8,rc);}}
put(state+120,v51);
break;}
case 9:{
put(state+124,0u);
for(uint32_t loop52=0;loop52<6u;++loop52){
put(state+126,(uint16_t)(0+(int32_t)loop52*(1)));
uint32_t v53=get(state+124);
uint32_t v54=get(state+126);
int32_t index55=(int32_t)(v54);
if(index55<0||index55>5)FAIL(5);
uint32_t v56;{vdplet_view_v1 av56={sizeof(av56),0,0,0,0};
rc=h->buffer_acquire(h->context,258,VDPL_READ,(uint32_t)(index55-(0))*2u,2,&av56);
if(rc){ROOT_FAIL(8,rc);}
v56=get(av56.data);
rc=h->buffer_release(h->context,av56.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v57=(uint32_t)(v53)+(uint32_t)(v56);
if(v57>65535u)FAIL(1);
put(state+124,v57);
}
break;}
}
put32(state+20,get32(state+20)+1u);put(state+24,r->entry_id);put32(state+32,get32(state+8));
h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return 0;
}
