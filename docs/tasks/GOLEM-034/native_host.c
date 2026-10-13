/* Generated native C execution on host only; actual Xtensa qualified separately. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vdplet.h"
#include GENERATED_SOURCE
static uint8_t state[65535],arguments[2]={111,0},retained[4][32];
static uint32_t active[4],serial,peak;static unsigned mode;
static int32_t acquire(void*ctx,uint16_t id,uint16_t access,uint32_t offset,uint32_t bytes,vdplet_view_v1*out) {
    (void)ctx;
    if(access!=VDPL_READ&&access!=VDPL_WRITE)return VDPL_E_ARGUMENT;
    uint8_t *data=0;uint32_t size=0;
    if(id==111){data=state;size=STATE_BYTES-(mode==3?2:0);}
    else if(id==222){data=arguments;size=2;if(access!=VDPL_READ)return VDPL_E_ARGUMENT;}
    else if(id>=20000&&id<=20003) {
        if((mode==1&&id==20000))return VDPL_E_BUFFER;
        if(mode==4&&id==20002)return VDPL_E_BUSY;
        static const unsigned sizes[4]={18,8,22,4};size=sizes[id-20000];data=retained[id-20000];
        if(mode==2&&id==20000)--size;
    } else return VDPL_E_BUFFER;
    if(offset>size)return VDPL_E_BOUNDS;
    if(bytes==VDPL_WHOLE_BUFFER)bytes=size-offset;
    if(bytes>size-offset)return VDPL_E_BOUNDS;
    unsigned count=0,slot=4;
    for(unsigned i=0;i<4;++i){if(active[i])++count;else slot=i;}
    if(slot==4)return VDPL_E_LIMIT;
    active[slot]=++serial;if(count+1>peak)peak=count+1;
    *out=(vdplet_view_v1){sizeof(*out),serial,data+offset,bytes,access};return 0;
}
static int32_t release(void*ctx,uint32_t lease) {
    (void)ctx;
    for(unsigned i=0;i<4;++i)if(active[i]==lease){active[i]=0;return 0;}
    return VDPL_E_HANDLE;
}
int main(void) {
    vdplet_host_v1 host={sizeof(host),1,0,1,0,acquire,release,0,0,0};
    vdplet_request_v1 request={sizeof(request),VDPL_INIT,0,65535,0,0};
    if(vdplet_entry(&host,&request))return 1;
    uint8_t header[4];
    while(fread(header,1,4,stdin)==4) {
        if(fread(state,1,STATE_BYTES,stdin)!=STATE_BYTES)return 2;
        for(unsigned i=0;i<4;++i)if(active[i])return 4;
        mode=header[2]|(unsigned)header[3]<<8;
        request=(vdplet_request_v1){sizeof(request),VDPL_RUN,(uint16_t)(header[0]|(uint16_t)header[1]<<8),222,0,mode==5?1:2};
        int32_t status=vdplet_entry(&host,&request);
        for(unsigned i=0;i<4;++i)if(active[i])return 5;
        if(fwrite(&status,1,4,stdout)!=4||fwrite(state,1,STATE_BYTES,stdout)!=STATE_BYTES)return 3;
    }
    if(peak>3)return 6;
    request.operation=VDPL_FINI;return vdplet_entry(&host,&request)!=0;
}
