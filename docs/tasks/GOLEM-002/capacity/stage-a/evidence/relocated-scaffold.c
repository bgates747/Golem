/* STAGE A MEASUREMENT ONLY. Never deploy this as D2.
 * Exact Rally source/helper; generated root is reacquired AFTER source returns.
 * Existing source success suffix is therefore too early. No ready-clear wrapper,
 * complete field-map/domain policy, root failure suppression or qualified draw
 * gate. Render-only fields absent from current kernels use explicit placeholders.
 * This scaffold measures a concrete typed copy/helper/publication design, not a
 * working game, atomic snapshot contract or final compiler binding feature. */
#define vdplet_entry source_entry
#include "/Agon/mystuff/golem/docs/tasks/GOLEM-002/capacity/stage-a/build/relocated/internal.c"
#undef vdplet_entry
#include "/Agon/mystuff/golem/docs/tasks/GOLEM-002/capacity/stage-a/build/helper/rally_render.c"

void *memcpy(void *to,const void *from,size_t n){
    uint8_t *d=to;const uint8_t *s=from;for(size_t i=0;i<n;++i)d[i]=s[i];return to;
}
void *memset(void *to,int value,size_t n){
    uint8_t *d=to;for(size_t i=0;i<n;++i)d[i]=(uint8_t)value;return to;
}
static RallyRenderContextV1 context={.struct_bytes=sizeof(context)};
/* Non-production standalone resource fixture; no installation is supplied. */
static RallyRenderBindingsV1 binding={76,1,1,
    {1003,VDPL_READ,3,149518},
    {{1004,VDPL_WRITE,9,20480},{1005,VDPL_WRITE,9,2048},{1006,VDPL_WRITE,9,256}},
    2101713653u,3635234752u,{0,5,120,63970,63984,63930}};
static int32_t read_opponents(const vdplet_host_v1*h,RallyRenderStateV1*s){
    const uint16_t ids[3]={504,505,502};
    const uint32_t sizes[3]={24,24,12};
    for(unsigned column=0;column<3;++column){
        vdplet_view_v1 v={sizeof(v),0,0,0,0};
        int32_t rc=h->buffer_acquire(h->context,ids[column],VDPL_READ,0,sizes[column],&v);
        if(rc)return rc;
        for(unsigned i=0;i<6;++i){
            if(column==0)s->opponents[i].station=signed32(get32(v.data+i*4));
            else if(column==1)s->opponents[i].lateral_q8=signed32(get32(v.data+i*4));
            else s->opponents[i].active=get(v.data+i*2);
        }
        rc=h->buffer_release(h->context,v.lease);if(rc)return rc;
    }
    for(unsigned i=0;i<6;++i){
        s->opponents[i].heading_turn16=0; /* missing per-opponent source field */
        if(!s->opponents[i].active)s->opponents[i]=(RallyRenderOpponentV1){0,0,0,0};
    }
    return 0;
}
__attribute__((visibility("default")))
int32_t vdplet_entry(const vdplet_host_v1*h,const vdplet_request_v1*r){
    int32_t rc=source_entry(h,r);if(rc||r->operation!=VDPL_RUN)return rc;
    if(r->entry_id!=3&&r->entry_id!=13&&r->entry_id!=14)return rc;
    vdplet_view_v1 root={sizeof(root),0,0,0,0};
    rc=h->buffer_acquire(h->context,501,VDPL_WRITE,0,VDPL_WHOLE_BUFFER,&root);if(rc)return rc;
    RallyRenderCompletionV1 done={.struct_bytes=48,.schema=1};
    RallyRenderStateV1 snapshot={.struct_bytes=108,.schema=1};
    snapshot.session=get32(root.data+8);
    snapshot.request_generation=get32(root.data+20); /* measurement: wrong freshness order */
    snapshot.simulation_tick=get32(root.data+136);
    snapshot.player_station=signed32(get32(root.data+198));
    snapshot.player_lateral_q8=signed32(get32(root.data+202));
    /* phase/steering/flags/profile/legacy fields are explicit zero placeholders. */
    if(r->entry_id==13){binding.session=snapshot.session;rc=rally_render_bind_v1(h,&context,&binding,&done);}
    else if(r->entry_id==14)rc=rally_render_stop_v1(&context,&done);
    else {rc=read_opponents(h,&snapshot);if(!rc)rc=rally_render_prepare_v1(h,&context,&snapshot,&done);}
    if(!rc){
        vdplet_view_v1 out={sizeof(out),0,0,0,0};
        rc=h->buffer_acquire(h->context,1002,VDPL_WRITE,5,48,&out);
        if(!rc){memcpy(out.data,&done,48);rc=h->buffer_release(h->context,out.lease);}
    }
    int32_t rr=h->buffer_release(h->context,root.lease);
    return rr?rr:rc;
}
