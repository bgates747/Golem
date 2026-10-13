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
if(r->operation!=VDPL_RUN||r->entry_id>=38)return VDPL_E_UNSUPPORTED;
if(r->argument_bytes!=2)return VDPL_E_ARGUMENT;
vdplet_view_v1 args={sizeof(args),0,0,0,0},view={sizeof(view),0,0,0,0};
int32_t rc=h->buffer_acquire(h->context,r->argument_buffer,VDPL_READ,0,2,&args);if(rc)return rc;
uint16_t id=get(args.data);rc=h->buffer_acquire(h->context,id,VDPL_WRITE,0,VDPL_WHOLE_BUFFER,&view);if(rc){h->buffer_release(h->context,args.lease);return rc;}
if(view.bytes!=154){h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return VDPL_E_BOUNDS;}
uint8_t*state=view.data;put(state+12,0);
static const uint8_t offsets[] __attribute__((aligned(4)))={0,1,2,3,4,5,6,7,26,27,30,31,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,};
static const uint8_t values[] __attribute__((aligned(4)))={82,71,1,0,112,0,154,0,4,0,0,0,38,0,42,0,0,0,0,0,48,53,50,82,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
static const uint8_t errors[] __attribute__((aligned(4)))={1,1,2,2,3,3,3,3,3,3,6,6,3,3,3,3,6,6,6,6,6,6,6,6,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,};
for(unsigned i=0;i<sizeof(offsets);++i)if(state[offsets[i]]!=values[i])ROOT_FAIL(errors[i],VDPL_E_ARGUMENT);
if(!get32(state+8))ROOT_FAIL(4,VDPL_E_ARGUMENT);
if(get(state+28)>=38u||get(state+28)!=r->entry_id)ROOT_FAIL(7,VDPL_E_ARGUMENT);
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,256,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=16u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context,257,2,0,VDPL_WHOLE_BUFFER,&bound);
if(rc){ROOT_FAIL(8,rc);}
uint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);
if(rc){ROOT_FAIL(8,rc);}
if(shape!=16u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}
put(state+14,0);
if(get(state+112)>1u)FAIL(32);
if((int64_t)(get32(state+142))<65536ll)FAIL(32);
if((int64_t)(get32(state+142))>6553600ll)FAIL(32);
if((int64_t)(signed32(get32(state+146)))<-38400ll)FAIL(32);
if((int64_t)(signed32(get32(state+146)))>38400ll)FAIL(32);
switch(r->entry_id){
case 0:{
uint32_t v0=get32(state+114);
uint32_t v1=get32(state+118);
uint64_t nv2=(uint64_t)(v0)+(uint64_t)(v1);
if(nv2>4294967295ull)FAIL(1);
uint32_t v2=(uint32_t)nv2;
put32(state+122,v2);
break;}
case 1:{
uint32_t v3=get32(state+114);
uint32_t v4=get32(state+118);
if(v3<v4)FAIL(2);
uint32_t v5=v3-v4;
put32(state+122,v5);
break;}
case 2:{
uint32_t v6=get32(state+126);
uint32_t v7=get32(state+130);
int64_t nv8=(int64_t)signed32(v6)+(int64_t)signed32(v7);
if(nv8 < -2147483648ll || nv8 >2147483647ll)FAIL(1);
uint32_t v8=(uint32_t)nv8;
put32(state+134,v8);
break;}
case 3:{
uint32_t v9=get32(state+126);
uint32_t v10=get32(state+130);
int64_t nv11=(int64_t)signed32(v9)-(int64_t)signed32(v10);
if(nv11 < -2147483648ll || nv11 >2147483647ll)FAIL(2);
uint32_t v11=(uint32_t)nv11;
put32(state+134,v11);
break;}
case 4:{
uint32_t v12=get32(state+114);
uint32_t v13=get32(state+118);
uint32_t v14=(v12<v13)?1u:0u;
if(v14>1u)FAIL(32);
put(state+112,v14);
break;}
case 5:{
uint32_t v15=get32(state+126);
uint32_t v16=get32(state+130);
uint32_t v17=(signed32(v15)>=signed32(v16))?1u:0u;
if(v17>1u)FAIL(32);
put(state+112,v17);
break;}
case 6:{
uint32_t v18=get32(state+114);
int64_t nv19=(int64_t)(v18);
if(nv19<-2147483648ll||nv19>2147483647ll)FAIL(32);
uint32_t v19=(uint32_t)nv19;
put32(state+134,v19);
break;}
case 7:{
uint32_t v20=get32(state+126);
int64_t nv21=(int64_t)(signed32(v20));
if(nv21<0ll||nv21>4294967295ll)FAIL(32);
uint32_t v21=(uint32_t)nv21;
put32(state+122,v21);
break;}
case 8:{
uint32_t v22=get32(state+114);
int64_t nv23=(int64_t)(v22);
if(nv23<0ll||nv23>65535ll)FAIL(32);
uint32_t v23=(uint32_t)nv23&65535u;
put(state+138,v23);
break;}
case 9:{
uint32_t v24=get32(state+126);
int64_t nv25=(int64_t)(signed32(v24));
if(nv25<-32768ll||nv25>32767ll)FAIL(32);
uint32_t v25=(uint32_t)nv25&65535u;
put(state+140,v25);
break;}
case 10:{
uint32_t v26=get32(state+126);
int64_t nv27=(int64_t)(signed32(v26));
if(nv27<0ll||nv27>65535ll)FAIL(32);
uint32_t v27=(uint32_t)nv27&65535u;
put(state+138,v27);
break;}
case 11:{
uint32_t v28=get32(state+114);
int64_t nv29=(int64_t)(v28);
if(nv29<-32768ll||nv29>32767ll)FAIL(32);
uint32_t v29=(uint32_t)nv29&65535u;
put(state+140,v29);
break;}
case 12:{
uint32_t v30=get(state+138);
int64_t nv31=(int64_t)(v30);
if(nv31<-2147483648ll||nv31>2147483647ll)FAIL(32);
uint32_t v31=(uint32_t)nv31;
put32(state+134,v31);
uint32_t v32=get(state+138);
int64_t nv33=(int64_t)(v32);
if(nv33<0ll||nv33>4294967295ll)FAIL(32);
uint32_t v33=(uint32_t)nv33;
put32(state+122,v33);
break;}
case 13:{
uint32_t v34=get(state+140);
int64_t nv35=(int64_t)(signed16(v34));
if(nv35<-2147483648ll||nv35>2147483647ll)FAIL(32);
uint32_t v35=(uint32_t)nv35;
put32(state+134,v35);
break;}
case 14:{
uint32_t v36=get(state+140);
int64_t nv37=(int64_t)(signed16(v36));
if(nv37<0ll||nv37>4294967295ll)FAIL(32);
uint32_t v37=(uint32_t)nv37;
put32(state+122,v37);
break;}
case 15:{
uint32_t v38=get(state+150);
int32_t index39=(int32_t)(v38);
if(index39<0||index39>3)FAIL(5);
uint32_t v40;{vdplet_view_v1 av40={sizeof(av40),0,0,0,0};
rc=h->buffer_acquire(h->context,256,VDPL_READ,(uint32_t)(index39-(0))*4u,4,&av40);
if(rc){ROOT_FAIL(8,rc);}
v40=get32(av40.data);
rc=h->buffer_release(h->context,av40.lease);if(rc){ROOT_FAIL(8,rc);}}
put32(state+122,v40);
break;}
case 16:{
uint32_t v41=get(state+152);
int32_t index42=signed16(v41);
if(index42<-2||index42>1)FAIL(5);
uint32_t v43;{vdplet_view_v1 av43={sizeof(av43),0,0,0,0};
rc=h->buffer_acquire(h->context,257,VDPL_READ,(uint32_t)(index42-(-2))*4u,4,&av43);
if(rc){ROOT_FAIL(8,rc);}
v43=get32(av43.data);
rc=h->buffer_release(h->context,av43.lease);if(rc){ROOT_FAIL(8,rc);}}
put32(state+134,v43);
break;}
case 17:{
uint32_t v44=get(state+150);
int32_t index45=(int32_t)(v44);
if(index45<0||index45>3)FAIL(5);
uint32_t v47=get32(state+114);
uint32_t v48=get32(state+118);
uint64_t nv49=(uint64_t)(v47)+(uint64_t)(v48);
if(nv49>4294967295ull)FAIL(1);
uint32_t v49=(uint32_t)nv49;
{vdplet_view_v1 store46={sizeof(store46),0,0,0,0};
rc=h->buffer_acquire(h->context,256,VDPL_WRITE,(uint32_t)(index45-(0))*4u,4,&store46);
if(rc){ROOT_FAIL(8,rc);}
put32(store46.data,v49);
rc=h->buffer_release(h->context,store46.lease);if(rc){ROOT_FAIL(8,rc);}}
break;}
case 18:{
uint32_t v50=get(state+152);
int32_t index51=signed16(v50);
if(index51<-2||index51>1)FAIL(5);
uint32_t v53=get32(state+126);
uint32_t v54=get32(state+130);
int64_t nv55=(int64_t)signed32(v53)-(int64_t)signed32(v54);
if(nv55 < -2147483648ll || nv55 >2147483647ll)FAIL(2);
uint32_t v55=(uint32_t)nv55;
{vdplet_view_v1 store52={sizeof(store52),0,0,0,0};
rc=h->buffer_acquire(h->context,257,VDPL_WRITE,(uint32_t)(index51-(-2))*4u,4,&store52);
if(rc){ROOT_FAIL(8,rc);}
put32(store52.data,v55);
rc=h->buffer_release(h->context,store52.lease);if(rc){ROOT_FAIL(8,rc);}}
break;}
case 19:{
uint32_t v56=get32(state+114);
if((int64_t)(v56)<65536ll)FAIL(32);
if((int64_t)(v56)>6553600ll)FAIL(32);
put32(state+142,v56);
break;}
case 20:{
uint32_t v57=get32(state+126);
if((int64_t)(signed32(v57))<-38400ll)FAIL(32);
if((int64_t)(signed32(v57))>38400ll)FAIL(32);
put32(state+146,v57);
break;}
case 21:{
uint32_t v58=get32(state+114);
int64_t nv59=(int64_t)(v58);
if(nv59<0ll||nv59>65535ll)FAIL(32);
uint32_t v59=(uint32_t)nv59&65535u;
int32_t index60=(int32_t)(v59);
if(index60<0||index60>3)FAIL(5);
uint32_t v61;{vdplet_view_v1 av61={sizeof(av61),0,0,0,0};
rc=h->buffer_acquire(h->context,256,VDPL_READ,(uint32_t)(index60-(0))*4u,4,&av61);
if(rc){ROOT_FAIL(8,rc);}
v61=get32(av61.data);
rc=h->buffer_release(h->context,av61.lease);if(rc){ROOT_FAIL(8,rc);}}
put32(state+122,v61);
break;}
case 22:{
uint32_t v62=get32(state+114);
uint32_t v63=get32(state+118);
uint32_t v64=(v62==v63)?1u:0u;
if(v64>1u)FAIL(32);
put(state+112,v64);
break;}
case 23:{
uint32_t v65=get32(state+114);
uint32_t v66=get32(state+118);
uint32_t v67=(v65!=v66)?1u:0u;
if(v67>1u)FAIL(32);
put(state+112,v67);
break;}
case 24:{
uint32_t v68=get32(state+114);
uint32_t v69=get32(state+118);
uint32_t v70=(v68<v69)?1u:0u;
if(v70>1u)FAIL(32);
put(state+112,v70);
break;}
case 25:{
uint32_t v71=get32(state+114);
uint32_t v72=get32(state+118);
uint32_t v73=(v71>v72)?1u:0u;
if(v73>1u)FAIL(32);
put(state+112,v73);
break;}
case 26:{
uint32_t v74=get32(state+114);
uint32_t v75=get32(state+118);
uint32_t v76=(v74<=v75)?1u:0u;
if(v76>1u)FAIL(32);
put(state+112,v76);
break;}
case 27:{
uint32_t v77=get32(state+114);
uint32_t v78=get32(state+118);
uint32_t v79=(v77>=v78)?1u:0u;
if(v79>1u)FAIL(32);
put(state+112,v79);
break;}
case 28:{
uint32_t v80=get32(state+126);
uint32_t v81=get32(state+130);
uint32_t v82=(signed32(v80)==signed32(v81))?1u:0u;
if(v82>1u)FAIL(32);
put(state+112,v82);
break;}
case 29:{
uint32_t v83=get32(state+126);
uint32_t v84=get32(state+130);
uint32_t v85=(signed32(v83)!=signed32(v84))?1u:0u;
if(v85>1u)FAIL(32);
put(state+112,v85);
break;}
case 30:{
uint32_t v86=get32(state+126);
uint32_t v87=get32(state+130);
uint32_t v88=(signed32(v86)<signed32(v87))?1u:0u;
if(v88>1u)FAIL(32);
put(state+112,v88);
break;}
case 31:{
uint32_t v89=get32(state+126);
uint32_t v90=get32(state+130);
uint32_t v91=(signed32(v89)>signed32(v90))?1u:0u;
if(v91>1u)FAIL(32);
put(state+112,v91);
break;}
case 32:{
uint32_t v92=get32(state+126);
uint32_t v93=get32(state+130);
uint32_t v94=(signed32(v92)<=signed32(v93))?1u:0u;
if(v94>1u)FAIL(32);
put(state+112,v94);
break;}
case 33:{
uint32_t v95=get32(state+126);
uint32_t v96=get32(state+130);
uint32_t v97=(signed32(v95)>=signed32(v96))?1u:0u;
if(v97>1u)FAIL(32);
put(state+112,v97);
break;}
case 34:{
uint32_t v98=get32(state+114);
int64_t nv99=(int64_t)(v98);
if(nv99<0ll||nv99>4294967295ll)FAIL(32);
uint32_t v99=(uint32_t)nv99;
put32(state+122,v99);
break;}
case 35:{
uint32_t v100=get32(state+126);
int64_t nv101=(int64_t)(signed32(v100));
if(nv101<-2147483648ll||nv101>2147483647ll)FAIL(32);
uint32_t v101=(uint32_t)nv101;
put32(state+134,v101);
break;}
case 36:{
uint32_t v102=get32(state+114);
uint32_t v103=get32(state+118);
uint64_t nv104=(uint64_t)(v102)+(uint64_t)(v103);
if(nv104>4294967295ull)FAIL(1);
uint32_t v104=(uint32_t)nv104;
put32(state+122,v104);
uint32_t v105=get32(state+114);
if((int64_t)(v105)<65536ll)FAIL(32);
if((int64_t)(v105)>6553600ll)FAIL(32);
put32(state+142,v105);
break;}
case 37:{
uint32_t v106=0u;
if(v106){
uint32_t v107=get32(state+114);
int64_t nv108=(int64_t)(v107);
if(nv108<0ll||nv108>65535ll)FAIL(32);
uint32_t v108=(uint32_t)nv108&65535u;
uint32_t v109=(v108==0u)?1u:0u;
v106=v109;
}
if(v106>1u)FAIL(32);
put(state+112,v106);
break;}
}
put32(state+20,get32(state+20)+1u);put(state+24,r->entry_id);put32(state+32,get32(state+8));
h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return 0;
}
