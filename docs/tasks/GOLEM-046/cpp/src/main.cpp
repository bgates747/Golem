// Qualification application: C++ loads/calls; Golem supplies math and drawing.
#include <agon/mos.h>
#include <agon/vdp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <agnb_agondev.hpp>
#include "generated/large.h"
#include "generated/config.h"
static_assert(sizeof(int)==3&&sizeof(long)==4,"AgonDev ADL ABI");
extern "C" {
volatile uint8_t cpp_reply_ready,cpp_reply[10];
void cpp_packet_callback(void);
int review_visual(void);
}
namespace {
uint8_t observed[128];unsigned used;
uint16_t token,handle;
FILE *logfile;
void note(const char *text){if(logfile){fputs(text,logfile);fputc('\n',logfile);fflush(logfile);}}
void send(const void *p,unsigned n){mos_puts(static_cast<const char*>(p),n,0);}
void put16(uint8_t *p,uint16_t v){p[0]=uint8_t(v);p[1]=uint8_t(v>>8);}
void put32(uint8_t *p,uint32_t v){put16(p,uint16_t(v));put16(p+2,uint16_t(v>>16));}
uint16_t get16(const volatile uint8_t *p){return uint16_t(p[0])|(uint16_t(p[1])<<8);}
void append(uint8_t value){if(used<sizeof observed)observed[used++]=value;}
uint32_t ticks(){uint32_t a,b;do{a=sys_vars->time;b=sys_vars->time;}while(a!=b);return a;}
bool await(volatile uint8_t *p,uint8_t mask,uint8_t value){
    uint32_t start=ticks();unsigned long spins=0;
    while((*p&mask)!=value){if(ticks()-start>1200u||++spins>3500000UL)return false;}
    return true;
}
void prefix(uint8_t *cmd,uint16_t id,uint8_t operation){cmd[0]=23;cmd[1]=0;cmd[2]=160;put16(cmd+3,id);cmd[5]=operation;}
bool nibble(uint16_t id,uint16_t offset,bool high,uint8_t &value){
    uint8_t init[10],copy[15],mask[10],tag[10];
    prefix(init,REVIEW_QUERY,5);init[6]=2;put16(init+7,3);init[9]=0x55;
    prefix(copy,REVIEW_QUERY,5);copy[6]=0xe2;put16(copy+7,3);put16(copy+9,1);put16(copy+11,id);put16(copy+13,offset);
    prefix(mask,REVIEW_QUERY,5);mask[6]=5;put16(mask+7,3);mask[9]=high?240:15;
    prefix(tag,REVIEW_QUERY,5);tag[6]=6;put16(tag+7,3);tag[9]=high?8:16;
    volatile uint8_t *gp=reinterpret_cast<volatile uint8_t*>(sys_vars)+sysvar_gp;
    *gp=255;send(init,sizeof init);send(copy,sizeof copy);send(mask,sizeof mask);send(tag,sizeof tag);golem_host_call(REVIEW_QUERY);
    uint32_t start=ticks();unsigned long spins=0;
    while(*gp==255){if(ticks()-start>1200u||++spins>3500000UL)return false;}
    value=*gp;
    if(high){if((value&15)!=8)return false;value&=240;}
    else{if((value&240)!=16)return false;value&=15;}
    return true;
}
bool observe(){
    const uint16_t offsets[]={GOLEM_FIELD_total_OFFSET,GOLEM_FIELD_total_OFFSET+1,12,13,14,15,20,21,22,23};
    for(uint16_t offset:offsets){uint8_t lo,hi;if(!nibble(GOLEM_FIELD_total_BUFFER,offset,false,lo)||!nibble(GOLEM_FIELD_total_BUFFER,offset,true,hi))return false;append(lo|hi);}
    return true;
}
bool request(uint8_t operation,const uint8_t *fields,uint16_t bytes,uint8_t kind,uint16_t &value){
    uint8_t packet[40]={23,0,216,0,0,1,operation,0,0,1,0};
    put16(packet+3,bytes+6);put16(packet+7,++token);memcpy(packet+11,fields,bytes);
    cpp_reply_ready=0;send(packet,bytes+11);
    uint32_t start=ticks();unsigned long spins=0;
    for(;;){
        if(cpp_reply_ready){
            uint8_t reply[10];for(unsigned i=0;i<10;++i)reply[i]=cpp_reply[i];cpp_reply_ready=0;
            if(reply[4]!=1||get16(reply+6)!=token)continue;
            if(reply[5]!=kind)return false;value=get16(reply+8);return true;
        }
        if(ticks()-start>1200u||++spins>3500000UL)return false;
    }
}
bool info(uint16_t field,uint16_t &value){uint8_t p[2];put16(p,field);return request(4,p,2,4,value);}
bool native_load(){
    uint16_t idle=0;if(!info(2,idle))return false;append(uint8_t(idle));append(uint8_t(idle>>8));if(idle)return false;
    uint8_t p[8];put16(p,GOLEM_AGNB_native_MODULE);put32(p+2,REVIEW_MODULE_BYTES);put16(p+6,0);
    if(!request(1,p,sizeof p,2,handle)||!handle)return false;
    golem_agnb_native_bind(handle);return true;
}
bool native_result(){uint16_t result;if(!info(3,result))return false;append(uint8_t(result));append(uint8_t(result>>8));return result==0;}
bool unload(){uint8_t p[2];put16(p,handle);uint16_t result;if(!request(3,p,2,3,result))return false;handle=0;return true;}
bool cycle(){
    note("LOAD");int loaded=REVIEW_NATIVE?golem_agnb_native_load():golem_agnb_buffered_load();append(uint8_t(loaded));
    if(loaded||!observe())return false;
    if(REVIEW_NATIVE){if(!native_load())return false;golem_agnb_native_setup();}
    else golem_agnb_buffered_setup();
    for(unsigned i=0;i<2;++i){
        note("CALL");
        if(REVIEW_NATIVE){golem_native_tick();if(!native_result())return false;}
        else golem_buffered_tick();
        if(!observe())return false;
    }
    note("CLEANUP");
    if(REVIEW_NATIVE){if(!unload())return false;golem_agnb_native_cleanup();}
    else golem_agnb_buffered_cleanup();
    return observe();
}
bool pixel_fence(){
    sys_vars->vdp_pflags&=uint8_t(~vdp_pflag_point);
    const uint8_t query[]={23,0,132,64,1,48,0};send(query,sizeof query);
    return await(&sys_vars->vdp_pflags,vdp_pflag_point,vdp_pflag_point);
}
}
int main(){
    logfile=fopen("cpp.log","w");note("START");mos_setkbvector(cpp_packet_callback,0);
    const uint8_t query[]={23,0,160,uint8_t(REVIEW_QUERY),uint8_t(REVIEW_QUERY>>8),2,
                           23,0,160,uint8_t(REVIEW_QUERY),uint8_t(REVIEW_QUERY>>8),0,4,0,23,0,128,0};
    send(query,sizeof query);
    bool passed=cycle()&&cycle();
    if(passed){
        const uint8_t screen[]={22,0,23,0,192,0,12,17,15,17,128};send(screen,sizeof screen);
        mos_putstring("GOLEM-046: C++ AGNB loader; ");mos_putstring(REVIEW_NATIVE?"native package\r\n":"stock package\r\n");
        mos_putstring("large.golem: nine 60,000-byte arrays; totals 45 then 90.\r\n");
        mos_putstring("review.golem: Golem draws; C++ owns loading and calls.\r\n");
        int loaded=review_visual();append(uint8_t(loaded));passed=loaded==0&&pixel_fence();
        passed=passed&&used==sizeof review_expected&&memcmp(observed,review_expected,used)==0;
    }
    // Additional malformed/missing-file tests use isolated explicit IDs. No setup
    // or source entry is called on these paths; earlier complete records survive.
    if(passed){
        note("REJECTIONS");
        passed=golem_host_load_agnb("absent.agnb")==int(agnb::Error::Open)&&
               golem_host_load_agnb("bad.agnb")==int(agnb::Error::Metadata)&&
               golem_host_load_agnb("truncated.agnb")==int(agnb::Error::Bounds);
        uint8_t lo,hi;passed=passed&&nibble(REVIEW_ERROR_BUFFER,0,false,lo)&&nibble(REVIEW_ERROR_BUFFER,0,true,hi)&&(lo|hi)==0x55;
        passed=passed&&golem_host_load_agnb("late.agnb")==int(agnb::Error::Metadata)&&
               nibble(REVIEW_ERROR_BUFFER,0,false,lo)&&nibble(REVIEW_ERROR_BUFFER,0,true,hi)&&(lo|hi)==17;
        golem_host_clear(REVIEW_ERROR_BUFFER);
        passed=passed&&golem_host_load_agnb("probe.agnb")==0&&
               nibble(REVIEW_ERROR_BUFFER,6,false,lo)&&nibble(REVIEW_ERROR_BUFFER,6,true,hi)&&(lo|hi)==17;
        golem_host_clear(REVIEW_ERROR_BUFFER);
    }
    mos_setkbvector(nullptr,0);
    FILE *file=fopen(passed?"results.dat":"failure.dat","wb");
    if(file){if(fwrite(observed,1,used,file)!=used)passed=false;if(fclose(file)!=0)passed=false;}
    else passed=false;
    note(passed?"PASS":"FAIL");if(logfile)fclose(logfile);
    const uint8_t cursor[]={31,0,40};send(cursor,sizeof cursor);
    if(passed)mos_putstring("C++ streamed-package checks passed.\r\nType dir at the MOS prompt and report to the agent that it works.\r\n");
    else mos_putstring("C++ AGNB test failed. Preserve this run for the agent.\r\n");
    return passed?0:1;
}
