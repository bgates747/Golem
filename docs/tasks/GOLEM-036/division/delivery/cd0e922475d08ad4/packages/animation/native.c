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
if(r->operation!=VDPL_RUN||r->entry_id>=1)return VDPL_E_UNSUPPORTED;
if(r->argument_bytes!=2)return VDPL_E_ARGUMENT;
vdplet_view_v1 args={sizeof(args),0,0,0,0},view={sizeof(view),0,0,0,0};
int32_t rc=h->buffer_acquire(h->context,r->argument_buffer,VDPL_READ,0,2,&args);if(rc)return rc;
uint16_t id=get(args.data);rc=h->buffer_acquire(h->context,id,VDPL_WRITE,0,VDPL_WHOLE_BUFFER,&view);if(rc){h->buffer_release(h->context,args.lease);return rc;}
if(view.bytes!=142){h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return VDPL_E_BOUNDS;}
uint8_t*state=view.data;put(state+12,0);
static const uint8_t offsets[] __attribute__((aligned(4)))={0,1,2,3,4,5,6,7,26,27,30,31,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,};
static const uint8_t values[] __attribute__((aligned(4)))={82,71,1,0,112,0,142,0,4,0,0,0,1,0,30,0,0,0,0,0,48,53,50,82,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
static const uint8_t errors[] __attribute__((aligned(4)))={1,1,2,2,3,3,3,3,3,3,6,6,3,3,3,3,6,6,6,6,6,6,6,6,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,};
for(unsigned i=0;i<sizeof(offsets);++i)if(state[offsets[i]]!=values[i])ROOT_FAIL(errors[i],VDPL_E_ARGUMENT);
if(!get32(state+8))ROOT_FAIL(4,VDPL_E_ARGUMENT);
if(get(state+28)>=1u||get(state+28)!=r->entry_id)ROOT_FAIL(7,VDPL_E_ARGUMENT);
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
if(shape!=6u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,6,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=6u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,7,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=6u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,8,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=6u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,9,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=6u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
put(state+14,0);
switch(r->entry_id){
case 0:{
for(uint32_t loop0=0;loop0<3u;++loop0){
put(state+112,(uint16_t)(0+(int32_t)loop0*(1)));
uint32_t v1=get(state+112);
int32_t index2=(int32_t)(v1);
if(index2<0||index2>2)FAIL(5);
uint32_t v3;{vdplet_view_v1 av3={sizeof(av3),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_READ,(uint32_t)(index2-(0))*4u,4,&av3);
if(rc){ROOT_FAIL(8,rc);}
v3=get32(av3.data);
rc=h->buffer_release(h->context,av3.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v4=(v3>=127u)?1u:0u;
if(v4){
uint32_t v5=get(state+112);
int32_t index6=(int32_t)(v5);
if(index6<0||index6>2)FAIL(5);
{vdplet_view_v1 store7={sizeof(store7),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index6-(0))*2u,2,&store7);
if(rc){ROOT_FAIL(8,rc);}
put(store7.data,0u);
rc=h->buffer_release(h->context,store7.lease);if(rc){ROOT_FAIL(8,rc);}}
}else{
uint32_t v8=get(state+112);
int32_t index9=(int32_t)(v8);
if(index9<0||index9>2)FAIL(5);
uint32_t v10;{vdplet_view_v1 av10={sizeof(av10),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_READ,(uint32_t)(index9-(0))*4u,4,&av10);
if(rc){ROOT_FAIL(8,rc);}
v10=get32(av10.data);
rc=h->buffer_release(h->context,av10.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v11=(v10<=0u)?1u:0u;
if(v11){
uint32_t v12=get(state+112);
int32_t index13=(int32_t)(v12);
if(index13<0||index13>2)FAIL(5);
{vdplet_view_v1 store14={sizeof(store14),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index13-(0))*2u,2,&store14);
if(rc){ROOT_FAIL(8,rc);}
put(store14.data,1u);
rc=h->buffer_release(h->context,store14.lease);if(rc){ROOT_FAIL(8,rc);}}
}else{
}
}
uint32_t v15=get(state+112);
int32_t index16=(int32_t)(v15);
if(index16<0||index16>2)FAIL(5);
uint32_t v17;{vdplet_view_v1 av17={sizeof(av17),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_READ,(uint32_t)(index16-(0))*2u,2,&av17);
if(rc){ROOT_FAIL(8,rc);}
v17=get(av17.data);
rc=h->buffer_release(h->context,av17.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v18=(v17==1u)?1u:0u;
if(v18){
uint32_t v19=get(state+112);
int32_t index20=(int32_t)(v19);
if(index20<0||index20>2)FAIL(5);
uint32_t v22=get(state+112);
int32_t index23=(int32_t)(v22);
if(index23<0||index23>2)FAIL(5);
uint32_t v24;{vdplet_view_v1 av24={sizeof(av24),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_READ,(uint32_t)(index23-(0))*4u,4,&av24);
if(rc){ROOT_FAIL(8,rc);}
v24=get32(av24.data);
rc=h->buffer_release(h->context,av24.lease);if(rc){ROOT_FAIL(8,rc);}}
uint64_t nv25=(uint64_t)(v24)+(uint64_t)(1u);
if(nv25>4294967295ull)FAIL(1);
uint32_t v25=(uint32_t)nv25;
{vdplet_view_v1 store21={sizeof(store21),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_WRITE,(uint32_t)(index20-(0))*4u,4,&store21);
if(rc){ROOT_FAIL(8,rc);}
put32(store21.data,v25);
rc=h->buffer_release(h->context,store21.lease);if(rc){ROOT_FAIL(8,rc);}}
}else{
uint32_t v26=get(state+112);
int32_t index27=(int32_t)(v26);
if(index27<0||index27>2)FAIL(5);
uint32_t v29=get(state+112);
int32_t index30=(int32_t)(v29);
if(index30<0||index30>2)FAIL(5);
uint32_t v31;{vdplet_view_v1 av31={sizeof(av31),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_READ,(uint32_t)(index30-(0))*4u,4,&av31);
if(rc){ROOT_FAIL(8,rc);}
v31=get32(av31.data);
rc=h->buffer_release(h->context,av31.lease);if(rc){ROOT_FAIL(8,rc);}}
if(v31<1u)FAIL(2);
uint32_t v32=v31-1u;
{vdplet_view_v1 store28={sizeof(store28),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_WRITE,(uint32_t)(index27-(0))*4u,4,&store28);
if(rc){ROOT_FAIL(8,rc);}
put32(store28.data,v32);
rc=h->buffer_release(h->context,store28.lease);if(rc){ROOT_FAIL(8,rc);}}
}
uint32_t v33=get(state+112);
int32_t index34=(int32_t)(v33);
if(index34<0||index34>2)FAIL(5);
uint32_t v35;{vdplet_view_v1 av35={sizeof(av35),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_READ,(uint32_t)(index34-(0))*4u,4,&av35);
if(rc){ROOT_FAIL(8,rc);}
v35=get32(av35.data);
rc=h->buffer_release(h->context,av35.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v36=get(state+112);
int32_t index37=(int32_t)(v36);
if(index37<0||index37>2)FAIL(5);
uint32_t v38;{vdplet_view_v1 av38={sizeof(av38),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_READ,(uint32_t)(index37-(0))*4u,4,&av38);
if(rc){ROOT_FAIL(8,rc);}
v38=get32(av38.data);
rc=h->buffer_release(h->context,av38.lease);if(rc){ROOT_FAIL(8,rc);}}
uint64_t nv39=(uint64_t)(v35)*(uint64_t)(v38);
if(nv39>4294967295ull)FAIL(3);
uint32_t v39=(uint32_t)nv39;
put32(state+114,v39);
uint32_t v40=get32(state+114);
uint32_t v41=64u;
if(!v41)FAIL(4);
uint32_t v42=v40/v41,v43=v40%v41;
put32(state+118,v42);
put32(state+122,v43);
uint32_t v44=get(state+112);
int32_t index45=(int32_t)(v44);
if(index45<0||index45>2)FAIL(5);
uint32_t v47=get32(state+122);
{vdplet_view_v1 store46={sizeof(store46),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_WRITE,(uint32_t)(index45-(0))*4u,4,&store46);
if(rc){ROOT_FAIL(8,rc);}
put32(store46.data,v47);
rc=h->buffer_release(h->context,store46.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v48=get(state+112);
int32_t index49=(int32_t)(v48);
if(index49<0||index49>2)FAIL(5);
uint32_t v50;{vdplet_view_v1 av50={sizeof(av50),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_READ,(uint32_t)(index49-(0))*4u,4,&av50);
if(rc){ROOT_FAIL(8,rc);}
v50=get32(av50.data);
rc=h->buffer_release(h->context,av50.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv51=(int64_t)(v50);
if(nv51<-2147483648ll||nv51>2147483647ll)FAIL(32);
uint32_t v51=(uint32_t)nv51;
int64_t nv52=(int64_t)signed32(v51)-(int64_t)signed32(64u);
if(nv52 < -2147483648ll || nv52 >2147483647ll)FAIL(2);
uint32_t v52=(uint32_t)nv52;
int64_t nv53=(int64_t)signed32(v52)*(int64_t)signed32(3u);
if(nv53 < -2147483648ll || nv53 >2147483647ll)FAIL(3);
uint32_t v53=(uint32_t)nv53;
put32(state+126,v53);
uint32_t v54=get32(state+126);
uint32_t v55=8u;
if(!v55)FAIL(4);
if(v54==2147483648u && v55==4294967295u)FAIL(6);
uint32_t v56=(uint32_t)(signed32(v54)/signed32(v55)),v57=(uint32_t)(signed32(v54)%signed32(v55));
put32(state+130,v56);
put32(state+134,v57);
uint32_t v58=get(state+112);
int32_t index59=(int32_t)(v58);
if(index59<0||index59>2)FAIL(5);
uint32_t v61=get32(state+134);
{vdplet_view_v1 store60={sizeof(store60),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index59-(0))*4u,4,&store60);
if(rc){ROOT_FAIL(8,rc);}
put32(store60.data,v61);
rc=h->buffer_release(h->context,store60.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v62=get(state+112);
int32_t index63=(int32_t)(v62);
if(index63<0||index63>2)FAIL(5);
uint32_t v65=get32(state+118);
int64_t nv66=(int64_t)(v65);
if(nv66<0ll||nv66>65535ll)FAIL(32);
uint32_t v66=(uint32_t)nv66&65535u;
uint32_t v67=(uint32_t)(16u)+(uint32_t)(v66);
if(v67>65535u)FAIL(1);
{vdplet_view_v1 store64={sizeof(store64),0,0,0,0};
rc=h->buffer_acquire(h->context,7,VDPL_WRITE,(uint32_t)(index63-(0))*2u,2,&store64);
if(rc){ROOT_FAIL(8,rc);}
put(store64.data,v67);
rc=h->buffer_release(h->context,store64.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v68=get(state+112);
int32_t index69=(int32_t)(v68);
if(index69<0||index69>2)FAIL(5);
uint32_t v71=get(state+112);
int32_t index72=(int32_t)(v71);
if(index72<0||index72>2)FAIL(5);
uint32_t v73;{vdplet_view_v1 av73={sizeof(av73),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_READ,(uint32_t)(index72-(0))*2u,2,&av73);
if(rc){ROOT_FAIL(8,rc);}
v73=get(av73.data);
rc=h->buffer_release(h->context,av73.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv74=(int64_t)(v73);
if(nv74<-2147483648ll||nv74>2147483647ll)FAIL(32);
uint32_t v74=(uint32_t)nv74;
uint32_t v75=get32(state+130);
int64_t nv76=(int64_t)signed32(v74)+(int64_t)signed32(v75);
if(nv76 < -2147483648ll || nv76 >2147483647ll)FAIL(1);
uint32_t v76=(uint32_t)nv76;
int64_t nv77=(int64_t)(signed32(v76));
if(nv77<0ll||nv77>65535ll)FAIL(32);
uint32_t v77=(uint32_t)nv77&65535u;
{vdplet_view_v1 store70={sizeof(store70),0,0,0,0};
rc=h->buffer_acquire(h->context,8,VDPL_WRITE,(uint32_t)(index69-(0))*2u,2,&store70);
if(rc){ROOT_FAIL(8,rc);}
put(store70.data,v77);
rc=h->buffer_release(h->context,store70.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v78=get(state+112);
int32_t index79=(int32_t)(v78);
if(index79<0||index79>2)FAIL(5);
{vdplet_view_v1 store80={sizeof(store80),0,0,0,0};
rc=h->buffer_acquire(h->context,9,VDPL_WRITE,(uint32_t)(index79-(0))*2u,2,&store80);
if(rc){ROOT_FAIL(8,rc);}
put(store80.data,12u);
rc=h->buffer_release(h->context,store80.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v81=get32(state+130);
uint32_t v82=(signed32(v81)<signed32(0u))?1u:0u;
if(v82){
uint32_t v83=get(state+112);
int32_t index84=(int32_t)(v83);
if(index84<0||index84>2)FAIL(5);
{vdplet_view_v1 store85={sizeof(store85),0,0,0,0};
rc=h->buffer_acquire(h->context,9,VDPL_WRITE,(uint32_t)(index84-(0))*2u,2,&store85);
if(rc){ROOT_FAIL(8,rc);}
put(store85.data,48u);
rc=h->buffer_release(h->context,store85.lease);if(rc){ROOT_FAIL(8,rc);}}
}else{
}
}
uint32_t v86=get32(state+138);
uint64_t nv87=(uint64_t)(v86)+(uint64_t)(1u);
if(nv87>4294967295ull)FAIL(1);
uint32_t v87=(uint32_t)nv87;
put32(state+138,v87);
break;}
}
put32(state+20,get32(state+20)+1u);put(state+24,r->entry_id);put32(state+32,get32(state+8));
h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return 0;
}
