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
if(r->operation!=VDPL_RUN||r->entry_id>=11)return VDPL_E_UNSUPPORTED;
if(r->argument_bytes!=2)return VDPL_E_ARGUMENT;
vdplet_view_v1 args={sizeof(args),0,0,0,0},view={sizeof(view),0,0,0,0};
int32_t rc=h->buffer_acquire(h->context,r->argument_buffer,VDPL_READ,0,2,&args);if(rc)return rc;
uint16_t id=get(args.data);rc=h->buffer_acquire(h->context,id,VDPL_WRITE,0,VDPL_WHOLE_BUFFER,&view);if(rc){h->buffer_release(h->context,args.lease);return rc;}
if(view.bytes!=164){h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return VDPL_E_BOUNDS;}
uint8_t*state=view.data;put(state+12,0);
static const uint8_t offsets[] __attribute__((aligned(4)))={0,1,2,3,4,5,6,7,26,27,30,31,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,};
static const uint8_t values[] __attribute__((aligned(4)))={82,71,1,0,112,0,164,0,4,0,0,0,11,0,52,0,0,0,0,0,48,53,50,82,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
static const uint8_t errors[] __attribute__((aligned(4)))={1,1,2,2,3,3,3,3,3,3,6,6,3,3,3,3,6,6,6,6,6,6,6,6,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,};
for(unsigned i=0;i<sizeof(offsets);++i)if(state[offsets[i]]!=values[i])ROOT_FAIL(errors[i],VDPL_E_ARGUMENT);
if(!get32(state+8))ROOT_FAIL(4,VDPL_E_ARGUMENT);
if(get(state+28)>=11u||get(state+28)!=r->entry_id)ROOT_FAIL(7,VDPL_E_ARGUMENT);
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,2,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=8u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,3,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=8u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
put(state+14,0);
if(get(state+112)>1u)FAIL(32);
if((int64_t)(signed32(get32(state+150)))<-10ll)FAIL(32);
if((int64_t)(signed32(get32(state+150)))>10ll)FAIL(32);
if((int64_t)(signed32(get32(state+154)))<-3ll)FAIL(32);
if((int64_t)(signed32(get32(state+154)))>3ll)FAIL(32);
switch(r->entry_id){
case 0:{
uint32_t v0=get32(state+114);
uint32_t v1=get32(state+118);
uint32_t v2=v1;
if(!v2)FAIL(4);
uint32_t v3=v0/v2,v4=v0%v2;
put32(state+126,v3);
put32(state+130,v4);
break;}
case 1:{
uint32_t v5=get32(state+134);
uint32_t v6=get32(state+138);
uint32_t v7=v6;
if(!v7)FAIL(4);
if(v5==2147483648u && v7==4294967295u)FAIL(6);
uint32_t v8=(uint32_t)(signed32(v5)/signed32(v7)),v9=(uint32_t)(signed32(v5)%signed32(v7));
put32(state+142,v8);
put32(state+146,v9);
break;}
case 2:{
uint32_t v10=get32(state+114);
uint32_t v11=get32(state+118);
uint32_t v12=v11;
if(!v12)FAIL(4);
uint32_t v13=v10/v12,v14=v10%v12;
put32(state+114,v13);
put32(state+118,v14);
break;}
case 3:{
uint32_t v15=get32(state+134);
uint32_t v16=get32(state+138);
uint32_t v17=v16;
if(!v17)FAIL(4);
if(v15==2147483648u && v17==4294967295u)FAIL(6);
uint32_t v18=(uint32_t)(signed32(v15)/signed32(v17)),v19=(uint32_t)(signed32(v15)%signed32(v17));
put32(state+134,v18);
put32(state+138,v19);
break;}
case 4:{
uint32_t v20=get32(state+134);
uint32_t v21=get32(state+138);
uint32_t v22=v21;
if(!v22)FAIL(4);
if(v20==2147483648u && v22==4294967295u)FAIL(6);
uint32_t v23=(uint32_t)(signed32(v20)/signed32(v22)),v24=(uint32_t)(signed32(v20)%signed32(v22));
if((int64_t)(signed32(v23))<-10ll)FAIL(32);
if((int64_t)(signed32(v23))>10ll)FAIL(32);
if((int64_t)(signed32(v24))<-3ll)FAIL(32);
if((int64_t)(signed32(v24))>3ll)FAIL(32);
put32(state+150,v23);
put32(state+154,v24);
break;}
case 5:{
uint32_t v25=get(state+160);
uint32_t v26=(uint32_t)(v25)+(uint32_t)(1u);
if(v26>65535u)FAIL(1);
put(state+160,v26);
uint32_t v27=get32(state+134);
uint32_t v28=get32(state+138);
uint32_t v29=v28;
if(!v29)FAIL(4);
if(v27==2147483648u && v29==4294967295u)FAIL(6);
uint32_t v30=(uint32_t)(signed32(v27)/signed32(v29)),v31=(uint32_t)(signed32(v27)%signed32(v29));
put32(state+142,v30);
put32(state+146,v31);
break;}
case 6:{
uint32_t v32=get32(state+114);
uint32_t v33=get32(state+118);
uint64_t nv34=(uint64_t)(v32)*(uint64_t)(v33);
if(nv34>4294967295ull)FAIL(3);
uint32_t v34=(uint32_t)nv34;
uint32_t v35=get32(state+122);
uint64_t nv36=(uint64_t)(v35)+(uint64_t)(1u);
if(nv36>4294967295ull)FAIL(1);
uint32_t v36=(uint32_t)nv36;
uint32_t v37=v36;
if(!v37)FAIL(4);
uint32_t v38=v34/v37,v39=v34%v37;
put32(state+126,v38);
put32(state+130,v39);
break;}
case 7:{
uint32_t v40=get(state+158);
int32_t index41=(int32_t)(v40);
if(index41<0||index41>1)FAIL(5);
uint32_t v42;{vdplet_view_v1 av42={sizeof(av42),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_READ,(uint32_t)(index41-(0))*4u,4,&av42);
if(rc){ROOT_FAIL(8,rc);}
v42=get32(av42.data);
rc=h->buffer_release(h->context,av42.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v43=get32(state+118);
uint32_t v44=v43;
if(!v44)FAIL(4);
uint32_t v45=v42/v44,v46=v42%v44;
put32(state+126,v45);
put32(state+130,v46);
break;}
case 8:{
uint32_t v47=get(state+162);
int32_t index48=signed16(v47);
if(index48<-1||index48>0)FAIL(5);
uint32_t v49;{vdplet_view_v1 av49={sizeof(av49),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_READ,(uint32_t)(index48-(-1))*4u,4,&av49);
if(rc){ROOT_FAIL(8,rc);}
v49=get32(av49.data);
rc=h->buffer_release(h->context,av49.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v50=get32(state+138);
uint32_t v51=v50;
if(!v51)FAIL(4);
if(v49==2147483648u && v51==4294967295u)FAIL(6);
uint32_t v52=(uint32_t)(signed32(v49)/signed32(v51)),v53=(uint32_t)(signed32(v49)%signed32(v51));
put32(state+142,v52);
put32(state+146,v53);
break;}
case 9:{
if(0u){
uint32_t v54=get32(state+114);
uint32_t v55=get32(state+118);
uint32_t v56=v55;
if(!v56)FAIL(4);
uint32_t v57=v54/v56,v58=v54%v56;
put32(state+126,v57);
put32(state+130,v58);
}else{
}
break;}
case 10:{
uint32_t v59=get32(state+114);
uint32_t v60=get32(state+118);
uint32_t v61=v60;
if(!v61)FAIL(4);
uint32_t v62=v59/v61,v63=v59%v61;
put32(state+126,v62);
put32(state+130,v63);
uint32_t v64=get32(state+126);
uint32_t v65=get32(state+122);
uint32_t v66=v65;
if(!v66)FAIL(4);
uint32_t v67=v64/v66,v68=v64%v66;
put32(state+114,v67);
put32(state+118,v68);
break;}
}
put32(state+20,get32(state+20)+1u);put(state+24,r->entry_id);put32(state+32,get32(state+8));
h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return 0;
}
