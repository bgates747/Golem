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
if(view.bytes!=126){h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return VDPL_E_BOUNDS;}
uint8_t*state=view.data;put(state+12,0);
static const uint8_t offsets[] __attribute__((aligned(4)))={0,1,2,3,4,5,6,7,26,27,30,31,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,};
static const uint8_t values[] __attribute__((aligned(4)))={82,71,1,0,112,0,126,0,4,0,0,0,1,0,14,0,0,0,0,0,48,53,50,82,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
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
put(state+14,0);
if((int64_t)(get32(state+114))<3200000ll)FAIL(32);
if((int64_t)(get32(state+114))>3200000ll)FAIL(32);
switch(r->entry_id){
case 0:{
for(uint32_t loop0=0;loop0<3u;++loop0){
put(state+112,(uint16_t)(0+(int32_t)loop0*(1)));
uint32_t v1=get(state+112);
int64_t nv2=(int64_t)(v1);
if(nv2<0ll||nv2>4294967295ll)FAIL(32);
uint32_t v2=(uint32_t)nv2;
uint64_t nv3=(uint64_t)(v2)+(uint64_t)(2u);
if(nv3>4294967295ull)FAIL(1);
uint32_t v3=(uint32_t)nv3;
put32(state+118,v3);
uint32_t v4=get(state+112);
int32_t index5=(int32_t)(v4);
if(index5<0||index5>2)FAIL(5);
uint32_t v6;{vdplet_view_v1 av6={sizeof(av6),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_READ,(uint32_t)(index5-(0))*4u,4,&av6);
if(rc){ROOT_FAIL(8,rc);}
v6=get32(av6.data);
rc=h->buffer_release(h->context,av6.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v7=(v6>=3200288u)?1u:0u;
if(v7){
uint32_t v8=get(state+112);
int32_t index9=(int32_t)(v8);
if(index9<0||index9>2)FAIL(5);
{vdplet_view_v1 store10={sizeof(store10),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_WRITE,(uint32_t)(index9-(0))*2u,2,&store10);
if(rc){ROOT_FAIL(8,rc);}
put(store10.data,0u);
rc=h->buffer_release(h->context,store10.lease);if(rc){ROOT_FAIL(8,rc);}}
}else{
uint32_t v11=get(state+112);
int32_t index12=(int32_t)(v11);
if(index12<0||index12>2)FAIL(5);
uint32_t v13;{vdplet_view_v1 av13={sizeof(av13),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_READ,(uint32_t)(index12-(0))*4u,4,&av13);
if(rc){ROOT_FAIL(8,rc);}
v13=get32(av13.data);
rc=h->buffer_release(h->context,av13.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v14=(v13<=3200016u)?1u:0u;
if(v14){
uint32_t v15=get(state+112);
int32_t index16=(int32_t)(v15);
if(index16<0||index16>2)FAIL(5);
{vdplet_view_v1 store17={sizeof(store17),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_WRITE,(uint32_t)(index16-(0))*2u,2,&store17);
if(rc){ROOT_FAIL(8,rc);}
put(store17.data,1u);
rc=h->buffer_release(h->context,store17.lease);if(rc){ROOT_FAIL(8,rc);}}
}else{
}
}
uint32_t v18=get(state+112);
int32_t index19=(int32_t)(v18);
if(index19<0||index19>2)FAIL(5);
uint32_t v20;{vdplet_view_v1 av20={sizeof(av20),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_READ,(uint32_t)(index19-(0))*2u,2,&av20);
if(rc){ROOT_FAIL(8,rc);}
v20=get(av20.data);
rc=h->buffer_release(h->context,av20.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v21=(v20==1u)?1u:0u;
if(v21){
uint32_t v22=get(state+112);
int32_t index23=(int32_t)(v22);
if(index23<0||index23>2)FAIL(5);
uint32_t v25=get(state+112);
int32_t index26=(int32_t)(v25);
if(index26<0||index26>2)FAIL(5);
uint32_t v27;{vdplet_view_v1 av27={sizeof(av27),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_READ,(uint32_t)(index26-(0))*4u,4,&av27);
if(rc){ROOT_FAIL(8,rc);}
v27=get32(av27.data);
rc=h->buffer_release(h->context,av27.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v28=get32(state+118);
uint64_t nv29=(uint64_t)(v27)+(uint64_t)(v28);
if(nv29>4294967295ull)FAIL(1);
uint32_t v29=(uint32_t)nv29;
{vdplet_view_v1 store24={sizeof(store24),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_WRITE,(uint32_t)(index23-(0))*4u,4,&store24);
if(rc){ROOT_FAIL(8,rc);}
put32(store24.data,v29);
rc=h->buffer_release(h->context,store24.lease);if(rc){ROOT_FAIL(8,rc);}}
}else{
uint32_t v30=get(state+112);
int32_t index31=(int32_t)(v30);
if(index31<0||index31>2)FAIL(5);
uint32_t v33=get(state+112);
int32_t index34=(int32_t)(v33);
if(index34<0||index34>2)FAIL(5);
uint32_t v35;{vdplet_view_v1 av35={sizeof(av35),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_READ,(uint32_t)(index34-(0))*4u,4,&av35);
if(rc){ROOT_FAIL(8,rc);}
v35=get32(av35.data);
rc=h->buffer_release(h->context,av35.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v36=get32(state+118);
if(v35<v36)FAIL(2);
uint32_t v37=v35-v36;
{vdplet_view_v1 store32={sizeof(store32),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_WRITE,(uint32_t)(index31-(0))*4u,4,&store32);
if(rc){ROOT_FAIL(8,rc);}
put32(store32.data,v37);
rc=h->buffer_release(h->context,store32.lease);if(rc){ROOT_FAIL(8,rc);}}
}
uint32_t v38=get(state+112);
int32_t index39=(int32_t)(v38);
if(index39<0||index39>2)FAIL(5);
uint32_t v41=get(state+112);
int32_t index42=(int32_t)(v41);
if(index42<0||index42>2)FAIL(5);
uint32_t v43;{vdplet_view_v1 av43={sizeof(av43),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_READ,(uint32_t)(index42-(0))*4u,4,&av43);
if(rc){ROOT_FAIL(8,rc);}
v43=get32(av43.data);
rc=h->buffer_release(h->context,av43.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v44=get32(state+114);
if(v43<v44)FAIL(2);
uint32_t v45=v43-v44;
int64_t nv46=(int64_t)(v45);
if(nv46<0ll||nv46>65535ll)FAIL(32);
uint32_t v46=(uint32_t)nv46&65535u;
{vdplet_view_v1 store40={sizeof(store40),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index39-(0))*2u,2,&store40);
if(rc){ROOT_FAIL(8,rc);}
put(store40.data,v46);
rc=h->buffer_release(h->context,store40.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v47=get(state+112);
int32_t index48=(int32_t)(v47);
if(index48<0||index48>2)FAIL(5);
uint32_t v49;{vdplet_view_v1 av49={sizeof(av49),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_READ,(uint32_t)(index48-(0))*4u,4,&av49);
if(rc){ROOT_FAIL(8,rc);}
v49=get32(av49.data);
rc=h->buffer_release(h->context,av49.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v50=(signed32(v49)<=signed32(4294928896u))?1u:0u;
if(v50){
uint32_t v51=get(state+112);
int32_t index52=(int32_t)(v51);
if(index52<0||index52>2)FAIL(5);
{vdplet_view_v1 store53={sizeof(store53),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index52-(0))*4u,4,&store53);
if(rc){ROOT_FAIL(8,rc);}
put32(store53.data,256u);
rc=h->buffer_release(h->context,store53.lease);if(rc){ROOT_FAIL(8,rc);}}
}else{
uint32_t v54=get(state+112);
int32_t index55=(int32_t)(v54);
if(index55<0||index55>2)FAIL(5);
uint32_t v56;{vdplet_view_v1 av56={sizeof(av56),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_READ,(uint32_t)(index55-(0))*4u,4,&av56);
if(rc){ROOT_FAIL(8,rc);}
v56=get32(av56.data);
rc=h->buffer_release(h->context,av56.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v57=(signed32(v56)>=signed32(38400u))?1u:0u;
if(v57){
uint32_t v58=get(state+112);
int32_t index59=(int32_t)(v58);
if(index59<0||index59>2)FAIL(5);
{vdplet_view_v1 store60={sizeof(store60),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index59-(0))*4u,4,&store60);
if(rc){ROOT_FAIL(8,rc);}
put32(store60.data,4294967040u);
rc=h->buffer_release(h->context,store60.lease);if(rc){ROOT_FAIL(8,rc);}}
}else{
}
}
uint32_t v61=get(state+112);
int32_t index62=(int32_t)(v61);
if(index62<0||index62>2)FAIL(5);
uint32_t v64=get(state+112);
int32_t index65=(int32_t)(v64);
if(index65<0||index65>2)FAIL(5);
uint32_t v66;{vdplet_view_v1 av66={sizeof(av66),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_READ,(uint32_t)(index65-(0))*4u,4,&av66);
if(rc){ROOT_FAIL(8,rc);}
v66=get32(av66.data);
rc=h->buffer_release(h->context,av66.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v67=get(state+112);
int32_t index68=(int32_t)(v67);
if(index68<0||index68>2)FAIL(5);
uint32_t v69;{vdplet_view_v1 av69={sizeof(av69),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_READ,(uint32_t)(index68-(0))*4u,4,&av69);
if(rc){ROOT_FAIL(8,rc);}
v69=get32(av69.data);
rc=h->buffer_release(h->context,av69.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv70=(int64_t)signed32(v66)+(int64_t)signed32(v69);
if(nv70 < -2147483648ll || nv70 >2147483647ll)FAIL(1);
uint32_t v70=(uint32_t)nv70;
{vdplet_view_v1 store63={sizeof(store63),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_WRITE,(uint32_t)(index62-(0))*4u,4,&store63);
if(rc){ROOT_FAIL(8,rc);}
put32(store63.data,v70);
rc=h->buffer_release(h->context,store63.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v71=get(state+112);
int32_t index72=(int32_t)(v71);
if(index72<0||index72>2)FAIL(5);
{vdplet_view_v1 store73={sizeof(store73),0,0,0,0};
rc=h->buffer_acquire(h->context,7,VDPL_WRITE,(uint32_t)(index72-(0))*2u,2,&store73);
if(rc){ROOT_FAIL(8,rc);}
put(store73.data,12u);
rc=h->buffer_release(h->context,store73.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v74=get(state+112);
int32_t index75=(int32_t)(v74);
if(index75<0||index75>2)FAIL(5);
uint32_t v76;{vdplet_view_v1 av76={sizeof(av76),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_READ,(uint32_t)(index75-(0))*4u,4,&av76);
if(rc){ROOT_FAIL(8,rc);}
v76=get32(av76.data);
rc=h->buffer_release(h->context,av76.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v77=(signed32(v76)<signed32(0u))?1u:0u;
if(v77){
uint32_t v78=get(state+112);
int32_t index79=(int32_t)(v78);
if(index79<0||index79>2)FAIL(5);
{vdplet_view_v1 store80={sizeof(store80),0,0,0,0};
rc=h->buffer_acquire(h->context,7,VDPL_WRITE,(uint32_t)(index79-(0))*2u,2,&store80);
if(rc){ROOT_FAIL(8,rc);}
put(store80.data,48u);
rc=h->buffer_release(h->context,store80.lease);if(rc){ROOT_FAIL(8,rc);}}
}else{
}
}
uint32_t v81=get32(state+122);
uint64_t nv82=(uint64_t)(v81)+(uint64_t)(1u);
if(nv82>4294967295ull)FAIL(1);
uint32_t v82=(uint32_t)nv82;
put32(state+122,v82);
break;}
}
put32(state+20,get32(state+20)+1u);put(state+24,r->entry_id);put32(state+32,get32(state+8));
h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return 0;
}
