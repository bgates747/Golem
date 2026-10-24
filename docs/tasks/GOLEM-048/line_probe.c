#include "vdplet.h"
static uint16_t get(const uint8_t *p){return p[0]|(uint16_t)p[1]<<8;}
static void put(uint8_t *p,uint16_t n){p[0]=n;p[1]=n>>8;}
__attribute__((visibility("default")))
int32_t vdplet_entry(const vdplet_host_v1 *h,const vdplet_request_v1 *r){
 const vdplet_host_v1_2 *x=vdplet_lines_host(h);
 if(!x||!r||h->struct_bytes!=44||h->abi_minor!=2||h->capabilities!=15)return VDPL_E_ABI;
 if(r->operation==VDPL_INIT||r->operation==VDPL_FINI)
  return x->draw_lines(h->context,0,0)==VDPL_E_UNSUPPORTED?VDPL_OK:VDPL_E_INIT;
 if(r->operation!=VDPL_RUN||r->argument_bytes!=4||r->entry_id>6)return VDPL_E_ARGUMENT;
 vdplet_view_v1 a,o,c,extra;
 a.struct_bytes=sizeof(a);o.struct_bytes=sizeof(o);c.struct_bytes=sizeof(c);extra.struct_bytes=sizeof(extra);
 a.lease=o.lease=c.lease=extra.lease=0;
 int32_t rc=h->buffer_acquire(h->context,r->argument_buffer,VDPL_READ,0,4,&a);
 if(rc)return rc;
 rc=h->buffer_acquire(h->context,get(a.data+2),VDPL_WRITE,0,8,&o);
 if(!rc)rc=h->buffer_acquire(h->context,get(a.data),VDPL_READ,0,VDPL_WHOLE_BUFFER,&c);
 uint16_t first=0,before=0,after=0;
 if(!rc){
  if(r->entry_id==0){
   rc=h->buffer_acquire(h->context,60026,VDPL_READ,0,2,&extra);
   if(!rc){before=get(extra.data);rc=x->draw_lines(h->context,(const vdplet_line_v1*)c.data,c.bytes/8);after=get(extra.data);}
  }else if(r->entry_id==1){
   vdplet_frame_v1 f;f.struct_bytes=sizeof(f);
   rc=h->frame_begin(h->context,0,0,8,1,&f);
   if(!rc){rc=x->draw_lines(h->context,(const vdplet_line_v1*)c.data,c.bytes/8);first=h->frame_end(h->context,f.lease);}
  }else if(r->entry_id==2)rc=x->draw_lines(h->context,0,1);
  else if(r->entry_id==3)rc=x->draw_lines(h->context,0,VDPL_LINES_MAX_COUNT+1);
  else if(r->entry_id==4){
   first=x->draw_lines(h->context,(const vdplet_line_v1*)c.data,c.bytes/8);
   rc=first?first:x->draw_lines(h->context,0,VDPL_LINES_MAX_COUNT+1);
  }else if(r->entry_id==5)rc=x->draw_lines(h->context,0,0);
  else rc=x->draw_lines(0,(const vdplet_line_v1*)c.data,c.bytes/8);
  put(o.data,rc);put(o.data+2,first);put(o.data+4,before);put(o.data+6,after);
 }
 if(extra.lease)h->buffer_release(h->context,extra.lease);
 if(c.lease)h->buffer_release(h->context,c.lease);
 if(o.lease)h->buffer_release(h->context,o.lease);
 h->buffer_release(h->context,a.lease);
 return rc;
}
