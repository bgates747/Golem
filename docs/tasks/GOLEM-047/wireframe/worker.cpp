// Fixture adapter: unchanged owner's transform plus a separate native draw entry.
#define vdplet_entry wire_transform
#include "build/transform-source.cpp"
#undef vdplet_entry
#include "build/draw-layout.h"

namespace drawing {
int32_t run(const vdplet_host_v1 *h,const vdplet_request_v1 *r) {
    const auto *api=vdplet_vdu_host(h);
    if(!api||r->argument_bytes!=2)return VDPL_E_ABI;
    Leases lease(h);
    int32_t rc=lease.acquire(0,r->argument_buffer,VDPL_READ,2);
    if(rc)return lease.finish(rc);
    if(u16(lease.views[0].data)!=STATE_ID)return lease.finish(VDPL_E_ARGUMENT);
    rc=lease.acquire(1,STATE_ID,VDPL_READ,STATE_BYTES);
    if(rc)return lease.finish(rc);
    const auto *state=lease.views[1].data;
    if(u16(state+INTERFACE_STATUS)||u16(state+LANGUAGE_STATUS)||!u32(state+SUCCESS_GENERATION)||
       u16(state+SUCCESS_ENTRY)!=0||u32(state+SUCCESS_SESSION)!=u32(state+SESSION))
        return lease.finish(VDPL_E_ARGUMENT);
    rc=lease.acquire(2,OUTPUT_ID,VDPL_READ,VERTEX_BYTES);
    if(!rc)rc=lease.acquire(3,EDGE_ID,VDPL_READ,EDGE_COUNT*4);
    if(rc)return lease.finish(rc);
    uint8_t bytes[32*12];
    for(unsigned first=0;first<EDGE_COUNT;first+=32) {
        unsigned count=EDGE_COUNT-first;if(count>32)count=32;
        for(unsigned i=0;i<count;++i) {
            const auto *edge=lease.views[3].data+(first+i)*4;
            unsigned a=u16(edge),b=u16(edge+2);
            if(a>=VERTEX_COUNT||b>=VERTEX_COUNT)return lease.finish(VDPL_E_BOUNDS);
            auto *out=bytes+i*12;out[0]=25;out[1]=4;out[6]=25;out[7]=5;
            for(unsigned j=0;j<4;++j) {
                out[2+j]=lease.views[2].data[a*6+j];
                out[8+j]=lease.views[2].data[b*6+j];
            }
        }
        rc=api->vdu_submit(h->context,bytes,count*12);
        if(rc)return lease.finish(rc);
    }
    return lease.finish(VDPL_OK);
}
}
extern "C" __attribute__((visibility("default")))
int32_t vdplet_entry(const vdplet_host_v1 *h,const vdplet_request_v1 *r) {
    if(!h||!r)return VDPL_E_ARGUMENT;
    if(r->operation!=VDPL_RUN||r->entry_id==0)return wire_transform(h,r);
    return r->entry_id==1 ? drawing::run(h,r) : VDPL_E_UNSUPPORTED;
}
