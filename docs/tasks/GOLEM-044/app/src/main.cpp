// Ordinary MOS application. C++ owns input/transport; flower.golem owns the curve.
#include <agon/mos.h>
#include <agon/vdp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "parameters.hpp"
#include "generated/flower_package.h"

static_assert(sizeof(int)==3 && sizeof(void*)==3,"AgonDev ADL ABI");
static_assert(sizeof(VDU_ADV_CMD)==6,"VDP call wire size");
extern "C" {
volatile uint8_t flower_key_write,flower_key_read,flower_keys[256],flower_key_overflow;
volatile uint8_t flower_reply_ready,flower_reply[10],flower_escape;
void flower_packet_callback(void);
void golem_host_call(uint16_t entry){vdp_adv_call_buffer(entry);}
}

namespace {
flower::Parameters params;
FILE *logfile;
uint16_t token,module_handle,last_error;
uint8_t saved_mode,pane_top,columns;
bool mode_changed,common_installed,native_installed,renderer_installed,staging_installed;
bool uncertain,math_error,held[256];
uint8_t suppressed;
uint32_t segments,curves;

void journal(const char *format,...){
    if(!logfile)return;
    va_list args;va_start(args,format);vfprintf(logfile,format,args);va_end(args);
    fputc('\n',logfile);fflush(logfile);
}
void send(const void *data,unsigned size){mos_puts(static_cast<const char*>(data),size,0);}
void bytes(const uint8_t *data,unsigned size){send(data,size);}
void text(const char *s){mos_putstring(s);}
void cursor(unsigned col,unsigned row){const uint8_t cmd[]={31,uint8_t(col),uint8_t(row)};bytes(cmd,sizeof cmd);}
void row(unsigned at,const char *s){
    cursor(0,pane_top+at);for(unsigned i=1;i<columns;++i)putch(' ');
    cursor(0,pane_top+at);unsigned n=strlen(s);if(n>=columns)n=columns-1;send(s,n);
}
void heading(){row(0,"FLOWER - Golem flower.golem; C++ input/calls; VDP draws");}
uint32_t ticks(){uint32_t a,b;do{a=sys_vars->time;b=sys_vars->time;}while(a!=b);return a;}
bool deadline(uint32_t start,unsigned long spins){return ticks()-start>1200u || spins>3500000UL;}
bool wait_byte(volatile uint8_t *p,uint8_t mask,uint8_t value){
    uint32_t start=ticks();unsigned long spins=0;
    while((*p&mask)!=value){if(deadline(start,++spins))return false;}
    return true;
}
bool echo(){
    volatile uint8_t *gp=reinterpret_cast<volatile uint8_t*>(sys_vars)+sysvar_gp;
    static uint8_t tag=0x52;tag^=0xff;*gp=uint8_t(~tag);
    const uint8_t cmd[]={23,0,128,tag};bytes(cmd,sizeof cmd);
    return wait_byte(gp,255,tag);
}
bool timeout(){
    journal("TIMEOUT awaiting completion; checking ordered recovery barrier");
    // A later echo proves the pending finite command returned. Without it, keep
    // its module/storage attached and require reset; never free in-flight code.
    if(echo()){last_error=255;journal("RECOVERED ordered barrier");return false;}
    uncertain=true;journal("RESET_REQUIRED VDP did not complete; resources retained");
    return false;
}
bool request(uint8_t op,const uint8_t *fields,unsigned size,uint8_t kind,uint16_t &value){
    uint8_t cmd[37]={23,0,216,0,0,1,op,0,0,1,0};
    ++token;golem_put16(cmd+3,uint16_t(size+6));golem_put16(cmd+7,token);
    memcpy(cmd+11,fields,size);flower_reply_ready=0;bytes(cmd,size+11);
    uint32_t start=ticks();unsigned long spins=0;
    for(;;){
        if(flower_reply_ready){
            uint8_t reply[10];for(unsigned i=0;i<10;++i)reply[i]=flower_reply[i];flower_reply_ready=0;
            if(reply[4]!=1||uint16_t(reply[6]|uint16_t(reply[7])<<8)!=token)continue;
            value=uint16_t(reply[8]|uint16_t(reply[9])<<8);
            if(reply[5]==5){last_error=value;journal("RUNTIME_ERROR op=%u code=%u",unsigned(op),unsigned(value));return false;}
            if(reply[5]!=kind){last_error=254;return false;}
            return true;
        }
        if(deadline(start,++spins))return timeout();
    }
}
bool info(uint16_t field,uint16_t &value){uint8_t arg[2];golem_put16(arg,field);return request(4,arg,2,4,value);}
bool read16(uint16_t offset,uint16_t &value){
    uint8_t arg[6];golem_put16(arg,GOLEM_STATE_BUFFER);golem_put32(arg+2,offset);
    return request(5,arg,6,4,value);
}
void prefix(uint8_t *cmd,uint16_t id,uint8_t op){cmd[0]=23;cmd[1]=0;cmd[2]=160;golem_put16(cmd+3,id);cmd[5]=op;}
void clear_buffer(uint16_t id){uint8_t cmd[6];prefix(cmd,id,2);bytes(cmd,sizeof cmd);}
void update(uint16_t id,uint16_t offset,const uint8_t *data,uint16_t size){
    uint8_t cmd[11];prefix(cmd,id,5);cmd[6]=0xc2;golem_put16(cmd+7,offset);golem_put16(cmd+9,size);
    bytes(cmd,sizeof cmd);bytes(data,size);
}
bool plot_complete(){
    sys_vars->vdp_pflags&=uint8_t(~vdp_pflag_point);
    const uint8_t cmd[]={23,0,132,0,0,0,0};bytes(cmd,sizeof cmd);
    if(wait_byte(&sys_vars->vdp_pflags,vdp_pflag_point,vdp_pflag_point))return true;
    return timeout();
}
bool query_mode(){
    sys_vars->vdp_pflags&=uint8_t(~vdp_pflag_mode);const uint8_t cmd[]={23,0,134};bytes(cmd,sizeof cmd);
    if(wait_byte(&sys_vars->vdp_pflags,vdp_pflag_mode,vdp_pflag_mode))return true;
    return timeout();
}
bool set_mode(uint8_t mode){
    sys_vars->vdp_pflags&=uint8_t(~vdp_pflag_mode);const uint8_t cmd[]={22,mode};bytes(cmd,sizeof cmd);
    if(!wait_byte(&sys_vars->vdp_pflags,vdp_pflag_mode,vdp_pflag_mode))return timeout();
    return sys_vars->scrMode==mode;
}
bool discover(){
    flower_reply_ready=0;const uint8_t cmd[]={23,0,217};bytes(cmd,sizeof cmd);
    if(!echo())return timeout();
    if(!flower_reply_ready||flower_reply[4]!=1||flower_reply[5]!=1||!(flower_reply[8]&1))return false;
    uint16_t version,handle;
    if(!info(0,version)||version!=0x0100||!info(2,handle))return false;
    if(handle){last_error=7;return false;}
    return true;
}
bool install(){
    staging_installed=true;clear_buffer(FLOWER_STAGING);
    uint8_t cmd[8];prefix(cmd,FLOWER_STAGING,0);golem_put16(cmd+6,sizeof flower_module);
    bytes(cmd,sizeof cmd);bytes(flower_module,sizeof flower_module);
    if(flower_escape)return false;
    uint8_t args[8];golem_put16(args,FLOWER_STAGING);golem_put32(args+2,sizeof flower_module);golem_put16(args+6,0);
    if(!request(1,args,sizeof args,2,module_handle)||!module_handle){
        if(last_error==255){uncertain=true;journal("RESET_REQUIRED module load result unconfirmed");}
        return false;
    }
    common_installed=true;bytes(flower_common_install,sizeof flower_common_install);
    native_installed=true;bytes(flower_native_install,sizeof flower_native_install);
    uint8_t handle[2];golem_put16(handle,module_handle);
    for(unsigned i=0;i<sizeof flower_wrappers/sizeof flower_wrappers[0];++i)
        update(flower_wrappers[i],GOLEM_NATIVE_HANDLE_OFFSET,handle,2);
    renderer_installed=true;bytes(flower_renderer_install,sizeof flower_renderer_install);
    return echo();
}
struct Key {uint8_t ascii,vkey;bool down,repeat;};
bool key(Key &out){
    if(flower_key_read==flower_key_write)return false;
    unsigned at=unsigned(flower_key_read)*4;
    out.ascii=flower_keys[at];out.vkey=flower_keys[at+2];out.down=flower_keys[at+3]!=0;
    flower_key_read=uint8_t((flower_key_read+1)&63);
    out.repeat=held[out.vkey];held[out.vkey]=out.down;
    if(out.vkey==suppressed&&suppressed){if(!out.down)suppressed=0;return false;}
    return true;
}
void drain(){Key event;while(key(event)){};}
bool edit(const char *message){
    for(unsigned field=0;field<flower::Fields&&!flower_escape;++field){
        flower::Editor editor;editor.reset();char label[79];
        heading();snprintf(label,sizeof label,"%u/7 %s [%s]",field+1,flower::names[field],params.text[field]);row(1,label);
        row(2,"New value: ");row(3,message?message:"Enter keeps the current value. Esc exits.");message=0;
        cursor(11,pane_top+2);journal("PROMPT %u %s",field,params.text[field]);
        while(!flower_escape){
            Key event;if(!key(event)||!event.down)continue;
            if(event.ascii==13){
                if(flower::accept(params,field,editor.replacement)){
                    journal("ACCEPT %u %s %08lx",field,params.text[field],(unsigned long)flower::bits(params.value[field]));break;
                }
                row(3,"Invalid number or field range. Correct the replacement; Esc exits.");
                journal("REJECT %u %s",field,editor.replacement);
            }else if(editor.key(event.ascii)){
                char line[48];snprintf(line,sizeof line,"New value: %s",editor.replacement);row(2,line);
                row(3,"Enter accepts; blank Enter keeps the current value. Esc exits.");
                journal("EDIT %u %s",field,editor.replacement);
            }else if(event.ascii>=32&&event.ascii<=126){row(3,"Maximum 31 characters. Backspace to correct; Esc exits.");journal("LIMIT %u",field);}
            cursor(11+editor.size,pane_top+2);
        }
    }
    return !flower_escape;
}
bool invoke(uint16_t entry){
    math_error=false;
    golem_host_call(entry);uint16_t status=0;
    if(!info(3,status))return false;
    if(status){
        last_error=status;uint16_t language=0,interface=0;
        read16(GOLEM_ROOT_LANGUAGE_STATUS_OFFSET,language);read16(GOLEM_ROOT_INTERFACE_STATUS_OFFSET,interface);
        math_error=language!=0||interface!=0;
        journal("MATH_ERROR runtime=%u interface=%u language=%u",unsigned(status),unsigned(interface),unsigned(language));
        return false;
    }
    return true;
}
bool prepare(){
    uint8_t inputs[FLOWER_INPUT_BYTES];
    for(unsigned i=0;i<flower::Fields;++i)golem_put32(inputs+i*4,flower::bits(params.value[i]));
    update(GOLEM_STATE_BUFFER,FLOWER_INPUT_OFFSET,inputs,sizeof inputs);
    journal("PREPARE");return invoke(GOLEM_prepare_NATIVE);
}
bool draw(){
    uint16_t remaining=0;if(!read16(FLOWER_REMAINING_OFFSET,remaining))return false;
    const uint8_t setup[]={26,23,0,192,1,29,128,2,0,2,12};bytes(setup,sizeof setup);
    segments=0;journal("DRAW %lu",(unsigned long)(curves+1));
    while(remaining&&!flower_escape){
        const uint16_t before=remaining;
        drain();if(flower_escape)break;
        if(!invoke(GOLEM_batch_NATIVE))return false;
        if(flower_escape)break;
        golem_host_call(FLOWER_RENDER);
        if(!read16(FLOWER_REMAINING_OFFSET,remaining)||!plot_complete())return false;
        const uint16_t count=before-remaining;
        segments+=count;journal("BATCH %lu remaining=%u count=%u",(unsigned long)segments,unsigned(remaining),unsigned(count));
    }
    drain();
    if(flower_escape)return true;
    ++curves;heading();row(1,"Curve complete. Press any key to edit the retained values.");
    row(2,"Angles use decimal degrees, from 0 through 360.");row(3,"Esc exits and restores the previous screen mode.");
    journal("DONE %lu segments=%lu",(unsigned long)curves,(unsigned long)segments);return true;
}
void wait_key(){
    while(!flower_escape){Key event;if(!key(event)||!event.down||event.repeat)continue;
        suppressed=event.vkey;journal("REENTER key=%u",unsigned(event.ascii));return;}
}
bool cleanup(){
    if(uncertain)return false;
    if(module_handle){uint8_t arg[2];golem_put16(arg,module_handle);uint16_t value;
        if(!request(3,arg,2,3,value)){uncertain=true;journal("RESET_REQUIRED module unload result unconfirmed");return false;}module_handle=0;}
    if(renderer_installed)bytes(flower_renderer_cleanup,sizeof flower_renderer_cleanup);
    if(native_installed)bytes(flower_native_cleanup,sizeof flower_native_cleanup);
    if(common_installed)bytes(flower_common_cleanup,sizeof flower_common_cleanup);
    else if(staging_installed)clear_buffer(FLOWER_STAGING);
    return echo();
}
}

