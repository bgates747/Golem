/* Host execution of generated C only; not Xtensa/firmware evidence. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vdplet.h"
#include GENERATED_SOURCE

static uint8_t state[65535], arguments[2]={111,0};
static uint32_t leases;
static int32_t acquire(void *ctx,uint16_t id,uint16_t access,uint32_t offset,uint32_t bytes,vdplet_view_v1*out) {
    (void)ctx;
    uint8_t *data = id==111 ? state : arguments;
    uint32_t size = id==111 ? STATE_BYTES : 2;
    if ((id!=111&&id!=222)||(id==222&&access!=VDPL_READ)) return VDPL_E_ARGUMENT;
    if (offset>size) return VDPL_E_BOUNDS;
    if (bytes==VDPL_WHOLE_BUFFER) bytes=size-offset;
    if (bytes>size-offset||leases==4) return VDPL_E_BOUNDS;
    *out=(vdplet_view_v1){sizeof(*out),++leases,data+offset,bytes,access};
    return 0;
}
static int32_t release(void*ctx,uint32_t lease) {
    (void)ctx;
    return lease&&lease<=leases ? 0 : VDPL_E_HANDLE;
}
int main(void) {
    vdplet_host_v1 host={sizeof(host),1,0,1,0,acquire,release,0,0,0};
    vdplet_request_v1 request={sizeof(request),VDPL_INIT,0,65535,0,0};
    if(vdplet_entry(&host,&request)) return 1;
    uint8_t entry[2];
    while(fread(entry,1,2,stdin)==2) {
        if(fread(state,1,STATE_BYTES,stdin)!=STATE_BYTES) return 2;
        leases=0;
        request=(vdplet_request_v1){sizeof(request),VDPL_RUN,(uint16_t)(entry[0]|(uint16_t)entry[1]<<8),222,0,2};
        int32_t status=vdplet_entry(&host,&request);
        if(fwrite(&status,1,4,stdout)!=4||fwrite(state,1,STATE_BYTES,stdout)!=STATE_BYTES) return 3;
    }
    request.operation=VDPL_FINI;
    return vdplet_entry(&host,&request)!=0;
}
