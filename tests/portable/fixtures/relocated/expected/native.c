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
if(r->operation!=VDPL_RUN||r->entry_id>=17)return VDPL_E_UNSUPPORTED;
if(r->argument_bytes!=2)return VDPL_E_ARGUMENT;
vdplet_view_v1 args={sizeof(args),0,0,0,0},view={sizeof(view),0,0,0,0};
int32_t rc=h->buffer_acquire(h->context,r->argument_buffer,VDPL_READ,0,2,&args);if(rc)return rc;
uint16_t id=get(args.data);rc=h->buffer_acquire(h->context,id,VDPL_WRITE,0,VDPL_WHOLE_BUFFER,&view);if(rc){h->buffer_release(h->context,args.lease);return rc;}
if(view.bytes!=152){h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return VDPL_E_BOUNDS;}
uint8_t*state=view.data;put(state+12,0);
static const uint8_t offsets[] __attribute__((aligned(4)))={0,1,2,3,4,5,6,7,26,27,30,31,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,};
static const uint8_t values[] __attribute__((aligned(4)))={82,71,1,0,112,0,152,0,4,0,0,0,17,0,40,0,0,0,0,0,48,53,50,82,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
static const uint8_t errors[] __attribute__((aligned(4)))={1,1,2,2,3,3,3,3,3,3,6,6,3,3,3,3,6,6,6,6,6,6,6,6,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,};
for(unsigned i=0;i<sizeof(offsets);++i)if(state[offsets[i]]!=values[i])ROOT_FAIL(errors[i],VDPL_E_ARGUMENT);
if(!get32(state+8))ROOT_FAIL(4,VDPL_E_ARGUMENT);
if(get(state+28)>=17u||get(state+28)!=r->entry_id)ROOT_FAIL(7,VDPL_E_ARGUMENT);
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
if(shape!=4u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
put(state+14,0);
if(get(state+112)>1u)FAIL(32);
if((int64_t)(signed32(get32(state+146)))<-2ll)FAIL(32);
if((int64_t)(signed32(get32(state+146)))>2ll)FAIL(32);
switch(r->entry_id){
case 0:{
uint32_t v0=get32(state+114);
uint32_t v1=get(state+138);
if(v1>31u)FAIL(32);
uint32_t v2=v0>>v1;
put32(state+122,v2);
break;}
case 1:{
uint32_t v3=get32(state+126);
uint32_t v4=get(state+138);
if(v4>31u)FAIL(32);
uint32_t v5=v3>>v4;
if((v3&2147483648u)&&v4)v5|=4294967295u<<(32u-v4);
put32(state+134,v5);
break;}
case 2:{
uint32_t v6=get32(state+126);
uint32_t v7=get(state+138);
if(v7>31u)FAIL(32);
uint32_t v8=v6>>v7;
if((v6&2147483648u)&&v7)v8|=4294967295u<<(32u-v7);
uint32_t v9=get(state+140);
if(v9>31u)FAIL(32);
uint32_t v10=v8>>v9;
if((v8&2147483648u)&&v9)v10|=4294967295u<<(32u-v9);
put32(state+134,v10);
break;}
case 3:{
uint32_t v11=get32(state+114);
uint32_t v12=get32(state+118);
uint64_t nv13=(uint64_t)(v11)+(uint64_t)(v12);
if(nv13>4294967295ull)FAIL(1);
uint32_t v13=(uint32_t)nv13;
uint32_t v14=get(state+138);
uint32_t v15=(uint32_t)(v14)+(uint32_t)(1u);
if(v15>65535u)FAIL(1);
if(v15>31u)FAIL(32);
uint32_t v16=v13>>v15;
put32(state+122,v16);
break;}
case 4:{
uint32_t v17=get32(state+114);
uint32_t v18=get32(state+118);
uint32_t v19=get(state+138);
if(v19>31u)FAIL(32);
uint32_t v20=v18>>v19;
uint64_t nv21=(uint64_t)(v17)+(uint64_t)(v20);
if(nv21>4294967295ull)FAIL(1);
uint32_t v21=(uint32_t)nv21;
put32(state+122,v21);
break;}
case 5:{
uint32_t v22=get32(state+114);
uint32_t v23=get(state+138);
if(v23>31u)FAIL(32);
uint32_t v24=v22>>v23;
uint32_t v25=get32(state+118);
uint32_t v26=get(state+140);
if(v26>31u)FAIL(32);
uint32_t v27=v25>>v26;
uint64_t nv28=(uint64_t)(v24)+(uint64_t)(v27);
if(nv28>4294967295ull)FAIL(1);
uint32_t v28=(uint32_t)nv28;
put32(state+122,v28);
break;}
case 6:{
uint32_t v29=get32(state+114);
uint32_t v30=get32(state+118);
uint32_t v31=get(state+140);
if(v31>31u)FAIL(32);
uint32_t v32=v30>>v31;
int64_t nv33=(int64_t)(v32);
if(nv33<0ll||nv33>65535ll)FAIL(32);
uint32_t v33=(uint32_t)nv33&65535u;
if(v33>31u)FAIL(32);
uint32_t v34=v29>>v33;
put32(state+122,v34);
break;}
case 7:{
uint32_t v35=get32(state+126);
uint32_t v36=get(state+138);
if(v36>31u)FAIL(32);
uint32_t v37=v35>>v36;
if((v35&2147483648u)&&v36)v37|=4294967295u<<(32u-v36);
if((int64_t)(signed32(v37))<-2ll)FAIL(32);
if((int64_t)(signed32(v37))>2ll)FAIL(32);
put32(state+146,v37);
break;}
case 8:{
uint32_t v38=get(state+144);
uint32_t v39=(uint32_t)(v38)+(uint32_t)(1u);
if(v39>65535u)FAIL(1);
put(state+144,v39);
uint32_t v40=get32(state+114);
uint32_t v41=get(state+138);
if(v41>31u)FAIL(32);
uint32_t v42=v40>>v41;
put32(state+122,v42);
break;}
case 9:{
uint32_t v43=get32(state+114);
uint32_t v44=get32(state+118);
uint64_t nv45=(uint64_t)(v43)*(uint64_t)(v44);
if(nv45>4294967295ull)FAIL(3);
uint32_t v45=(uint32_t)nv45;
uint32_t v46=get(state+138);
uint32_t v47=(uint32_t)(v46)*(uint32_t)(2u);
if(v47>65535u)FAIL(3);
if(v47>31u)FAIL(32);
uint32_t v48=v45>>v47;
put32(state+122,v48);
break;}
case 10:{
uint32_t v49=get(state+142);
int32_t index50=(int32_t)(v49);
if(index50<0||index50>1)FAIL(5);
uint32_t v51;{vdplet_view_v1 av51={sizeof(av51),0,0,0,0};
rc=h->buffer_acquire(h->context,256,VDPL_READ,(uint32_t)(index50-(0))*4u,4,&av51);
if(rc){ROOT_FAIL(8,rc);}
v51=get32(av51.data);
rc=h->buffer_release(h->context,av51.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v52=get(state+138);
if(v52>31u)FAIL(32);
uint32_t v53=v51>>v52;
put32(state+122,v53);
break;}
case 11:{
uint32_t v54=get(state+150);
int32_t index55=signed16(v54);
if(index55<-1||index55>0)FAIL(5);
uint32_t v56;{vdplet_view_v1 av56={sizeof(av56),0,0,0,0};
rc=h->buffer_acquire(h->context,257,VDPL_READ,(uint32_t)(index55-(-1))*4u,4,&av56);
if(rc){ROOT_FAIL(8,rc);}
v56=get32(av56.data);
rc=h->buffer_release(h->context,av56.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v57=get(state+138);
if(v57>31u)FAIL(32);
uint32_t v58=v56>>v57;
if((v56&2147483648u)&&v57)v58|=4294967295u<<(32u-v57);
put32(state+134,v58);
break;}
case 12:{
uint32_t v59=get32(state+114);
uint32_t v60=get(state+142);
int32_t index61=(int32_t)(v60);
if(index61<0||index61>1)FAIL(5);
uint32_t v62;{vdplet_view_v1 av62={sizeof(av62),0,0,0,0};
rc=h->buffer_acquire(h->context,258,VDPL_READ,(uint32_t)(index61-(0))*2u,2,&av62);
if(rc){ROOT_FAIL(8,rc);}
v62=get(av62.data);
rc=h->buffer_release(h->context,av62.lease);if(rc){ROOT_FAIL(8,rc);}}
if(v62>31u)FAIL(32);
uint32_t v63=v59>>v62;
put32(state+122,v63);
break;}
case 13:{
uint32_t v64=get(state+142);
int32_t index65=(int32_t)(v64);
if(index65<0||index65>1)FAIL(5);
uint32_t v67=get32(state+114);
uint32_t v68=get(state+138);
if(v68>31u)FAIL(32);
uint32_t v69=v67>>v68;
{vdplet_view_v1 store66={sizeof(store66),0,0,0,0};
rc=h->buffer_acquire(h->context,256,VDPL_WRITE,(uint32_t)(index65-(0))*4u,4,&store66);
if(rc){ROOT_FAIL(8,rc);}
put32(store66.data,v69);
rc=h->buffer_release(h->context,store66.lease);if(rc){ROOT_FAIL(8,rc);}}
break;}
case 14:{
uint32_t v70=get(state+150);
int32_t index71=signed16(v70);
if(index71<-1||index71>0)FAIL(5);
uint32_t v73=get32(state+126);
uint32_t v74=get(state+138);
if(v74>31u)FAIL(32);
uint32_t v75=v73>>v74;
if((v73&2147483648u)&&v74)v75|=4294967295u<<(32u-v74);
{vdplet_view_v1 store72={sizeof(store72),0,0,0,0};
rc=h->buffer_acquire(h->context,257,VDPL_WRITE,(uint32_t)(index71-(-1))*4u,4,&store72);
if(rc){ROOT_FAIL(8,rc);}
put32(store72.data,v75);
rc=h->buffer_release(h->context,store72.lease);if(rc){ROOT_FAIL(8,rc);}}
break;}
case 15:{
uint32_t v76=0u;
if(v76){
uint32_t v77=get32(state+114);
uint32_t v78=get(state+138);
if(v78>31u)FAIL(32);
uint32_t v79=v77>>v78;
uint32_t v80=(v79==0u)?1u:0u;
v76=v80;
}
if(v76>1u)FAIL(32);
put(state+112,v76);
break;}
case 16:{
put32(state+122,1u);
put32(state+134,4294967294u);
break;}
}
put32(state+20,get32(state+20)+1u);put(state+24,r->entry_id);put32(state+32,get32(state+8));
h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return 0;
}
