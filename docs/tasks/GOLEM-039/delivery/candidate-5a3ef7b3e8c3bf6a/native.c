/* Generated from the shared Golem model; module state stays in VDP buffers. */
#include "vdplet.h"
static volatile uint32_t identity=0x474c3332u;
static uint16_t get(const uint8_t*p){return (uint16_t)p[0]|(uint16_t)p[1]<<8;}
static void put(uint8_t*p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static inline int32_t signed16(uint32_t v){return (int32_t)v-((v&32768u)?65536:0); }
static inline int32_t signed32(uint32_t v){return v>2147483647u?-1-(int32_t)(4294967295u-v):(int32_t)v;}
#define FAIL(n) do {put(state+14,n);h->buffer_release(h->context,view.lease);return VDPL_E_ARGUMENT;}while(0)
#define ROOT_FAIL(n,rc) do{put(state+12,n);h->buffer_release(h->context,view.lease);return (rc);}while(0)
static inline uint32_t get32(const uint8_t*p){return (uint32_t)get(p)|(uint32_t)get(p+2)<<16;}
static inline void put32(uint8_t*p,uint32_t v){put(p,(uint16_t)v);put(p+2,(uint16_t)(v>>16));}
static inline float fvalue(uint32_t u){union{uint32_t u;float f;}v={.u=u};return v.f;}
static inline uint32_t fbits(float f){union{uint32_t u;float f;}v={.f=f};return v.u;}
static inline uint32_t ordered(uint32_t v){return (v&0x80000000u)?~v:(v^0x80000000u);}
#define FP_RESULT(v) do{if(((v)&0x7f800000u)==0x7f800000u)FAIL(33);if(!((v)&0x7f800000u))(v)=0;}while(0)
#define FP_INPUT(v) do{if(!((v)&0x7f800000u)&&((v)&0x007fffffu))FAIL(33);FP_RESULT(v);}while(0)

#include "helper/rally_render.c"
void*memcpy(void*d,const void*s,size_t n){uint8_t*a=d;const uint8_t*b=s;for(size_t i=0;i<n;++i)a[i]=b[i];return d;}
void*memset(void*d,int v,size_t n){uint8_t*a=d;for(size_t i=0;i<n;++i)a[i]=(uint8_t)v;return d;}
static RallyRenderContextV1 renderer={.struct_bytes=sizeof(renderer)};
static RallyRenderBindingsV1 render_binding={76,1,0,{50300,1,0u,149518u},{{50301,2,0u,20480u},{50302,2,0u,2048u},{50303,2,0u,256u}},2101713653u,3635234752u,{0,5,120,63970,63984,63930}};
static int32_t renderer_hook(const vdplet_host_v1*h,unsigned entry,uint8_t*state){
unsigned op=0;switch(entry){
case 13:op=1;break;
case 15:op=2;break;
case 14:op=1;break;
case 16:op=3;break;
default:return 0;}
RallyRenderCompletionV1 done={.struct_bytes=48,.schema=1};
RallyRenderStateV1 s={.struct_bytes=108,.schema=1};
s.session=get32(state+292);
s.request_generation=get32(state+296);
s.simulation_tick=get32(state+136);
s.player_station=get32(state+198);
s.player_lateral_q8=signed32(get32(state+202));
s.phase=get(state+304);
s.steering=signed16(get(state+306));
s.flags=get(state+308);
if(!s.session||s.session!=get32(state+8)||get32(state+198)>2147483647u)return VDPL_E_ARGUMENT;
int32_t rc=0;
if(op==1){render_binding.session=s.session;rc=rally_render_bind_v1(h,&renderer,&render_binding,&done);}
else if(op==3)rc=rally_render_stop_v1(&renderer,&done);
else {int32_t heading=signed32(get32(state+120));if(heading< -1920||heading>1920)return VDPL_E_ARGUMENT;
{vdplet_view_v1 v={sizeof(v),0,0,0,0};rc=h->buffer_acquire(h->context,4,VDPL_READ,0,24,&v);if(rc)return rc;for(unsigned i=0;i<6;++i)s.opponents[i].station=get32(v.data+i*4);rc=h->buffer_release(h->context,v.lease);if(rc)return rc;}
{vdplet_view_v1 v={sizeof(v),0,0,0,0};rc=h->buffer_acquire(h->context,5,VDPL_READ,0,24,&v);if(rc)return rc;for(unsigned i=0;i<6;++i)s.opponents[i].lateral_q8=get32(v.data+i*4);rc=h->buffer_release(h->context,v.lease);if(rc)return rc;}
{vdplet_view_v1 v={sizeof(v),0,0,0,0};rc=h->buffer_acquire(h->context,2,VDPL_READ,0,12,&v);if(rc)return rc;for(unsigned i=0;i<6;++i)s.opponents[i].active=get(v.data+i*2);rc=h->buffer_release(h->context,v.lease);if(rc)return rc;}
for(unsigned i=0;i<6;++i){if(s.opponents[i].active)s.opponents[i].heading_turn16=(int16_t)heading;else s.opponents[i]=(RallyRenderOpponentV1){0,0,0,0};}rc=rally_render_prepare_v1(h,&renderer,&s,&done);}
if(done.reason==RALLY_RENDER_REASON_RELEASE)return rc;
vdplet_view_v1 out={sizeof(out),0,0,0,0};int32_t pr=h->buffer_acquire(h->context,50304,VDPL_WRITE,0,48,&out);if(pr)return pr;memcpy(out.data,&done,48);pr=h->buffer_release(h->context,out.lease);return pr?pr:rc;}
__attribute__((visibility("default")))
int32_t vdplet_entry(const vdplet_host_v1*h,const vdplet_request_v1*r){
if(!h||!r||h->struct_bytes<sizeof(*h)||r->struct_bytes<sizeof(*r)||h->abi_major!=1||!(h->capabilities&VDPL_CAP_BUFFERS)||!h->buffer_acquire||!h->buffer_release)return VDPL_E_ABI;
if(r->operation==VDPL_INIT)return identity==0x474c3332u?0:VDPL_E_INIT;
if(r->operation==VDPL_FINI)return 0;
if(r->operation!=VDPL_RUN||r->entry_id>=17)return VDPL_E_UNSUPPORTED;
if(r->argument_bytes!=2)return VDPL_E_ARGUMENT;
vdplet_view_v1 args={sizeof(args),0,0,0,0},view={sizeof(view),0,0,0,0};
int32_t rc=h->buffer_acquire(h->context,r->argument_buffer,VDPL_READ,0,2,&args);if(rc)return rc;
uint16_t id=get(args.data);rc=h->buffer_release(h->context,args.lease);if(rc)return rc;args.lease=0;rc=h->buffer_acquire(h->context,id,VDPL_WRITE,0,VDPL_WHOLE_BUFFER,&view);if(rc){return rc;}
if(view.bytes!=310){h->buffer_release(h->context,view.lease);return VDPL_E_BOUNDS;}
uint8_t*state=view.data;put(state+12,0);
static const uint8_t offsets[] __attribute__((aligned(4)))={0,1,2,3,4,5,6,7,26,27,30,31,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,};
static const uint8_t values[] __attribute__((aligned(4)))={82,71,1,0,112,0,54,1,4,0,0,0,17,0,198,0,0,0,0,0,48,53,50,82,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
static const uint8_t errors[] __attribute__((aligned(4)))={1,1,2,2,3,3,3,3,3,3,6,6,3,3,3,3,6,6,6,6,6,6,6,6,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,};
for(unsigned i=0;i<sizeof(offsets);++i)if(state[offsets[i]]!=values[i])ROOT_FAIL(errors[i],VDPL_E_ARGUMENT);
if(!get32(state+8))ROOT_FAIL(4,VDPL_E_ARGUMENT);
if(get(state+28)>=17u||get(state+28)!=r->entry_id)ROOT_FAIL(7,VDPL_E_ARGUMENT);
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,2,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=12u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,3,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=24u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,4,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=24u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,5,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=24u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,6,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=12u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,7,1,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=48u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,8,1,0,VDPL_WHOLE_BUFFER,&bound);
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
if(get(state+304)>7999u)FAIL(32);
if(signed16(get(state+306))<-21)FAIL(32);
if(signed16(get(state+306))>21)FAIL(32);
if(get(state+308)>7u)FAIL(32);
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
rc=h->buffer_acquire(h->context,8,VDPL_READ,(uint32_t)(index17-(0))*2u,2,&av18);
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
rc=h->buffer_acquire(h->context,8,VDPL_READ,(uint32_t)(index24-(0))*2u,2,&av25);
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
rc=h->buffer_acquire(h->context,7,VDPL_READ,(uint32_t)(index51-(0))*2u,2,&av52);
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
rc=h->buffer_acquire(h->context,7,VDPL_READ,(uint32_t)(index55-(0))*2u,2,&av56);
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
rc=h->buffer_acquire(h->context,7,VDPL_READ,(uint32_t)(index63-(0))*2u,2,&av64);
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
rc=h->buffer_acquire(h->context,8,VDPL_READ,(uint32_t)(index85-(0))*2u,2,&av86);
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
rc=h->buffer_acquire(h->context,8,VDPL_READ,(uint32_t)(index92-(0))*2u,2,&av93);
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
rc=h->buffer_acquire(h->context,7,VDPL_READ,(uint32_t)(index112-(0))*2u,2,&av113);
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
uint64_t nv138=(uint64_t)(v137)+(uint64_t)(6800u);
if(nv138>4294967295ull)FAIL(1);
uint32_t v138=(uint32_t)nv138;
int64_t nv139=(int64_t)(v138);
if(nv139<-2147483648ll||nv139>2147483647ll)FAIL(32);
uint32_t v139=(uint32_t)nv139;
int64_t nv140=(int64_t)signed32(v136)-(int64_t)signed32(v139);
if(nv140 < -2147483648ll || nv140 >2147483647ll)FAIL(2);
uint32_t v140=(uint32_t)nv140;
put32(state+166,v140);
uint32_t v141=get32(state+166);
uint32_t v142=get32(state+186);
uint32_t v143=v142;
if(!v143)FAIL(4);
if(v141==2147483648u && v143==4294967295u)FAIL(6);
uint32_t v144=(uint32_t)(signed32(v141)/signed32(v143)),v145=(uint32_t)(signed32(v141)%signed32(v143));
put32(state+170,v144);
put32(state+174,v145);
uint32_t v146=get32(state+174);
put32(state+214,v146);
uint32_t v147=get32(state+214);
uint32_t v148=(signed32(v147)<signed32(0u))?1u:0u;
if(v148){
uint32_t v149=get32(state+214);
uint32_t v150=get32(state+186);
int64_t nv151=(int64_t)signed32(v149)+(int64_t)signed32(v150);
if(nv151 < -2147483648ll || nv151 >2147483647ll)FAIL(1);
uint32_t v151=(uint32_t)nv151;
put32(state+214,v151);
}else{
}
uint32_t v152=get32(state+186);
uint32_t v153=2u;
if(!v153)FAIL(4);
if(v152==2147483648u && v153==4294967295u)FAIL(6);
uint32_t v154=(uint32_t)(signed32(v152)/signed32(v153)),v155=(uint32_t)(signed32(v152)%signed32(v153));
put32(state+222,v154);
put32(state+174,v155);
uint32_t v156=get32(state+214);
uint32_t v157=get32(state+222);
uint32_t v158=(signed32(v156)>signed32(v157))?1u:0u;
if(v158){
uint32_t v159=get32(state+214);
uint32_t v160=get32(state+186);
int64_t nv161=(int64_t)signed32(v159)-(int64_t)signed32(v160);
if(nv161 < -2147483648ll || nv161 >2147483647ll)FAIL(2);
uint32_t v161=(uint32_t)nv161;
put32(state+214,v161);
}else{
}
uint32_t v162=get32(state+214);
put32(state+166,v162);
uint32_t v163=get32(state+166);
uint32_t v164=(signed32(v163)<signed32(0u))?1u:0u;
if(v164){
uint32_t v165=get32(state+166);
int64_t nv166=(int64_t)signed32(0u)-(int64_t)signed32(v165);
if(nv166 < -2147483648ll || nv166 >2147483647ll)FAIL(2);
uint32_t v166=(uint32_t)nv166;
put32(state+166,v166);
}else{
}
uint32_t v167=get32(state+210);
uint32_t v168=get32(state+202);
int64_t nv169=(int64_t)signed32(v167)-(int64_t)signed32(v168);
if(nv169 < -2147483648ll || nv169 >2147483647ll)FAIL(2);
uint32_t v169=(uint32_t)nv169;
put32(state+218,v169);
uint32_t v170=get32(state+218);
uint32_t v171=(signed32(v170)<signed32(0u))?1u:0u;
if(v171){
uint32_t v172=get32(state+218);
int64_t nv173=(int64_t)signed32(0u)-(int64_t)signed32(v172);
if(nv173 < -2147483648ll || nv173 >2147483647ll)FAIL(2);
uint32_t v173=(uint32_t)nv173;
put32(state+218,v173);
}else{
}
uint32_t v174=get32(state+166);
uint32_t v175=(signed32(v174)<signed32(3200u))?1u:0u;
uint32_t v176=v175;
if(v176){
uint32_t v177=get32(state+218);
uint32_t v178=(signed32(v177)<signed32(6144u))?1u:0u;
v176=v178;
}
if(v176>1u)FAIL(32);
put(state+226,v176);
break;}
case 3:{
uint32_t v179=get(state+116);
uint32_t v180=(v179==0u)?1u:0u;
if(v180){
uint32_t v181=get(state+114);
uint32_t v182=(uint32_t)(v181)+(uint32_t)(1u);
if(v182>65535u)FAIL(1);
put(state+114,v182);
uint32_t v183=get(state+114);
uint32_t v184=(v183>=6u)?1u:0u;
if(v184){
put(state+114,0u);
}else{
}
uint32_t v185=get(state+114);
uint32_t v186=(uint32_t)(v185)*(uint32_t)(4u);
if(v186>65535u)FAIL(3);
put(state+118,v186);
uint32_t v187=get(state+118);
int32_t index188=(int32_t)(v187);
if(index188<0||index188>23)FAIL(5);
uint32_t v189;{vdplet_view_v1 av189={sizeof(av189),0,0,0,0};
rc=h->buffer_acquire(h->context,7,VDPL_READ,(uint32_t)(index188-(0))*2u,2,&av189);
if(rc){ROOT_FAIL(8,rc);}
v189=get(av189.data);
rc=h->buffer_release(h->context,av189.lease);if(rc){ROOT_FAIL(8,rc);}}
if(v189>32767u)FAIL(32);
put(state+116,v189);
uint32_t v190=get(state+118);
uint32_t v191=(uint32_t)(v190)+(uint32_t)(2u);
if(v191>65535u)FAIL(1);
int32_t index192=(int32_t)(v191);
if(index192<0||index192>23)FAIL(5);
uint32_t v193;{vdplet_view_v1 av193={sizeof(av193),0,0,0,0};
rc=h->buffer_acquire(h->context,7,VDPL_READ,(uint32_t)(index192-(0))*2u,2,&av193);
if(rc){ROOT_FAIL(8,rc);}
v193=get(av193.data);
rc=h->buffer_release(h->context,av193.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv194=(int64_t)(signed16(v193));
if(nv194<-2147483648ll||nv194>2147483647ll)FAIL(32);
uint32_t v194=(uint32_t)nv194;
put32(state+120,v194);
}else{
}
uint32_t v195=get(state+114);
uint32_t v196=(uint32_t)(v195)*(uint32_t)(4u);
if(v196>65535u)FAIL(3);
put(state+118,v196);
uint32_t v197=get32(state+120);
uint32_t v198=get(state+118);
uint32_t v199=(uint32_t)(v198)+(uint32_t)(3u);
if(v199>65535u)FAIL(1);
int32_t index200=(int32_t)(v199);
if(index200<0||index200>23)FAIL(5);
uint32_t v201;{vdplet_view_v1 av201={sizeof(av201),0,0,0,0};
rc=h->buffer_acquire(h->context,7,VDPL_READ,(uint32_t)(index200-(0))*2u,2,&av201);
if(rc){ROOT_FAIL(8,rc);}
v201=get(av201.data);
rc=h->buffer_release(h->context,av201.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv202=(int64_t)(signed16(v201));
if(nv202<-2147483648ll||nv202>2147483647ll)FAIL(32);
uint32_t v202=(uint32_t)nv202;
int64_t nv203=(int64_t)signed32(v197)+(int64_t)signed32(v202);
if(nv203 < -2147483648ll || nv203 >2147483647ll)FAIL(1);
uint32_t v203=(uint32_t)nv203;
put32(state+120,v203);
uint32_t v204=get32(state+120);
put32(state+140,v204);
uint32_t v205=get32(state+140);
if((int64_t)(signed32(v205))<-32768ll)FAIL(32);
if((int64_t)(signed32(v205))>32767ll)FAIL(32);
put32(state+144,v205);
uint32_t v206=get32(state+144);
put32(state+148,v206);
uint32_t v207=get32(state+148);
uint32_t v208=(signed32(v207)<signed32(0u))?1u:0u;
if(v208){
uint32_t v209=get32(state+148);
int64_t nv210=(int64_t)signed32(0u)-(int64_t)signed32(v209);
if(nv210 < -2147483648ll || nv210 >2147483647ll)FAIL(2);
uint32_t v210=(uint32_t)nv210;
put32(state+148,v210);
}else{
}
uint32_t v211=get32(state+148);
uint32_t v212=(signed32(v211)>signed32(16384u))?1u:0u;
if(v212){
uint32_t v213=get32(state+148);
int64_t nv214=(int64_t)signed32(32768u)-(int64_t)signed32(v213);
if(nv214 < -2147483648ll || nv214 >2147483647ll)FAIL(2);
uint32_t v214=(uint32_t)nv214;
put32(state+148,v214);
}else{
}
uint32_t v215=get32(state+148);
uint32_t v216=64u;
if(!v216)FAIL(4);
if(v215==2147483648u && v216==4294967295u)FAIL(6);
uint32_t v217=(uint32_t)(signed32(v215)/signed32(v216)),v218=(uint32_t)(signed32(v215)%signed32(v216));
put32(state+152,v217);
put32(state+156,v218);
uint32_t v219=get32(state+152);
int64_t nv220=(int64_t)(signed32(v219));
if(nv220<0ll||nv220>65535ll)FAIL(32);
uint32_t v220=(uint32_t)nv220&65535u;
put(state+160,v220);
uint32_t v221=get(state+160);
int32_t index222=(int32_t)(v221);
if(index222<0||index222>256)FAIL(5);
uint32_t v223;{vdplet_view_v1 av223={sizeof(av223),0,0,0,0};
rc=h->buffer_acquire(h->context,8,VDPL_READ,(uint32_t)(index222-(0))*2u,2,&av223);
if(rc){ROOT_FAIL(8,rc);}
v223=get(av223.data);
rc=h->buffer_release(h->context,av223.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv224=(int64_t)(signed16(v223));
if(nv224<-2147483648ll||nv224>2147483647ll)FAIL(32);
uint32_t v224=(uint32_t)nv224;
put32(state+162,v224);
uint32_t v225=get(state+160);
uint32_t v226=(v225<256u)?1u:0u;
if(v226){
uint32_t v227=get(state+160);
uint32_t v228=(uint32_t)(v227)+(uint32_t)(1u);
if(v228>65535u)FAIL(1);
int32_t index229=(int32_t)(v228);
if(index229<0||index229>256)FAIL(5);
uint32_t v230;{vdplet_view_v1 av230={sizeof(av230),0,0,0,0};
rc=h->buffer_acquire(h->context,8,VDPL_READ,(uint32_t)(index229-(0))*2u,2,&av230);
if(rc){ROOT_FAIL(8,rc);}
v230=get(av230.data);
rc=h->buffer_release(h->context,av230.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv231=(int64_t)(signed16(v230));
if(nv231<-2147483648ll||nv231>2147483647ll)FAIL(32);
uint32_t v231=(uint32_t)nv231;
uint32_t v232=get32(state+162);
int64_t nv233=(int64_t)signed32(v231)-(int64_t)signed32(v232);
if(nv233 < -2147483648ll || nv233 >2147483647ll)FAIL(2);
uint32_t v233=(uint32_t)nv233;
uint32_t v234=get32(state+156);
int64_t nv235=(int64_t)signed32(v233)*(int64_t)signed32(v234);
if(nv235 < -2147483648ll || nv235 >2147483647ll)FAIL(3);
uint32_t v235=(uint32_t)nv235;
put32(state+166,v235);
uint32_t v236=get32(state+166);
uint32_t v237=64u;
if(!v237)FAIL(4);
if(v236==2147483648u && v237==4294967295u)FAIL(6);
uint32_t v238=(uint32_t)(signed32(v236)/signed32(v237)),v239=(uint32_t)(signed32(v236)%signed32(v237));
put32(state+170,v238);
put32(state+174,v239);
uint32_t v240=get32(state+162);
uint32_t v241=get32(state+170);
int64_t nv242=(int64_t)signed32(v240)+(int64_t)signed32(v241);
if(nv242 < -2147483648ll || nv242 >2147483647ll)FAIL(1);
uint32_t v242=(uint32_t)nv242;
put32(state+162,v242);
}else{
}
uint32_t v243=get32(state+144);
uint32_t v244=(signed32(v243)<signed32(0u))?1u:0u;
if(v244){
uint32_t v245=get32(state+162);
int64_t nv246=(int64_t)signed32(0u)-(int64_t)signed32(v245);
if(nv246 < -2147483648ll || nv246 >2147483647ll)FAIL(2);
uint32_t v246=(uint32_t)nv246;
put32(state+162,v246);
}else{
}
uint32_t v247=get(state+118);
uint32_t v248=(uint32_t)(v247)+(uint32_t)(1u);
if(v248>65535u)FAIL(1);
int32_t index249=(int32_t)(v248);
if(index249<0||index249>23)FAIL(5);
uint32_t v250;{vdplet_view_v1 av250={sizeof(av250),0,0,0,0};
rc=h->buffer_acquire(h->context,7,VDPL_READ,(uint32_t)(index249-(0))*2u,2,&av250);
if(rc){ROOT_FAIL(8,rc);}
v250=get(av250.data);
rc=h->buffer_release(h->context,av250.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv251=(int64_t)(signed16(v250));
if(nv251<-2147483648ll||nv251>2147483647ll)FAIL(32);
uint32_t v251=(uint32_t)nv251;
put32(state+178,v251);
uint32_t v252=get32(state+178);
int64_t nv253=(int64_t)signed32(v252)*(int64_t)signed32(2u);
if(nv253 < -2147483648ll || nv253 >2147483647ll)FAIL(3);
uint32_t v253=(uint32_t)nv253;
uint32_t v254=get32(state+162);
int64_t nv255=(int64_t)signed32(v253)*(int64_t)signed32(v254);
if(nv255 < -2147483648ll || nv255 >2147483647ll)FAIL(3);
uint32_t v255=(uint32_t)nv255;
put32(state+166,v255);
uint32_t v256=get32(state+166);
uint32_t v257=12800u;
if(!v257)FAIL(4);
if(v256==2147483648u && v257==4294967295u)FAIL(6);
uint32_t v258=(uint32_t)(signed32(v256)/signed32(v257)),v259=(uint32_t)(signed32(v256)%signed32(v257));
put32(state+170,v258);
put32(state+174,v259);
uint32_t v260=get32(state+124);
uint32_t v261=get32(state+170);
int64_t nv262=(int64_t)signed32(v260)+(int64_t)signed32(v261);
if(nv262 < -2147483648ll || nv262 >2147483647ll)FAIL(1);
uint32_t v262=(uint32_t)nv262;
put32(state+124,v262);
uint32_t v263=get32(state+178);
int64_t nv264=(int64_t)signed32(v263)*(int64_t)signed32(2u);
if(nv264 < -2147483648ll || nv264 >2147483647ll)FAIL(3);
uint32_t v264=(uint32_t)nv264;
int64_t nv265=(int64_t)(signed32(v264));
if(nv265<0ll||nv265>4294967295ll)FAIL(32);
uint32_t v265=(uint32_t)nv265;
put32(state+132,v265);
uint32_t v266=get32(state+128);
uint32_t v267=get32(state+132);
uint64_t nv268=(uint64_t)(v266)+(uint64_t)(v267);
if(nv268>4294967295ull)FAIL(1);
uint32_t v268=(uint32_t)nv268;
put32(state+128,v268);
uint32_t v269=get(state+116);
if((uint32_t)(v269)<(uint32_t)(1u))FAIL(2);
uint32_t v270=(uint32_t)(v269)-(uint32_t)(1u);
put(state+116,v270);
uint32_t v271=get32(state+182);
if((int64_t)(signed32(v271))<1ll)FAIL(32);
if((int64_t)(signed32(v271))>3276800ll)FAIL(32);
put32(state+186,v271);
if(0u>1u)FAIL(32);
put(state+228,0u);
for(uint32_t loop272=0;loop272<6u;++loop272){
put(state+112,(uint16_t)(0+(int32_t)loop272*(1)));
uint32_t v273=get(state+112);
int32_t index274=(int32_t)(v273);
if(index274<0||index274>5)FAIL(5);
uint32_t v275;{vdplet_view_v1 av275={sizeof(av275),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_READ,(uint32_t)(index274-(0))*2u,2,&av275);
if(rc){ROOT_FAIL(8,rc);}
v275=get(av275.data);
rc=h->buffer_release(h->context,av275.lease);if(rc){ROOT_FAIL(8,rc);}}
if(v275>1u)FAIL(32);
put(state+238,v275);
uint32_t v276=get(state+112);
int32_t index277=(int32_t)(v276);
if(index277<0||index277>5)FAIL(5);
{vdplet_view_v1 store278={sizeof(store278),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index277-(0))*2u,2,&store278);
if(rc){ROOT_FAIL(8,rc);}
put(store278.data,0u);
rc=h->buffer_release(h->context,store278.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v279=get(state+238);
uint32_t v280=(v279==1u)?1u:0u;
if(v280){
uint32_t v281=get(state+112);
int32_t index282=(int32_t)(v281);
if(index282<0||index282>5)FAIL(5);
uint32_t v284=get(state+112);
int32_t index285=(int32_t)(v284);
if(index285<0||index285>5)FAIL(5);
uint32_t v286;{vdplet_view_v1 av286={sizeof(av286),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_READ,(uint32_t)(index285-(0))*4u,4,&av286);
if(rc){ROOT_FAIL(8,rc);}
v286=get32(av286.data);
rc=h->buffer_release(h->context,av286.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v287=get32(state+132);
int64_t nv288=(int64_t)(v287);
if(nv288<-2147483648ll||nv288>2147483647ll)FAIL(32);
uint32_t v288=(uint32_t)nv288;
int64_t nv289=(int64_t)signed32(v286)+(int64_t)signed32(v288);
if(nv289 < -2147483648ll || nv289 >2147483647ll)FAIL(1);
uint32_t v289=(uint32_t)nv289;
{vdplet_view_v1 store283={sizeof(store283),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_WRITE,(uint32_t)(index282-(0))*4u,4,&store283);
if(rc){ROOT_FAIL(8,rc);}
put32(store283.data,v289);
rc=h->buffer_release(h->context,store283.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v290=get(state+112);
int32_t index291=(int32_t)(v290);
if(index291<0||index291>5)FAIL(5);
uint32_t v292;{vdplet_view_v1 av292={sizeof(av292),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_READ,(uint32_t)(index291-(0))*4u,4,&av292);
if(rc){ROOT_FAIL(8,rc);}
v292=get32(av292.data);
rc=h->buffer_release(h->context,av292.lease);if(rc){ROOT_FAIL(8,rc);}}
int64_t nv293=(int64_t)(v292);
if(nv293<-2147483648ll||nv293>2147483647ll)FAIL(32);
uint32_t v293=(uint32_t)nv293;
uint32_t v294=get32(state+132);
int64_t nv295=(int64_t)(v294);
if(nv295<-2147483648ll||nv295>2147483647ll)FAIL(32);
uint32_t v295=(uint32_t)nv295;
int64_t nv296=(int64_t)signed32(v293)+(int64_t)signed32(v295);
if(nv296 < -2147483648ll || nv296 >2147483647ll)FAIL(1);
uint32_t v296=(uint32_t)nv296;
put32(state+166,v296);
uint32_t v297=get32(state+166);
uint32_t v298=get32(state+186);
uint32_t v299=v298;
if(!v299)FAIL(4);
if(v297==2147483648u && v299==4294967295u)FAIL(6);
uint32_t v300=(uint32_t)(signed32(v297)/signed32(v299)),v301=(uint32_t)(signed32(v297)%signed32(v299));
put32(state+170,v300);
put32(state+174,v301);
uint32_t v302=get32(state+174);
uint32_t v303=(signed32(v302)<signed32(0u))?1u:0u;
if(v303){
uint32_t v304=get32(state+174);
uint32_t v305=get32(state+186);
int64_t nv306=(int64_t)signed32(v304)+(int64_t)signed32(v305);
if(nv306 < -2147483648ll || nv306 >2147483647ll)FAIL(1);
uint32_t v306=(uint32_t)nv306;
put32(state+174,v306);
}else{
}
uint32_t v307=get(state+112);
int32_t index308=(int32_t)(v307);
if(index308<0||index308>5)FAIL(5);
uint32_t v310=get32(state+174);
int64_t nv311=(int64_t)(signed32(v310));
if(nv311<0ll||nv311>4294967295ll)FAIL(32);
uint32_t v311=(uint32_t)nv311;
{vdplet_view_v1 store309={sizeof(store309),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index308-(0))*4u,4,&store309);
if(rc){ROOT_FAIL(8,rc);}
put32(store309.data,v311);
rc=h->buffer_release(h->context,store309.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v312=get(state+112);
uint32_t v313=3u;
if(!v313)FAIL(4);
uint32_t v314=v312/v313,v315=v312%v313;
put(state+230,v314);
put(state+232,v315);
uint32_t v316=get(state+232);
int64_t nv317=(int64_t)(v316);
if(nv317<-2147483648ll||nv317>2147483647ll)FAIL(32);
uint32_t v317=(uint32_t)nv317;
int64_t nv318=(int64_t)signed32(v317)-(int64_t)signed32(1u);
if(nv318 < -2147483648ll || nv318 >2147483647ll)FAIL(2);
uint32_t v318=(uint32_t)nv318;
int64_t nv319=(int64_t)signed32(v318)*(int64_t)signed32(9216u);
if(nv319 < -2147483648ll || nv319 >2147483647ll)FAIL(3);
uint32_t v319=(uint32_t)nv319;
if((int64_t)(signed32(v319))<-14260ll)FAIL(32);
if((int64_t)(signed32(v319))>14260ll)FAIL(32);
put32(state+234,v319);
uint32_t v320=get(state+112);
uint32_t v321=2u;
if(!v321)FAIL(4);
uint32_t v322=v320/v321,v323=v320%v321;
put(state+230,v322);
put(state+232,v323);
uint32_t v324=get(state+232);
uint32_t v325=(v324==0u)?1u:0u;
if(v325){
uint32_t v326=get32(state+234);
uint32_t v327=get32(state+124);
int64_t nv328=(int64_t)signed32(v326)+(int64_t)signed32(v327);
if(nv328 < -2147483648ll || nv328 >2147483647ll)FAIL(1);
uint32_t v328=(uint32_t)nv328;
int64_t nv329=(int64_t)signed32(v328)-(int64_t)signed32(5044u);
if(nv329 < -2147483648ll || nv329 >2147483647ll)FAIL(2);
uint32_t v329=(uint32_t)nv329;
if((int64_t)(signed32(v329))<-14260ll)FAIL(32);
if((int64_t)(signed32(v329))>14260ll)FAIL(32);
put32(state+234,v329);
}else{
uint32_t v330=get32(state+234);
uint32_t v331=get32(state+124);
int64_t nv332=(int64_t)signed32(v330)-(int64_t)signed32(v331);
if(nv332 < -2147483648ll || nv332 >2147483647ll)FAIL(2);
uint32_t v332=(uint32_t)nv332;
int64_t nv333=(int64_t)signed32(v332)+(int64_t)signed32(5044u);
if(nv333 < -2147483648ll || nv333 >2147483647ll)FAIL(1);
uint32_t v333=(uint32_t)nv333;
if((int64_t)(signed32(v333))<-14260ll)FAIL(32);
if((int64_t)(signed32(v333))>14260ll)FAIL(32);
put32(state+234,v333);
}
uint32_t v334=get(state+112);
int32_t index335=(int32_t)(v334);
if(index335<0||index335>5)FAIL(5);
uint32_t v337=get32(state+234);
{vdplet_view_v1 store336={sizeof(store336),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_WRITE,(uint32_t)(index335-(0))*4u,4,&store336);
if(rc){ROOT_FAIL(8,rc);}
put32(store336.data,v337);
rc=h->buffer_release(h->context,store336.lease);if(rc){ROOT_FAIL(8,rc);}}
uint32_t v338=get(state+112);
int32_t index339=(int32_t)(v338);
if(index339<0||index339>5)FAIL(5);
uint32_t v340;{vdplet_view_v1 av340={sizeof(av340),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_READ,(uint32_t)(index339-(0))*4u,4,&av340);
if(rc){ROOT_FAIL(8,rc);}
v340=get32(av340.data);
rc=h->buffer_release(h->context,av340.lease);if(rc){ROOT_FAIL(8,rc);}}
put32(state+206,v340);
uint32_t v341=get(state+112);
int32_t index342=(int32_t)(v341);
if(index342<0||index342>5)FAIL(5);
uint32_t v343;{vdplet_view_v1 av343={sizeof(av343),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_READ,(uint32_t)(index342-(0))*4u,4,&av343);
if(rc){ROOT_FAIL(8,rc);}
v343=get32(av343.data);
rc=h->buffer_release(h->context,av343.lease);if(rc){ROOT_FAIL(8,rc);}}
if((int64_t)(signed32(v343))<-14260ll)FAIL(32);
if((int64_t)(signed32(v343))>14260ll)FAIL(32);
put32(state+210,v343);
uint32_t v344=get32(state+182);
if((int64_t)(signed32(v344))<1ll)FAIL(32);
if((int64_t)(signed32(v344))>3276800ll)FAIL(32);
put32(state+186,v344);
uint32_t v345=get32(state+206);
int64_t nv346=(int64_t)(v345);
if(nv346<-2147483648ll||nv346>2147483647ll)FAIL(32);
uint32_t v346=(uint32_t)nv346;
uint32_t v347=get32(state+198);
uint64_t nv348=(uint64_t)(v347)+(uint64_t)(6800u);
if(nv348>4294967295ull)FAIL(1);
uint32_t v348=(uint32_t)nv348;
int64_t nv349=(int64_t)(v348);
if(nv349<-2147483648ll||nv349>2147483647ll)FAIL(32);
uint32_t v349=(uint32_t)nv349;
int64_t nv350=(int64_t)signed32(v346)-(int64_t)signed32(v349);
if(nv350 < -2147483648ll || nv350 >2147483647ll)FAIL(2);
uint32_t v350=(uint32_t)nv350;
put32(state+166,v350);
uint32_t v351=get32(state+166);
uint32_t v352=get32(state+186);
uint32_t v353=v352;
if(!v353)FAIL(4);
if(v351==2147483648u && v353==4294967295u)FAIL(6);
uint32_t v354=(uint32_t)(signed32(v351)/signed32(v353)),v355=(uint32_t)(signed32(v351)%signed32(v353));
put32(state+170,v354);
put32(state+174,v355);
uint32_t v356=get32(state+174);
put32(state+214,v356);
uint32_t v357=get32(state+214);
uint32_t v358=(signed32(v357)<signed32(0u))?1u:0u;
if(v358){
uint32_t v359=get32(state+214);
uint32_t v360=get32(state+186);
int64_t nv361=(int64_t)signed32(v359)+(int64_t)signed32(v360);
if(nv361 < -2147483648ll || nv361 >2147483647ll)FAIL(1);
uint32_t v361=(uint32_t)nv361;
put32(state+214,v361);
}else{
}
uint32_t v362=get32(state+186);
uint32_t v363=2u;
if(!v363)FAIL(4);
if(v362==2147483648u && v363==4294967295u)FAIL(6);
uint32_t v364=(uint32_t)(signed32(v362)/signed32(v363)),v365=(uint32_t)(signed32(v362)%signed32(v363));
put32(state+222,v364);
put32(state+174,v365);
uint32_t v366=get32(state+214);
uint32_t v367=get32(state+222);
uint32_t v368=(signed32(v366)>signed32(v367))?1u:0u;
if(v368){
uint32_t v369=get32(state+214);
uint32_t v370=get32(state+186);
int64_t nv371=(int64_t)signed32(v369)-(int64_t)signed32(v370);
if(nv371 < -2147483648ll || nv371 >2147483647ll)FAIL(2);
uint32_t v371=(uint32_t)nv371;
put32(state+214,v371);
}else{
}
uint32_t v372=get32(state+214);
put32(state+166,v372);
uint32_t v373=get32(state+166);
uint32_t v374=(signed32(v373)<signed32(0u))?1u:0u;
if(v374){
uint32_t v375=get32(state+166);
int64_t nv376=(int64_t)signed32(0u)-(int64_t)signed32(v375);
if(nv376 < -2147483648ll || nv376 >2147483647ll)FAIL(2);
uint32_t v376=(uint32_t)nv376;
put32(state+166,v376);
}else{
}
uint32_t v377=get32(state+210);
uint32_t v378=get32(state+202);
int64_t nv379=(int64_t)signed32(v377)-(int64_t)signed32(v378);
if(nv379 < -2147483648ll || nv379 >2147483647ll)FAIL(2);
uint32_t v379=(uint32_t)nv379;
put32(state+218,v379);
uint32_t v380=get32(state+218);
uint32_t v381=(signed32(v380)<signed32(0u))?1u:0u;
if(v381){
uint32_t v382=get32(state+218);
int64_t nv383=(int64_t)signed32(0u)-(int64_t)signed32(v382);
if(nv383 < -2147483648ll || nv383 >2147483647ll)FAIL(2);
uint32_t v383=(uint32_t)nv383;
put32(state+218,v383);
}else{
}
uint32_t v384=get32(state+166);
uint32_t v385=(signed32(v384)<signed32(3200u))?1u:0u;
uint32_t v386=v385;
if(v386){
uint32_t v387=get32(state+218);
uint32_t v388=(signed32(v387)<signed32(6144u))?1u:0u;
v386=v388;
}
if(v386>1u)FAIL(32);
put(state+226,v386);
uint32_t v389=get(state+226);
if(v389){
uint32_t v390=get(state+112);
int32_t index391=(int32_t)(v390);
if(index391<0||index391>5)FAIL(5);
{vdplet_view_v1 store392={sizeof(store392),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index391-(0))*2u,2,&store392);
if(rc){ROOT_FAIL(8,rc);}
put(store392.data,1u);
rc=h->buffer_release(h->context,store392.lease);if(rc){ROOT_FAIL(8,rc);}}
if(1u>1u)FAIL(32);
put(state+228,1u);
}else{
}
}else{
}
}
uint32_t v393=get32(state+136);
uint64_t nv394=(uint64_t)(v393)+(uint64_t)(1u);
if(nv394>4294967295ull)FAIL(1);
uint32_t v394=(uint32_t)nv394;
put32(state+136,v394);
break;}
case 4:{
uint32_t v395=get32(state+182);
if((int64_t)(signed32(v395))<1ll)FAIL(32);
if((int64_t)(signed32(v395))>3276800ll)FAIL(32);
put32(state+186,v395);
uint32_t v396=get32(state+190);
uint32_t v397=get32(state+186);
uint32_t v398=v397;
if(!v398)FAIL(4);
if(v396==2147483648u && v398==4294967295u)FAIL(6);
uint32_t v399=(uint32_t)(signed32(v396)/signed32(v398)),v400=(uint32_t)(signed32(v396)%signed32(v398));
put32(state+170,v399);
put32(state+174,v400);
uint32_t v401=get32(state+174);
uint32_t v402=(signed32(v401)<signed32(0u))?1u:0u;
if(v402){
uint32_t v403=get32(state+174);
uint32_t v404=get32(state+186);
int64_t nv405=(int64_t)signed32(v403)+(int64_t)signed32(v404);
if(nv405 < -2147483648ll || nv405 >2147483647ll)FAIL(1);
uint32_t v405=(uint32_t)nv405;
put32(state+174,v405);
}else{
}
uint32_t v406=get32(state+174);
int64_t nv407=(int64_t)(signed32(v406));
if(nv407<0ll||nv407>4294967295ll)FAIL(32);
uint32_t v407=(uint32_t)nv407;
put32(state+194,v407);
break;}
case 5:{
uint32_t v408=get32(state+240);
uint32_t v409=get32(state+244);
int64_t nv410=(int64_t)signed32(v408)*(int64_t)signed32(v409);
if(nv410 < -2147483648ll || nv410 >2147483647ll)FAIL(3);
uint32_t v410=(uint32_t)nv410;
put32(state+248,v410);
break;}
case 6:{
uint32_t v411=get32(state+256);
uint32_t v412=get32(state+260);
uint64_t nv413=(uint64_t)(v411)*(uint64_t)(v412);
if(nv413>4294967295ull)FAIL(3);
uint32_t v413=(uint32_t)nv413;
put32(state+264,v413);
break;}
case 7:{
uint32_t v414=get(state+280);
uint32_t v415=(uint32_t)(v414)+(uint32_t)(1u);
if(v415>65535u)FAIL(1);
put(state+280,v415);
uint32_t v416=get32(state+240);
uint32_t v417=get32(state+244);
uint32_t v418=v417;
if(!v418)FAIL(4);
if(v416==2147483648u && v418==4294967295u)FAIL(6);
uint32_t v419=(uint32_t)(signed32(v416)/signed32(v418)),v420=(uint32_t)(signed32(v416)%signed32(v418));
put32(state+248,v419);
put32(state+252,v420);
break;}
case 8:{
uint32_t v421=get(state+280);
uint32_t v422=(uint32_t)(v421)+(uint32_t)(1u);
if(v422>65535u)FAIL(1);
put(state+280,v422);
uint32_t v423=get32(state+256);
uint32_t v424=get32(state+260);
uint32_t v425=v424;
if(!v425)FAIL(4);
uint32_t v426=v423/v425,v427=v423%v425;
put32(state+264,v426);
put32(state+268,v427);
break;}
case 9:{
uint32_t v428=get(state+280);
uint32_t v429=(uint32_t)(v428)+(uint32_t)(1u);
if(v429>65535u)FAIL(1);
put(state+280,v429);
uint32_t v430=get32(state+240);
uint32_t v431=get32(state+244);
uint32_t v432=v431;
if(!v432)FAIL(4);
if(v430==2147483648u && v432==4294967295u)FAIL(6);
uint32_t v433=(uint32_t)(signed32(v430)/signed32(v432)),v434=(uint32_t)(signed32(v430)%signed32(v432));
if((int64_t)(signed32(v433))<-10ll)FAIL(32);
if((int64_t)(signed32(v433))>10ll)FAIL(32);
if((int64_t)(signed32(v434))<-3ll)FAIL(32);
if((int64_t)(signed32(v434))>3ll)FAIL(32);
put32(state+272,v433);
put32(state+276,v434);
break;}
case 10:{
uint32_t v435=get32(state+240);
uint32_t v436=get32(state+244);
uint32_t v437=v436;
if(!v437)FAIL(4);
if(v435==2147483648u && v437==4294967295u)FAIL(6);
uint32_t v438=(uint32_t)(signed32(v435)/signed32(v437)),v439=(uint32_t)(signed32(v435)%signed32(v437));
put32(state+240,v438);
put32(state+244,v439);
break;}
case 11:{
uint32_t v440=get32(state+240);
uint32_t v441=get(state+282);
if(v441>31u)FAIL(32);
uint32_t v442=v440>>v441;
if((v440&2147483648u)&&v441)v442|=4294967295u<<(32u-v441);
put32(state+284,v442);
break;}
case 12:{
uint32_t v443=get32(state+256);
uint32_t v444=get(state+282);
if(v444>31u)FAIL(32);
uint32_t v445=v443>>v444;
put32(state+288,v445);
break;}
case 13:{
put(state+114,0u);
put(state+116,60u);
put(state+118,0u);
put32(state+120,0u);
put32(state+124,0u);
put32(state+128,0u);
put32(state+132,0u);
put32(state+136,0u);
put32(state+198,3270000u);
put32(state+300,3270000u);
if((int64_t)(signed32(0u))<-38400ll)FAIL(32);
if((int64_t)(signed32(0u))>38400ll)FAIL(32);
put32(state+202,0u);
put32(state+292,7u);
put32(state+296,0u);
if(0u>7999u)FAIL(32);
put(state+304,0u);
if(signed16(0u)<-21)FAIL(32);
if(signed16(0u)>21)FAIL(32);
put(state+306,0u);
if(3u>7u)FAIL(32);
put(state+308,3u);
if(0u>1u)FAIL(32);
put(state+228,0u);
int32_t index446=(int32_t)(0u);
if(index446<0||index446>5)FAIL(5);
{vdplet_view_v1 store447={sizeof(store447),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_WRITE,(uint32_t)(index446-(0))*2u,2,&store447);
if(rc){ROOT_FAIL(8,rc);}
put(store447.data,1u);
rc=h->buffer_release(h->context,store447.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index448=(int32_t)(0u);
if(index448<0||index448>5)FAIL(5);
{vdplet_view_v1 store449={sizeof(store449),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_WRITE,(uint32_t)(index448-(0))*4u,4,&store449);
if(rc){ROOT_FAIL(8,rc);}
put32(store449.data,60000u);
rc=h->buffer_release(h->context,store449.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index450=(int32_t)(0u);
if(index450<0||index450>5)FAIL(5);
{vdplet_view_v1 store451={sizeof(store451),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index450-(0))*4u,4,&store451);
if(rc){ROOT_FAIL(8,rc);}
put32(store451.data,60000u);
rc=h->buffer_release(h->context,store451.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index452=(int32_t)(0u);
if(index452<0||index452>5)FAIL(5);
{vdplet_view_v1 store453={sizeof(store453),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_WRITE,(uint32_t)(index452-(0))*4u,4,&store453);
if(rc){ROOT_FAIL(8,rc);}
put32(store453.data,4294953036u);
rc=h->buffer_release(h->context,store453.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index454=(int32_t)(0u);
if(index454<0||index454>5)FAIL(5);
{vdplet_view_v1 store455={sizeof(store455),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index454-(0))*2u,2,&store455);
if(rc){ROOT_FAIL(8,rc);}
put(store455.data,0u);
rc=h->buffer_release(h->context,store455.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index456=(int32_t)(1u);
if(index456<0||index456>5)FAIL(5);
{vdplet_view_v1 store457={sizeof(store457),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_WRITE,(uint32_t)(index456-(0))*2u,2,&store457);
if(rc){ROOT_FAIL(8,rc);}
put(store457.data,1u);
rc=h->buffer_release(h->context,store457.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index458=(int32_t)(1u);
if(index458<0||index458>5)FAIL(5);
{vdplet_view_v1 store459={sizeof(store459),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_WRITE,(uint32_t)(index458-(0))*4u,4,&store459);
if(rc){ROOT_FAIL(8,rc);}
put32(store459.data,50000u);
rc=h->buffer_release(h->context,store459.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index460=(int32_t)(1u);
if(index460<0||index460>5)FAIL(5);
{vdplet_view_v1 store461={sizeof(store461),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index460-(0))*4u,4,&store461);
if(rc){ROOT_FAIL(8,rc);}
put32(store461.data,50000u);
rc=h->buffer_release(h->context,store461.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index462=(int32_t)(1u);
if(index462<0||index462>5)FAIL(5);
{vdplet_view_v1 store463={sizeof(store463),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_WRITE,(uint32_t)(index462-(0))*4u,4,&store463);
if(rc){ROOT_FAIL(8,rc);}
put32(store463.data,5044u);
rc=h->buffer_release(h->context,store463.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index464=(int32_t)(1u);
if(index464<0||index464>5)FAIL(5);
{vdplet_view_v1 store465={sizeof(store465),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index464-(0))*2u,2,&store465);
if(rc){ROOT_FAIL(8,rc);}
put(store465.data,0u);
rc=h->buffer_release(h->context,store465.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index466=(int32_t)(2u);
if(index466<0||index466>5)FAIL(5);
{vdplet_view_v1 store467={sizeof(store467),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_WRITE,(uint32_t)(index466-(0))*2u,2,&store467);
if(rc){ROOT_FAIL(8,rc);}
put(store467.data,1u);
rc=h->buffer_release(h->context,store467.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index468=(int32_t)(2u);
if(index468<0||index468>5)FAIL(5);
{vdplet_view_v1 store469={sizeof(store469),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_WRITE,(uint32_t)(index468-(0))*4u,4,&store469);
if(rc){ROOT_FAIL(8,rc);}
put32(store469.data,40000u);
rc=h->buffer_release(h->context,store469.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index470=(int32_t)(2u);
if(index470<0||index470>5)FAIL(5);
{vdplet_view_v1 store471={sizeof(store471),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index470-(0))*4u,4,&store471);
if(rc){ROOT_FAIL(8,rc);}
put32(store471.data,40000u);
rc=h->buffer_release(h->context,store471.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index472=(int32_t)(2u);
if(index472<0||index472>5)FAIL(5);
{vdplet_view_v1 store473={sizeof(store473),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_WRITE,(uint32_t)(index472-(0))*4u,4,&store473);
if(rc){ROOT_FAIL(8,rc);}
put32(store473.data,4172u);
rc=h->buffer_release(h->context,store473.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index474=(int32_t)(2u);
if(index474<0||index474>5)FAIL(5);
{vdplet_view_v1 store475={sizeof(store475),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index474-(0))*2u,2,&store475);
if(rc){ROOT_FAIL(8,rc);}
put(store475.data,0u);
rc=h->buffer_release(h->context,store475.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index476=(int32_t)(3u);
if(index476<0||index476>5)FAIL(5);
{vdplet_view_v1 store477={sizeof(store477),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_WRITE,(uint32_t)(index476-(0))*2u,2,&store477);
if(rc){ROOT_FAIL(8,rc);}
put(store477.data,1u);
rc=h->buffer_release(h->context,store477.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index478=(int32_t)(3u);
if(index478<0||index478>5)FAIL(5);
{vdplet_view_v1 store479={sizeof(store479),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_WRITE,(uint32_t)(index478-(0))*4u,4,&store479);
if(rc){ROOT_FAIL(8,rc);}
put32(store479.data,30000u);
rc=h->buffer_release(h->context,store479.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index480=(int32_t)(3u);
if(index480<0||index480>5)FAIL(5);
{vdplet_view_v1 store481={sizeof(store481),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index480-(0))*4u,4,&store481);
if(rc){ROOT_FAIL(8,rc);}
put32(store481.data,30000u);
rc=h->buffer_release(h->context,store481.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index482=(int32_t)(3u);
if(index482<0||index482>5)FAIL(5);
{vdplet_view_v1 store483={sizeof(store483),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_WRITE,(uint32_t)(index482-(0))*4u,4,&store483);
if(rc){ROOT_FAIL(8,rc);}
put32(store483.data,4294963124u);
rc=h->buffer_release(h->context,store483.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index484=(int32_t)(3u);
if(index484<0||index484>5)FAIL(5);
{vdplet_view_v1 store485={sizeof(store485),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index484-(0))*2u,2,&store485);
if(rc){ROOT_FAIL(8,rc);}
put(store485.data,0u);
rc=h->buffer_release(h->context,store485.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index486=(int32_t)(4u);
if(index486<0||index486>5)FAIL(5);
{vdplet_view_v1 store487={sizeof(store487),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_WRITE,(uint32_t)(index486-(0))*2u,2,&store487);
if(rc){ROOT_FAIL(8,rc);}
put(store487.data,1u);
rc=h->buffer_release(h->context,store487.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index488=(int32_t)(4u);
if(index488<0||index488>5)FAIL(5);
{vdplet_view_v1 store489={sizeof(store489),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_WRITE,(uint32_t)(index488-(0))*4u,4,&store489);
if(rc){ROOT_FAIL(8,rc);}
put32(store489.data,20000u);
rc=h->buffer_release(h->context,store489.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index490=(int32_t)(4u);
if(index490<0||index490>5)FAIL(5);
{vdplet_view_v1 store491={sizeof(store491),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index490-(0))*4u,4,&store491);
if(rc){ROOT_FAIL(8,rc);}
put32(store491.data,20000u);
rc=h->buffer_release(h->context,store491.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index492=(int32_t)(4u);
if(index492<0||index492>5)FAIL(5);
{vdplet_view_v1 store493={sizeof(store493),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_WRITE,(uint32_t)(index492-(0))*4u,4,&store493);
if(rc){ROOT_FAIL(8,rc);}
put32(store493.data,4294962252u);
rc=h->buffer_release(h->context,store493.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index494=(int32_t)(4u);
if(index494<0||index494>5)FAIL(5);
{vdplet_view_v1 store495={sizeof(store495),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index494-(0))*2u,2,&store495);
if(rc){ROOT_FAIL(8,rc);}
put(store495.data,0u);
rc=h->buffer_release(h->context,store495.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index496=(int32_t)(5u);
if(index496<0||index496>5)FAIL(5);
{vdplet_view_v1 store497={sizeof(store497),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_WRITE,(uint32_t)(index496-(0))*2u,2,&store497);
if(rc){ROOT_FAIL(8,rc);}
put(store497.data,1u);
rc=h->buffer_release(h->context,store497.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index498=(int32_t)(5u);
if(index498<0||index498>5)FAIL(5);
{vdplet_view_v1 store499={sizeof(store499),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_WRITE,(uint32_t)(index498-(0))*4u,4,&store499);
if(rc){ROOT_FAIL(8,rc);}
put32(store499.data,10000u);
rc=h->buffer_release(h->context,store499.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index500=(int32_t)(5u);
if(index500<0||index500>5)FAIL(5);
{vdplet_view_v1 store501={sizeof(store501),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index500-(0))*4u,4,&store501);
if(rc){ROOT_FAIL(8,rc);}
put32(store501.data,10000u);
rc=h->buffer_release(h->context,store501.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index502=(int32_t)(5u);
if(index502<0||index502>5)FAIL(5);
{vdplet_view_v1 store503={sizeof(store503),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_WRITE,(uint32_t)(index502-(0))*4u,4,&store503);
if(rc){ROOT_FAIL(8,rc);}
put32(store503.data,14260u);
rc=h->buffer_release(h->context,store503.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index504=(int32_t)(5u);
if(index504<0||index504>5)FAIL(5);
{vdplet_view_v1 store505={sizeof(store505),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index504-(0))*2u,2,&store505);
if(rc){ROOT_FAIL(8,rc);}
put(store505.data,0u);
rc=h->buffer_release(h->context,store505.lease);if(rc){ROOT_FAIL(8,rc);}}
break;}
case 14:{
put(state+114,0u);
put(state+116,60u);
put(state+118,0u);
put32(state+120,0u);
put32(state+124,0u);
put32(state+128,0u);
put32(state+132,0u);
put32(state+136,0u);
put32(state+198,3270000u);
put32(state+300,3270000u);
if((int64_t)(signed32(0u))<-38400ll)FAIL(32);
if((int64_t)(signed32(0u))>38400ll)FAIL(32);
put32(state+202,0u);
put32(state+292,7u);
put32(state+296,0u);
if(0u>7999u)FAIL(32);
put(state+304,0u);
if(signed16(0u)<-21)FAIL(32);
if(signed16(0u)>21)FAIL(32);
put(state+306,0u);
if(3u>7u)FAIL(32);
put(state+308,3u);
if(0u>1u)FAIL(32);
put(state+228,0u);
int32_t index506=(int32_t)(0u);
if(index506<0||index506>5)FAIL(5);
{vdplet_view_v1 store507={sizeof(store507),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_WRITE,(uint32_t)(index506-(0))*2u,2,&store507);
if(rc){ROOT_FAIL(8,rc);}
put(store507.data,1u);
rc=h->buffer_release(h->context,store507.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index508=(int32_t)(0u);
if(index508<0||index508>5)FAIL(5);
{vdplet_view_v1 store509={sizeof(store509),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_WRITE,(uint32_t)(index508-(0))*4u,4,&store509);
if(rc){ROOT_FAIL(8,rc);}
put32(store509.data,60000u);
rc=h->buffer_release(h->context,store509.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index510=(int32_t)(0u);
if(index510<0||index510>5)FAIL(5);
{vdplet_view_v1 store511={sizeof(store511),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index510-(0))*4u,4,&store511);
if(rc){ROOT_FAIL(8,rc);}
put32(store511.data,60000u);
rc=h->buffer_release(h->context,store511.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index512=(int32_t)(0u);
if(index512<0||index512>5)FAIL(5);
{vdplet_view_v1 store513={sizeof(store513),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_WRITE,(uint32_t)(index512-(0))*4u,4,&store513);
if(rc){ROOT_FAIL(8,rc);}
put32(store513.data,4294953036u);
rc=h->buffer_release(h->context,store513.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index514=(int32_t)(0u);
if(index514<0||index514>5)FAIL(5);
{vdplet_view_v1 store515={sizeof(store515),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index514-(0))*2u,2,&store515);
if(rc){ROOT_FAIL(8,rc);}
put(store515.data,0u);
rc=h->buffer_release(h->context,store515.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index516=(int32_t)(1u);
if(index516<0||index516>5)FAIL(5);
{vdplet_view_v1 store517={sizeof(store517),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_WRITE,(uint32_t)(index516-(0))*2u,2,&store517);
if(rc){ROOT_FAIL(8,rc);}
put(store517.data,1u);
rc=h->buffer_release(h->context,store517.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index518=(int32_t)(1u);
if(index518<0||index518>5)FAIL(5);
{vdplet_view_v1 store519={sizeof(store519),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_WRITE,(uint32_t)(index518-(0))*4u,4,&store519);
if(rc){ROOT_FAIL(8,rc);}
put32(store519.data,50000u);
rc=h->buffer_release(h->context,store519.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index520=(int32_t)(1u);
if(index520<0||index520>5)FAIL(5);
{vdplet_view_v1 store521={sizeof(store521),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index520-(0))*4u,4,&store521);
if(rc){ROOT_FAIL(8,rc);}
put32(store521.data,50000u);
rc=h->buffer_release(h->context,store521.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index522=(int32_t)(1u);
if(index522<0||index522>5)FAIL(5);
{vdplet_view_v1 store523={sizeof(store523),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_WRITE,(uint32_t)(index522-(0))*4u,4,&store523);
if(rc){ROOT_FAIL(8,rc);}
put32(store523.data,5044u);
rc=h->buffer_release(h->context,store523.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index524=(int32_t)(1u);
if(index524<0||index524>5)FAIL(5);
{vdplet_view_v1 store525={sizeof(store525),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index524-(0))*2u,2,&store525);
if(rc){ROOT_FAIL(8,rc);}
put(store525.data,0u);
rc=h->buffer_release(h->context,store525.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index526=(int32_t)(2u);
if(index526<0||index526>5)FAIL(5);
{vdplet_view_v1 store527={sizeof(store527),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_WRITE,(uint32_t)(index526-(0))*2u,2,&store527);
if(rc){ROOT_FAIL(8,rc);}
put(store527.data,1u);
rc=h->buffer_release(h->context,store527.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index528=(int32_t)(2u);
if(index528<0||index528>5)FAIL(5);
{vdplet_view_v1 store529={sizeof(store529),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_WRITE,(uint32_t)(index528-(0))*4u,4,&store529);
if(rc){ROOT_FAIL(8,rc);}
put32(store529.data,40000u);
rc=h->buffer_release(h->context,store529.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index530=(int32_t)(2u);
if(index530<0||index530>5)FAIL(5);
{vdplet_view_v1 store531={sizeof(store531),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index530-(0))*4u,4,&store531);
if(rc){ROOT_FAIL(8,rc);}
put32(store531.data,40000u);
rc=h->buffer_release(h->context,store531.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index532=(int32_t)(2u);
if(index532<0||index532>5)FAIL(5);
{vdplet_view_v1 store533={sizeof(store533),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_WRITE,(uint32_t)(index532-(0))*4u,4,&store533);
if(rc){ROOT_FAIL(8,rc);}
put32(store533.data,4172u);
rc=h->buffer_release(h->context,store533.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index534=(int32_t)(2u);
if(index534<0||index534>5)FAIL(5);
{vdplet_view_v1 store535={sizeof(store535),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index534-(0))*2u,2,&store535);
if(rc){ROOT_FAIL(8,rc);}
put(store535.data,0u);
rc=h->buffer_release(h->context,store535.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index536=(int32_t)(3u);
if(index536<0||index536>5)FAIL(5);
{vdplet_view_v1 store537={sizeof(store537),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_WRITE,(uint32_t)(index536-(0))*2u,2,&store537);
if(rc){ROOT_FAIL(8,rc);}
put(store537.data,1u);
rc=h->buffer_release(h->context,store537.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index538=(int32_t)(3u);
if(index538<0||index538>5)FAIL(5);
{vdplet_view_v1 store539={sizeof(store539),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_WRITE,(uint32_t)(index538-(0))*4u,4,&store539);
if(rc){ROOT_FAIL(8,rc);}
put32(store539.data,30000u);
rc=h->buffer_release(h->context,store539.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index540=(int32_t)(3u);
if(index540<0||index540>5)FAIL(5);
{vdplet_view_v1 store541={sizeof(store541),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index540-(0))*4u,4,&store541);
if(rc){ROOT_FAIL(8,rc);}
put32(store541.data,30000u);
rc=h->buffer_release(h->context,store541.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index542=(int32_t)(3u);
if(index542<0||index542>5)FAIL(5);
{vdplet_view_v1 store543={sizeof(store543),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_WRITE,(uint32_t)(index542-(0))*4u,4,&store543);
if(rc){ROOT_FAIL(8,rc);}
put32(store543.data,4294963124u);
rc=h->buffer_release(h->context,store543.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index544=(int32_t)(3u);
if(index544<0||index544>5)FAIL(5);
{vdplet_view_v1 store545={sizeof(store545),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index544-(0))*2u,2,&store545);
if(rc){ROOT_FAIL(8,rc);}
put(store545.data,0u);
rc=h->buffer_release(h->context,store545.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index546=(int32_t)(4u);
if(index546<0||index546>5)FAIL(5);
{vdplet_view_v1 store547={sizeof(store547),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_WRITE,(uint32_t)(index546-(0))*2u,2,&store547);
if(rc){ROOT_FAIL(8,rc);}
put(store547.data,1u);
rc=h->buffer_release(h->context,store547.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index548=(int32_t)(4u);
if(index548<0||index548>5)FAIL(5);
{vdplet_view_v1 store549={sizeof(store549),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_WRITE,(uint32_t)(index548-(0))*4u,4,&store549);
if(rc){ROOT_FAIL(8,rc);}
put32(store549.data,20000u);
rc=h->buffer_release(h->context,store549.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index550=(int32_t)(4u);
if(index550<0||index550>5)FAIL(5);
{vdplet_view_v1 store551={sizeof(store551),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index550-(0))*4u,4,&store551);
if(rc){ROOT_FAIL(8,rc);}
put32(store551.data,20000u);
rc=h->buffer_release(h->context,store551.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index552=(int32_t)(4u);
if(index552<0||index552>5)FAIL(5);
{vdplet_view_v1 store553={sizeof(store553),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_WRITE,(uint32_t)(index552-(0))*4u,4,&store553);
if(rc){ROOT_FAIL(8,rc);}
put32(store553.data,4294962252u);
rc=h->buffer_release(h->context,store553.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index554=(int32_t)(4u);
if(index554<0||index554>5)FAIL(5);
{vdplet_view_v1 store555={sizeof(store555),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index554-(0))*2u,2,&store555);
if(rc){ROOT_FAIL(8,rc);}
put(store555.data,0u);
rc=h->buffer_release(h->context,store555.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index556=(int32_t)(5u);
if(index556<0||index556>5)FAIL(5);
{vdplet_view_v1 store557={sizeof(store557),0,0,0,0};
rc=h->buffer_acquire(h->context,2,VDPL_WRITE,(uint32_t)(index556-(0))*2u,2,&store557);
if(rc){ROOT_FAIL(8,rc);}
put(store557.data,1u);
rc=h->buffer_release(h->context,store557.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index558=(int32_t)(5u);
if(index558<0||index558>5)FAIL(5);
{vdplet_view_v1 store559={sizeof(store559),0,0,0,0};
rc=h->buffer_acquire(h->context,3,VDPL_WRITE,(uint32_t)(index558-(0))*4u,4,&store559);
if(rc){ROOT_FAIL(8,rc);}
put32(store559.data,10000u);
rc=h->buffer_release(h->context,store559.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index560=(int32_t)(5u);
if(index560<0||index560>5)FAIL(5);
{vdplet_view_v1 store561={sizeof(store561),0,0,0,0};
rc=h->buffer_acquire(h->context,4,VDPL_WRITE,(uint32_t)(index560-(0))*4u,4,&store561);
if(rc){ROOT_FAIL(8,rc);}
put32(store561.data,10000u);
rc=h->buffer_release(h->context,store561.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index562=(int32_t)(5u);
if(index562<0||index562>5)FAIL(5);
{vdplet_view_v1 store563={sizeof(store563),0,0,0,0};
rc=h->buffer_acquire(h->context,5,VDPL_WRITE,(uint32_t)(index562-(0))*4u,4,&store563);
if(rc){ROOT_FAIL(8,rc);}
put32(store563.data,14260u);
rc=h->buffer_release(h->context,store563.lease);if(rc){ROOT_FAIL(8,rc);}}
int32_t index564=(int32_t)(5u);
if(index564<0||index564>5)FAIL(5);
{vdplet_view_v1 store565={sizeof(store565),0,0,0,0};
rc=h->buffer_acquire(h->context,6,VDPL_WRITE,(uint32_t)(index564-(0))*2u,2,&store565);
if(rc){ROOT_FAIL(8,rc);}
put(store565.data,0u);
rc=h->buffer_release(h->context,store565.lease);if(rc){ROOT_FAIL(8,rc);}}
break;}
case 15:{
uint32_t v566=get32(state+296);
uint64_t nv567=(uint64_t)(v566)+(uint64_t)(1u);
if(nv567>4294967295ull)FAIL(1);
uint32_t v567=(uint32_t)nv567;
put32(state+296,v567);
break;}
case 16:{
if(0u>7u)FAIL(32);
put(state+308,0u);
break;}
}
rc=renderer_hook(h,r->entry_id,state);if(rc){int32_t rr=h->buffer_release(h->context,view.lease);return rr?rr:rc;}put32(state+20,get32(state+20)+1u);put(state+24,r->entry_id);put32(state+32,get32(state+8));
return h->buffer_release(h->context,view.lease);
}
