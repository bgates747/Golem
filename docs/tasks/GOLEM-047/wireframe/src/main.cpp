// Finite service review: native C++ transforms and native VDU byte submission.
#include <agon/mos.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <agnb_agondev.hpp>
#include "generated/config.h"
extern "C" {
volatile uint8_t cpp_reply_ready,cpp_reply[10];
void cpp_packet_callback(void);
}
static_assert(sizeof(float)==4 && sizeof(long)==4,"AgonDev transport types");
namespace {
agnb::Workspace workspace;
uint16_t token,handle,last_error,frame_count;
uint8_t stage;
bool resources_started,ownership_uncertain;
FILE *logfile;
void send(const void *p,unsigned n){mos_puts(static_cast<const char*>(p),n,0);}
void put16(uint8_t *p,uint16_t n){p[0]=uint8_t(n);p[1]=uint8_t(n>>8);}
void put32(uint8_t *p,uint32_t n){put16(p,uint16_t(n));put16(p+2,uint16_t(n>>16));}
uint16_t get16(const volatile uint8_t *p){return uint16_t(p[0])|(uint16_t(p[1])<<8);}
uint32_t ticks(){uint32_t a,b;do{a=sys_vars->time;b=sys_vars->time;}while(a!=b);return a;}
void note(const char *s){if(logfile){fputs(s,logfile);fputc('\n',logfile);fflush(logfile);}}
void command(uint8_t *p,uint16_t id,uint8_t op){p[0]=23;p[1]=0;p[2]=160;put16(p+3,id);p[5]=op;}
void call(uint16_t id){uint8_t p[6];command(p,id,1);send(p,sizeof p);}
void clear(uint16_t id){uint8_t p[6];command(p,id,2);send(p,sizeof p);}
void update(uint16_t id,uint16_t offset,const void *data,uint16_t n){
 uint8_t p[11];command(p,id,5);p[6]=0xc2;put16(p+7,offset);put16(p+9,n);send(p,sizeof p);send(data,n);
}
bool request(uint8_t op,const uint8_t *fields,uint16_t size,uint8_t kind,uint16_t &value){
 uint8_t p[40]={23,0,216,0,0,1,op,0,0,1,0};put16(p+3,size+6);put16(p+7,++token);memcpy(p+11,fields,size);
 cpp_reply_ready=0;send(p,size+11);uint32_t start=ticks();unsigned long spins=0;
 for(;;){
  if(cpp_reply_ready){uint8_t r[10];for(unsigned i=0;i<10;++i)r[i]=cpp_reply[i];cpp_reply_ready=0;
   if(r[4]!=1||get16(r+6)!=token)continue;
   value=get16(r+8);if(r[5]!=kind){last_error=value;return false;}return true;
  }
  if(ticks()-start>1200u||++spins>7000000UL){last_error=65535;return false;}
 }
}
bool info(uint16_t field,uint16_t &value){uint8_t p[2];put16(p,field);return request(4,p,2,4,value);}
bool read16(uint16_t id,uint16_t offset,uint16_t &value){uint8_t p[6];put16(p,id);put32(p+2,offset);return request(5,p,6,4,value);}
bool fence(){
 volatile uint8_t *gp=reinterpret_cast<volatile uint8_t*>(sys_vars)+sysvar_gp;
 uint8_t tag=uint8_t(*gp+1),p[]={23,0,128,tag};send(p,sizeof p);uint32_t start=ticks();unsigned long spins=0;
 while(*gp!=tag)if(ticks()-start>1200u||++spins>7000000UL){last_error=65534;return false;}
 return true;
}
void cursor(uint8_t x,uint8_t y){uint8_t p[]={31,x,y};send(p,sizeof p);}
void preamble(){
 const uint8_t p[]={12,18,0,REVIEW_BENCHMARK?15:3,31,0,0};send(p,sizeof p);
 if(REVIEW_BENCHMARK){
  printf("VDPlet / corsair / 2685v 4903e");cursor(0,45);printf("WASD rotate Q/E roll +/- distance");cursor(0,46);printf("IJKL camera R reset Esc exit");
 }else{
  printf("VDPlet submits VDU command bytes");cursor(0,1);printf("Native C++ transform + native chunked drawing; existing VDP rasterizers");
  cursor(0,92);printf("Corsair: 2685 vertices, 4903 edges. Frame %u / %u",unsigned(frame_count+1),unsigned(REVIEW_FRAMES));
 }
}
bool render(const float *controls,bool native,uint32_t &math,uint32_t &drawing,uint32_t &full){
 uint32_t start=ticks();update(STATE_ID,CONTROLS_OFFSET,controls,32);uint8_t args[6];put16(args,handle);put16(args+2,0);put16(args+4,ARGUMENT_ID);
 uint16_t value=0;if(!request(2,args,sizeof args,3,value)||value){last_error=value;return false;}
 if(!read16(STATE_ID,12,value)||value||!read16(STATE_ID,14,value)||value){last_error=value;return false;}
 uint32_t transformed=ticks();preamble();call(native?NATIVE_FRAME_ID:BASELINE_FRAME_ID);
 if(!fence()||!info(3,value)||value){last_error=value;return false;}
 uint32_t end=ticks();math=transformed-start;drawing=end-transformed;full=end-start;++frame_count;return true;
}
bool load(){
 uint16_t idle=0;if(!info(2,idle)||idle){last_error=7;return false;}
 resources_started=true;auto rc=agnb::load_buffers_mos("scene.agnb",workspace);if(rc!=agnb::Error::Ok){last_error=uint16_t(rc);return false;}
 if(REVIEW_BENCHMARK){rc=agnb::load_buffers_mos("baseline.agnb",workspace);if(rc!=agnb::Error::Ok){last_error=uint16_t(rc);return false;}}
 uint8_t fields[8];put16(fields,MODULE_ID);put32(fields+2,MODULE_BYTES);put16(fields+6,0);
 ownership_uncertain=true;
 if(!request(1,fields,sizeof fields,2,handle)||!handle)return false;ownership_uncertain=false;
 uint8_t bytes[2];put16(bytes,handle);update(NATIVE_FRAME_ID,11,bytes,2);return fence();
}
bool cleanup(){
 // Drain raster work before releasing code or data, including failure paths.
 const uint8_t drain[]={23,0,202};send(drain,sizeof drain);if(!fence())return false;
 if(ownership_uncertain)return false;
 if(handle){uint8_t p[2];put16(p,handle);uint16_t rc;if(!request(3,p,2,3,rc)||rc)return false;handle=0;}
 if(resources_started)for(uint16_t id:OWNED_IDS)clear(id);return fence();
}
}
int main(){
 logfile=fopen("review.log","wb");note("START");mos_setkbvector(cpp_packet_callback,0);
 uint16_t capabilities=0;bool passed=false;stage=1;
 if(info(1,capabilities)&&(capabilities&5)==5&&load()){
  stage=2;uint8_t mode[]={22,REVIEW_BENCHMARK?20:19,23,0,192,1,23,1,0,17,REVIEW_BENCHMARK?15:3};send(mode,sizeof mode);fence();
  if(REVIEW_BENCHMARK){
   FILE *csv=fopen("benchmark.csv","wb");if(csv)fputs("order,pose,arm,math_cs,draw_cs,full_cs\n",csv);
   passed=csv!=nullptr;
   for(unsigned order=0;order<2&&passed;++order)for(unsigned pose=0;pose<6&&passed;++pose)for(unsigned n=0;n<2&&passed;++n){
    bool native=(n==0)==(order==1);uint32_t math=0,drawing=0,full=0;
    passed=render(REVIEW_POSES[pose],native,math,drawing,full);
    if(passed)fprintf(csv,"%u,%u,%u,%lu,%lu,%lu\n",order,pose,unsigned(native),(unsigned long)math,(unsigned long)drawing,(unsigned long)full);
   }
   if(csv)fclose(csv);
   uint32_t math,drawing,full;if(passed)passed=render(REVIEW_POSES[5],BENCH_END_SUBMIT,math,drawing,full);
  }else{
   passed=true;
   for(unsigned frame=0;frame<REVIEW_FRAMES&&passed;++frame){
    float controls[8]={20,float(-180+int(frame)*15),0,0,0,0,1.0f/3.0f,24576};
    uint32_t math,drawing,full;passed=render(controls,true,math,drawing,full);
   }
  }
 }
 stage=3;uint16_t low=0,high=0,stack=0;
 if(passed)passed=read16(STATE_ID,20,low)&&read16(STATE_ID,22,high)&&high==0&&low==frame_count&&info(17,stack);
 bool cleaned=cleanup();if(!cleaned)passed=false;
 uint16_t slot=65535;if(passed)passed=info(2,slot)&&slot==0;
 mos_setkbvector(nullptr,0);
 const uint8_t viewport[]={28,0,REVIEW_BENCHMARK?47:95,REVIEW_BENCHMARK?63:127,REVIEW_BENCHMARK?42:88,31,0,0,23,1,1};send(viewport,sizeof viewport);
 if(passed){
  uint8_t result[20]={'V','D','U','1'};put16(result+4,capabilities);put16(result+6,frame_count);put16(result+8,low);put16(result+10,stack);put16(result+12,slot);put16(result+14,REVIEW_BENCHMARK);put16(result+16,BENCH_END_SUBMIT);put16(result+18,0);
  FILE *f=fopen("results.dat","wb");if(f){fwrite(result,1,sizeof result,f);fclose(f);}else passed=false;
 }
 if(passed){note("PASS");printf("VDU submission review passed.\r\n");}
 else{note("FAIL");uint8_t failure[]={stage,uint8_t(last_error),uint8_t(last_error>>8)};FILE *f=fopen("failure.dat","wb");if(f){fwrite(failure,1,sizeof failure,f);fclose(f);}printf("Review failed: %u; %s\r\n",unsigned(last_error),cleaned?"resources released":"reset required");}
 if(logfile)fclose(logfile);
 if(passed){printf("Type dir at the MOS prompt and report to the agent that it works.\r\n");}
 return passed?0:1;
}
