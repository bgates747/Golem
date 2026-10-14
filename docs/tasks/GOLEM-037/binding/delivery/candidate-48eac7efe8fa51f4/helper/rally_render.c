/* Rally-owned isolated renderer. No SDK dispatcher, UART, scheduler or heap. */
#include "rally_render.h"
#include <stdint.h>
#include "asset_meta.inc"
static uint16_t u16(const uint8_t *p){return p[0]|(uint16_t)p[1]<<8;}
static uint32_t u32(const uint8_t *p){return u16(p)|(uint32_t)u16(p+2)<<16;}
static int s16(const uint8_t *p){return (int16_t)u16(p);}
static int32_t s32(const uint8_t *p){return (int32_t)u32(p);}
static uint32_t hash(const uint8_t *p,unsigned n){uint32_t h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;}
#include "render_core.inc"

static int overlap(uintptr_t a,uint32_t n,uintptr_t b,uint32_t m){
    return n && m && (a<=b?b-a<n:a-b<m);
}
static int objects(const void *a,uint32_t n,const void *b,uint32_t m){
    return overlap((uintptr_t)a,n,(uintptr_t)b,m);
}
static int completion_ok(const RallyRenderCompletionV1 *c){
    return c && c->struct_bytes==sizeof(*c) && c->schema==1;
}
static void clear_completion(RallyRenderCompletionV1 *c){
    uint8_t *p=(uint8_t *)c;for(unsigned i=0;i<sizeof(*c);++i)p[i]=0;
    c->struct_bytes=sizeof(*c);c->schema=1;
}
static int32_t fail(RallyRenderCompletionV1 *c,int reason,int32_t result,int32_t service){
    c->ready=0;c->reason=(uint16_t)reason;c->result=result;c->service_result=service;return result;
}
static int host_ok(const vdplet_host_v1 *h){
    return h && h->struct_bytes>=sizeof(*h) && h->abi_major==1 &&
           (h->capabilities&VDPL_CAP_BUFFERS) && h->buffer_acquire && h->buffer_release;
}
static int range_ok(const RallyRenderRangeV1 *r,unsigned access){
    return r->buffer && r->buffer!=65535 && r->access==access && r->bytes && r->offset<=UINT32_MAX-r->bytes;
}
static int resources_ok(const RallyRenderResourcesV1 *r){
    const unsigned bmp[3]={r->player_bitmap_base,r->traffic_bitmap_base,r->sign_bitmap_base};
    const unsigned counts[3]={5,30,8};
    const unsigned matrix[3]={r->traffic_matrix_base,r->player_mirror_matrix,r->sign_matrix};
    const unsigned mc[3]={6,1,1};
    for(unsigned i=0;i<3;++i){
        if(bmp[i]+counts[i]>256 || matrix[i]+mc[i]>65535)return 0;
        for(unsigned j=0;j<i;++j)
            if(overlap(bmp[i],counts[i],bmp[j],counts[j]) || overlap(matrix[i],mc[i],matrix[j],mc[j]))return 0;
    }
    return 1;
}
static int32_t acquire(const vdplet_host_v1 *h,const RallyRenderRangeV1 *r,vdplet_view_v1 *v){
    v->struct_bytes=sizeof(*v);v->lease=0;v->data=0;v->bytes=0;v->access=0;
    return h->buffer_acquire(h->context,r->buffer,r->access,r->offset,r->bytes,v);
}
static int32_t release(const vdplet_host_v1 *h,vdplet_view_v1 *v){
    if(!v->lease)return 0;
    const uint32_t lease=v->lease;v->lease=0;return h->buffer_release(h->context,lease);
}
static int32_t view_ok(const vdplet_view_v1 *v,const RallyRenderRangeV1 *r){
    return v->lease && v->data && v->bytes==r->bytes && (v->access&r->access)==r->access;
}
static int asset_ok(const vdplet_view_v1 *v,const RallyRenderBindingsV1 *b,int checksum){
    unsigned which=b->track_identity==asset_meta[0][0]?0:1;
    if(b->track_identity!=asset_meta[which][0] || b->asset_checksum!=asset_meta[which][1] ||
       v->bytes!=asset_meta[which][2] || v->bytes<192)return 0;
    const uint8_t *p=v->data;unsigned count=asset_meta[which][3];
    if(u32(p)!=0x54343252u || u16(p+4)!=1 || u16(p+6)!=10 || u32(p+8)!=v->bytes ||
       u32(p+12)!=b->track_identity || u16(p+16)!=count || u16(p+18) || u32(p+20)!=count*64 ||
       u32(p+24)!=b->asset_checksum || u32(p+28))return 0;
    if(checksum && hash(p+32,v->bytes-32)!=b->asset_checksum)return 0;
    static const unsigned widths[10]={12,8,2,1,1,2,1,4,2,2};
    static const unsigned counts[8]={2408,16803,4000,101,1060,129,257,289};
    unsigned end=192;
    for(unsigned i=0;i<10;++i){
        const uint8_t *r=p+32+i*16;unsigned n=i<2?count*(i?1:19):counts[i-2];
        unsigned off=u32(r+8),bytes=n*widths[i];
        if(u16(r)!=i+1 || u16(r+2)!=widths[i] || u32(r+4)!=n || u32(r+12)!=bytes ||
           off%4 || off<end || off>v->bytes || bytes>v->bytes-off)return 0;
        end=off+bytes;
    }
    return 1;
}
/* Caller guarantees retained allocation identities cannot change mid-entry.
   Temporary addresses below are never dereferenced after releasing that view. */
