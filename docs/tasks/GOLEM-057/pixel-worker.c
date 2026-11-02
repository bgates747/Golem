/* Qualification worker: finite pixel copies and optional copied-sink operations.
 * Arguments: output:u16,source:u16,x:s16,y:s16,width:u16,height:u16,
 * stride:u16,format:u16,value:u32. All buffers are host-installed beforehand. */
#include "vdplet.h"
_Static_assert(sizeof(vdplet_host_v1_3)==72, "Xtensa media ABI");
_Static_assert(sizeof(vdplet_host_v1_5)==80, "Xtensa sink ABI");
_Static_assert(sizeof(vdplet_pcm_sink_v1)==56, "Xtensa sink request ABI");

void *memcpy(void *destination,const void *source,unsigned count) {
    volatile uint8_t *d=destination;const volatile uint8_t *s=source;
    while(count--)*d++=*s++;return destination;
}
void *memset(void *destination,int value,unsigned count) {
    volatile uint8_t *d=destination;while(count--)*d++=(uint8_t)value;return destination;
}
static uint32_t generation;
static uint16_t u16(const uint8_t *p){return (uint16_t)p[0]|((uint16_t)p[1]<<8);}
static uint32_t u32(const uint8_t *p){return (uint32_t)u16(p)|((uint32_t)u16(p+2)<<16);}
static void put32(uint8_t *p,uint32_t n){unsigned i;for(i=0;i<4;++i)p[i]=(uint8_t)(n>>(8*i));}

__attribute__((visibility("default")))
int32_t vdplet_entry(const vdplet_host_v1 *host,const vdplet_request_v1 *request) {
    const vdplet_host_v1_3 *pixels=vdplet_pixels_host(host);
    const vdplet_host_v1_5 *sink=vdplet_pcm_sink_host(host);
    vdplet_pcm_sink_v1 io={sizeof(io),0,31,0,0,0,0,0,0,0,0,0,0,0};
    vdplet_surface_v1 surface={sizeof(surface),0,0,0,0,0,0};
    vdplet_view_v1 args={sizeof(args),0,0,0,0},source={sizeof(source),0,0,0,0},out={sizeof(out),0,0,0,0};
    uint8_t a[20]={0},result[128]={0};uint16_t output,source_id,width,height,stride,format;
    int16_t x,y;uint32_t value;int32_t status=0,rc;unsigned operation;
    if(request->operation==VDPL_INIT){generation=0;return pixels?VDPL_OK:VDPL_E_ABI;}
    if(request->operation==VDPL_FINI){
        if(sink&&generation){io.generation=generation;sink->pcm_sink(host->context,VDPL_SINK_CLOSE,&io);}
        generation=0;return VDPL_OK;
    }
    if(request->operation!=VDPL_RUN||request->argument_bytes!=sizeof(a))return VDPL_E_ARGUMENT;
    if(request->entry_id==99)return VDPL_E_LIMIT;
    rc=host->buffer_acquire(host->context,request->argument_buffer,VDPL_READ,0,sizeof(a),&args);
    if(rc)return rc;memcpy(a,args.data,sizeof(a));rc=host->buffer_release(host->context,args.lease);if(rc)return rc;
    output=u16(a);source_id=u16(a+2);x=(int16_t)u16(a+4);y=(int16_t)u16(a+6);
    width=u16(a+8);height=u16(a+10);stride=u16(a+12);format=u16(a+14);value=u32(a+16);
    if(request->entry_id==0){
        status=pixels?pixels->surface_info(host->context,&surface):VDPL_E_UNSUPPORTED;
    }else if(request->entry_id==1){
        status=host->buffer_acquire(host->context,source_id,VDPL_READ,0,VDPL_WHOLE_BUFFER,&source);
        if(!status){
            vdplet_pixels_v1 write={sizeof(write),format,x,y,width,height,stride,source.bytes,source.data};
            if(value&&value<write.source_bytes)write.source_bytes=value;
            status=pixels?pixels->pixels_write(host->context,&write):VDPL_E_UNSUPPORTED;
            rc=host->buffer_release(host->context,source.lease);if(rc)return rc;
        }
    }else if(request->entry_id>=10&&request->entry_id<=18){
        operation=request->entry_id==10?VDPL_SINK_DISCOVER:
            request->entry_id==11?VDPL_SINK_OPEN:request->entry_id==12?VDPL_SINK_WRITE:
            request->entry_id==13?VDPL_SINK_STATE:request->entry_id==14?VDPL_SINK_RESET:
            request->entry_id==15?VDPL_SINK_CLOSE:request->entry_id==16?VDPL_SINK_MUTE:
            request->entry_id==17?VDPL_SINK_UNMUTE:VDPL_SINK_STOP;
        io.generation=generation;
        if(operation==VDPL_SINK_OPEN){io.sample_rate=value;io.capacity_frames=width;io.start_frames=height;}
        status=sink?VDPL_OK:VDPL_E_UNSUPPORTED;
        if(!status&&operation==VDPL_SINK_WRITE){
            status=host->buffer_acquire(host->context,source_id,VDPL_READ,0,VDPL_WHOLE_BUFFER,&source);
            if(!status){io.samples=source.data;io.frames=value;if(value>source.bytes)status=VDPL_E_BOUNDS;}
        }
        if(!status)status=sink->pcm_sink(host->context,operation,&io);
        if(source.lease){rc=host->buffer_release(host->context,source.lease);if(rc)return rc;}
        if(!status){generation=operation==VDPL_SINK_CLOSE?0:io.generation;}
    }else status=VDPL_E_UNSUPPORTED;
    put32(result,(uint32_t)status);put32(result+4,host->struct_bytes);
    put32(result+8,host->abi_minor);put32(result+12,host->capabilities);
    put32(result+16,pixels!=0);put32(result+20,vdplet_pcm_host(host)!=0);
    put32(result+24,vdplet_media_host(host)!=0);put32(result+28,sink!=0);
    memcpy(result+32,&surface,sizeof(surface));memcpy(result+64,&io,sizeof(io));
    rc=host->buffer_acquire(host->context,output,VDPL_WRITE,0,sizeof(result),&out);if(rc)return rc;
    memcpy(out.data,result,sizeof(result));return host->buffer_release(host->context,out.lease);
}
