/* Strict SDK mock: real bounded lease slots and retained application arrays. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vdplet.h"
#include "fixture.h"
#include GENERATED_SOURCE
static uint8_t state[65536],arguments[2]={STATE_ID&255,STATE_ID>>8};
static uint32_t active[4],serial,peak;static unsigned mode;
static const unsigned array_count=ARRAY_COUNT;
static uint8_t *arrays[ARRAY_COUNT+1];
static int32_t acquire(void*ctx,uint16_t id,uint16_t access,uint32_t offset,uint32_t bytes,vdplet_view_v1*out) {
    (void)ctx;if(access!=VDPL_READ&&access!=VDPL_WRITE)return VDPL_E_ARGUMENT;
    uint8_t *data=0;uint32_t size=0;
    if(id==STATE_ID){data=state;size=STATE_BYTES-(mode==3?2:0);}
    else if(id==ARGS_ID){data=arguments;size=2;if(access!=VDPL_READ)return VDPL_E_ARGUMENT;}
    else for(unsigned i=0;i<array_count;++i)if(id==array_ids[i]) {
        if(mode==1&&i==array_count-1)return VDPL_E_BUFFER;
        if(mode==4&&i==0)return VDPL_E_BUSY;
        if(access==VDPL_WRITE&&array_readonly[i])return VDPL_E_ARGUMENT;
        data=arrays[i];size=array_bytes[i];
        if(mode==2&&i==array_count-1)--size;
        if(mode==6&&i==array_count-1)++size;
    }
    if(!data)return VDPL_E_BUFFER;
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
    (void)ctx;for(unsigned i=0;i<4;++i)if(lease&&active[i]==lease){active[i]=0;return 0;}
    return VDPL_E_HANDLE;
}
int main(void) {
    for(unsigned i=0;i<array_count;++i){arrays[i]=calloc(array_bytes[i]+2,1);if(!arrays[i])return 7;}
    vdplet_host_v1 host={sizeof(host),1,0,1,0,acquire,release,0,0,0};
    vdplet_request_v1 request={sizeof(request),VDPL_INIT,0,65535,0,0};
    if(vdplet_entry(&host,&request))return 1;
    uint8_t header[4];
    while(fread(header,1,4,stdin)==4) {
        if(fread(state,1,STATE_BYTES,stdin)!=STATE_BYTES)return 2;
        for(unsigned i=0;i<array_count;++i)if(fread(arrays[i],1,array_bytes[i],stdin)!=array_bytes[i])return 2;
        for(unsigned i=0;i<4;++i)if(active[i])return 4;
        mode=header[2]|(unsigned)header[3]<<8;
        request=(vdplet_request_v1){sizeof(request),VDPL_RUN,(uint16_t)(header[0]|(uint16_t)header[1]<<8),ARGS_ID,0,mode==5?1:2};
        int32_t status=vdplet_entry(&host,&request);
        for(unsigned i=0;i<4;++i)if(active[i])return 5;
        if(fwrite(&status,1,4,stdout)!=4||fwrite(state,1,STATE_BYTES,stdout)!=STATE_BYTES)return 3;
        for(unsigned i=0;i<array_count;++i)if(fwrite(arrays[i],1,array_bytes[i],stdout)!=array_bytes[i])return 3;
    }
    if(peak>3)return 6;
    request.operation=VDPL_FINI;int rc=vdplet_entry(&host,&request);
    for(unsigned i=0;i<array_count;++i)free(arrays[i]);
    return rc!=0;
}