static int32_t preflight(const vdplet_host_v1 *h,const RallyRenderBindingsV1 *b,const vdplet_view_v1 *asset,
                         const RallyRenderContextV1 *ctx,const void *input,unsigned input_bytes,
                         RallyRenderCompletionV1 *c){
    uintptr_t starts[3];unsigned lengths[3];
    for(unsigned i=0;i<3;++i){
        vdplet_view_v1 v;int32_t rc=acquire(h,&b->output[i],&v);
        int reason=RALLY_RENDER_REASON_OK;
        if(rc)reason=RALLY_RENDER_REASON_ACQUIRE;
        else if(!view_ok(&v,&b->output[i])){rc=VDPL_E_BOUNDS;reason=RALLY_RENDER_REASON_CAPACITY;}
        else{
            starts[i]=(uintptr_t)v.data;lengths[i]=v.bytes;
            if(objects(v.data,v.bytes,asset->data,asset->bytes) || objects(v.data,v.bytes,ctx,sizeof(*ctx)) ||
               objects(v.data,v.bytes,input,input_bytes) || objects(v.data,v.bytes,c,sizeof(*c)) ||
               objects(v.data,v.bytes,h,sizeof(*h))){rc=VDPL_E_ARGUMENT;reason=RALLY_RENDER_REASON_ALIAS;}
            for(unsigned j=0;j<i && !rc;++j)
                if(overlap(starts[i],lengths[i],starts[j],lengths[j])){rc=VDPL_E_ARGUMENT;reason=RALLY_RENDER_REASON_ALIAS;}
        }
        int32_t rr=release(h,&v);
        if(rr)return fail(c,RALLY_RENDER_REASON_RELEASE,rr,rr);
        if(rc)return fail(c,reason,rc,reason==RALLY_RENDER_REASON_ACQUIRE?rc:0);
    }
    return 0;
}
static int state_ok(const RallyRenderStateV1 *s,int lap,uint32_t session){
    if(s->struct_bytes!=sizeof(*s) || s->schema!=1 || s->session!=session || s->player_station<0 ||
       s->player_station>=lap || s->player_lateral_q8< -38400 || s->player_lateral_q8>38400 ||
       s->phase>=8000 || s->steering< -21 || s->steering>21 || s->flags>7 || s->presentation_profile>1)return 0;
    if(s->presentation_profile==0 && (s->reference_crash_remaining || s->reference_invulnerable))return 0;
    if(s->reference_crash_remaining>150 || s->reference_invulnerable>120)return 0;
    for(unsigned i=0;i<6;++i){const RallyRenderOpponentV1 *p=&s->opponents[i];
        if(p->active>1)return 0;
        if(!p->active){if(p->station || p->lateral_q8 || p->heading_turn16)return 0;}
        else if(p->station<0 || p->station>=lap || p->lateral_q8< -14260 || p->lateral_q8>14260 ||
                p->heading_turn16< -1920 || p->heading_turn16>1920)return 0;
    }
    return 1;
}
int32_t rally_render_bind_v1(const vdplet_host_v1 *h,RallyRenderContextV1 *ctx,
                           const RallyRenderBindingsV1 *input,RallyRenderCompletionV1 *c){
    if(!completion_ok(c) || !ctx || ctx->struct_bytes!=sizeof(*ctx) || !input ||
       objects(ctx,sizeof(*ctx),c,sizeof(*c)) || objects(input,sizeof(*input),ctx,sizeof(*ctx)) ||
       objects(input,sizeof(*input),c,sizeof(*c)))return VDPL_E_ARGUMENT;
    clear_completion(c);ctx->bound=0;
    if(!host_ok(h))return fail(c,RALLY_RENDER_REASON_ARGUMENT,VDPL_E_ABI,0);
    const RallyRenderBindingsV1 b=*input;c->session=b.session;
    if(b.struct_bytes!=sizeof(b) || b.schema!=1 || !b.session || !range_ok(&b.asset,VDPL_READ) ||
       !resources_ok(&b.resources))return fail(c,RALLY_RENDER_REASON_ARGUMENT,VDPL_E_ARGUMENT,0);
    for(unsigned i=0;i<3;++i)if(!range_ok(&b.output[i],VDPL_WRITE))return fail(c,RALLY_RENDER_REASON_ARGUMENT,VDPL_E_ARGUMENT,0);
    vdplet_view_v1 asset;int32_t rc=acquire(h,&b.asset,&asset);
    if(rc)fail(c,RALLY_RENDER_REASON_ACQUIRE,rc,rc);
    else if(!view_ok(&asset,&b.asset) || !asset_ok(&asset,&b,1))rc=fail(c,RALLY_RENDER_REASON_ASSET,VDPL_E_FORMAT,0);
    else rc=preflight(h,&b,&asset,ctx,input,sizeof(*input),c);
    int32_t rr=release(h,&asset);
    if(rr)rc=fail(c,RALLY_RENDER_REASON_RELEASE,rr,rr);
    if(!rc){ctx->bindings=b;ctx->bound=1;}
    return rc;
}
int32_t rally_render_prepare_v1(const vdplet_host_v1 *h,RallyRenderContextV1 *ctx,
                              const RallyRenderStateV1 *input,RallyRenderCompletionV1 *c){
    if(!completion_ok(c) || !ctx || ctx->struct_bytes!=sizeof(*ctx) || !input ||
       objects(ctx,sizeof(*ctx),c,sizeof(*c)) || objects(input,sizeof(*input),ctx,sizeof(*ctx)) ||
       objects(input,sizeof(*input),c,sizeof(*c)))return VDPL_E_ARGUMENT;
    clear_completion(c);
    if(!host_ok(h))return fail(c,RALLY_RENDER_REASON_ARGUMENT,VDPL_E_ABI,0);
    if(ctx->bound!=1)return fail(c,RALLY_RENDER_REASON_NOT_BOUND,VDPL_E_INIT,0);
    const RallyRenderBindingsV1 *b=&ctx->bindings;ctx->snapshot=*input;
    const RallyRenderStateV1 *state=&ctx->snapshot;
    unsigned which=b->track_identity==asset_meta[0][0]?0:1;
    const int lap=(int)asset_meta[which][3]*6400;
    c->session=state->session;c->request_generation=state->request_generation;c->simulation_tick=state->simulation_tick;
    if(!state_ok(state,lap,b->session))return fail(c,RALLY_RENDER_REASON_STATE,VDPL_E_ARGUMENT,0);
    vdplet_view_v1 asset;int32_t rc=acquire(h,&b->asset,&asset);
    if(rc)fail(c,RALLY_RENDER_REASON_ACQUIRE,rc,rc);
    else if(!view_ok(&asset,&b->asset) || !asset_ok(&asset,b,0))rc=fail(c,RALLY_RENDER_REASON_ASSET,VDPL_E_FORMAT,0);
    else rc=preflight(h,b,&asset,ctx,input,sizeof(*input),c);
    if(!rc){
        Assets a;a.lap=lap;a.count=(int)asset_meta[which][3];a.bin_offset=which?0:360;
        a.state=state;a.resources=&b->resources;a.context=ctx;
        for(unsigned i=0;i<10;++i)a.t[i]=asset.data+u32(asset.data+40+i*16);
        int local=state->player_station%6400;
        a.fraction14=(local/100)*256+(local%100)*256/100;a.f=a.fraction14/4;a.ff=a.f*a.f/4096;
        a.coeff=a.t[0]+(state->player_station/6400)*228;
        for(unsigned i=0;i<3 && !rc;++i){
            vdplet_view_v1 v;rc=acquire(h,&b->output[i],&v);
            if(rc)fail(c,RALLY_RENDER_REASON_ACQUIRE,rc,rc);
            else if(!view_ok(&v,&b->output[i]))rc=fail(c,RALLY_RENDER_REASON_CAPACITY,VDPL_E_BOUNDS,0);
            else{
                Out out={v.data,0,v.bytes,0};
                if(i==0)c->bands=(uint16_t)road(&out,&a);
                else if(i==1){unsigned n=0;c->cars=(uint16_t)actors(&out,&a,&n);c->signs=(uint16_t)n;}
                else player(&out,&a);
                if(out.error)rc=fail(c,RALLY_RENDER_REASON_CAPACITY,VDPL_E_BOUNDS,0);
                else c->output_bytes[i]=out.n;
            }
            int32_t rr=release(h,&v);if(rr)rc=fail(c,RALLY_RENDER_REASON_RELEASE,rr,rr);
        }
    }
    int32_t rr=release(h,&asset);if(rr)rc=fail(c,RALLY_RENDER_REASON_RELEASE,rr,rr);
    if(c->reason==RALLY_RENDER_REASON_RELEASE)ctx->bound=0;
    if(!rc)c->ready=1;
    return rc;
}
int32_t rally_render_stop_v1(RallyRenderContextV1 *ctx,RallyRenderCompletionV1 *c){
    if(!completion_ok(c) || !ctx || ctx->struct_bytes!=sizeof(*ctx) || objects(ctx,sizeof(*ctx),c,sizeof(*c)))return VDPL_E_ARGUMENT;
    clear_completion(c);ctx->bound=0;return 0;
}