int main(){
    logfile=fopen("flower.log","w");journal("START");params.defaults();
    mos_setkbvector(flower_packet_callback,0);
    const char *startup_error=0;
    if(!query_mode())startup_error="Display did not reply.";
    else{
        saved_mode=sys_vars->scrMode;journal("SAVED_MODE %u %u %u",unsigned(saved_mode),unsigned(sys_vars->scrWidth),unsigned(sys_vars->scrHeight));
        if(!discover())startup_error=last_error==7?"The VDPlet module slot is busy.":"Flower needs a compatible VDPlet-enabled VDP.";
        else if(!install())startup_error="Could not install Flower's module.";
        else{
            mode_changed=true;
            if(!set_mode(19))startup_error="Could not set mode 19.";
            if(!startup_error&&(sys_vars->scrWidth!=1024||sys_vars->scrHeight!=768))startup_error="Mode 19 needs a 1024x768 display.";
        }
    }
    if(!startup_error&&!flower_escape){
        pane_top=sys_vars->scrRows-4;columns=sys_vars->scrCols;
        const uint8_t colours[]={23,1,0,19,0,255,0,0,0,19,1,255,255,255,255,17,1,17,128,18,0,1};bytes(colours,sizeof colours);
        journal("READY mode=%u",unsigned(sys_vars->scrMode));
        const char *message=0;
        while(!flower_escape&&!uncertain){
            if(!edit(message))break;
            if(!prepare()){if(uncertain)break;message=math_error?"These values exceed the calculation's range. Correct them; Esc exits.":"VDP call failed; earlier picture kept. Esc exits.";continue;}
            if(flower_escape)break;
            if(!draw()){if(uncertain)break;message="Calculation stopped. Earlier drawing is kept. Correct values; Esc exits.";continue;}
            if(flower_escape)break;
            wait_key();message=0;
        }
    }
    const bool cleaned=cleanup();
    if(uncertain){
        // Trusted module did not return: preserving storage is safer than freeing
        // executing code. Recovery is a machine reset, not a fictitious MOS return.
        for(;;){} // Interrupts remain enabled. See the documented timeout boundary.
    }
    bool restored=true;if(mode_changed)restored=set_mode(saved_mode);
    if(uncertain){for(;;){}} // Display/transport recovery requires a reset.
    const uint8_t on[]={23,1,1};bytes(on,sizeof on);
    mos_setkbvector(0,0);sys_vars->keyascii=0;
    journal("EXIT saved=%u restored=%u cleanup=%u overflow=%u curves=%lu",unsigned(saved_mode),unsigned(sys_vars->scrMode),unsigned(cleaned&&restored),unsigned(flower_key_overflow),(unsigned long)curves);
    if(logfile)fclose(logfile);
    if(flower_escape)startup_error=0;
    if(startup_error){text("\r\nFlower: ");text(startup_error);text("\r\n");}
    else text("\r\nFlower finished. Type dir at the MOS prompt to check keyboard response.\r\n");
    return cleaned&&restored?0:1;
}
