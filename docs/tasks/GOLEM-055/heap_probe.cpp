// Task-local, read-only sampler for the pinned AgonDev malloc free list.
// Not linked into distributed compiler workers. No allocator is replaced.
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern "C" {
// AgonDev prepends one underscore to these C names. The linked symbols are
// ___heapbot, ___heaptop, __sbrkbase and __alloc_base respectively.
extern unsigned char __heapbot[], __heaptop[];
extern unsigned char *_sbrkbase;
extern unsigned char _alloc_base[];
}
namespace ag {extern const char *phase;}

namespace {
struct Block {Block *next;size_t bytes;};
static_assert(sizeof(Block)==6,"This probe requires AgonDev's 24-bit allocator");
struct Snapshot {
    size_t extent,used,free_bins,tail,total_free,largest_payload,free_blocks,request;
    const char *phase;
    bool success,valid;
};
static Snapshot peak{},failure{};
static size_t samples=0,failures=0,largest_request=0;

Snapshot read_heap(size_t request,bool success) {
    Snapshot s{};s.request=request;s.success=success;s.phase=ag::phase;
    uintptr_t bottom=(uintptr_t)__heapbot,top=(uintptr_t)__heaptop;
    uintptr_t current=(uintptr_t)_sbrkbase;
    if(current<bottom||current>top)return s;
    s.extent=current-bottom;s.tail=top-current;
    // sbrk rejects a new end >= heap limit, hence one byte must remain.
    if(s.tail>sizeof(Block))s.largest_payload=s.tail-sizeof(Block)-1;
    uintptr_t previous=0;
    Block *node=reinterpret_cast<Block*>(_alloc_base)->next;
    while(node) {
        uintptr_t at=(uintptr_t)node;
        if(at<bottom||at<=previous||at>current||current-at<sizeof(Block))return s;
        size_t bytes=node->bytes;
        if(bytes<sizeof(Block)||bytes>current-at||s.free_bins>s.extent-bytes)return s;
        s.free_bins+=bytes;++s.free_blocks;
        if(bytes-sizeof(Block)>s.largest_payload)s.largest_payload=bytes-sizeof(Block);
        previous=at;node=node->next;
    }
    s.used=s.extent-s.free_bins;s.total_free=s.free_bins+s.tail;s.valid=true;
    return s;
}
void print_snapshot(FILE *f,const char *label,const Snapshot &s) {
    fprintf(f,"%s valid=%u request=%lu success=%u phase=%s extent=%lu used_with_headers=%lu free_bins=%lu uncommitted_tail=%lu total_free=%lu largest_payload_fit=%lu free_blocks=%lu\n",
        label,unsigned(s.valid),(unsigned long)s.request,unsigned(s.success),s.phase?s.phase:"(none)",
        (unsigned long)s.extent,(unsigned long)s.used,(unsigned long)s.free_bins,
        (unsigned long)s.tail,(unsigned long)s.total_free,(unsigned long)s.largest_payload,
        (unsigned long)s.free_blocks);
}
}

extern "C" void heap_probe_reset() {
    memset(&peak,0,sizeof peak);memset(&failure,0,sizeof failure);
    samples=failures=largest_request=0;
}
extern "C" void heap_probe_sample(size_t request,bool success) {
    ++samples;if(request>largest_request)largest_request=request;
    const Snapshot s=read_heap(request,success);
    if(s.valid&&(!peak.valid||s.used>peak.used||(s.used==peak.used&&s.extent>peak.extent)))peak=s;
    if(!success){++failures;failure=s;}
}
extern "C" void heap_probe_report(const char *job) {
    const Snapshot final=read_heap(0,true);
    // Reporting happens after compiler recovery/return. The sampler itself
    // uses no allocation, I/O, or extra per-allocation metadata.
    // Append mode failed in stock emulator qualification. Use the ordinary
    // read/write modes already used by other compiler artifacts instead.
    FILE *f=fopen("heap.log","r+b");
    if(f){if(fseek(f,0,SEEK_END)){fclose(f);f=nullptr;}}
    else f=fopen("heap.log","wb");
    if(!f)f=stderr;
    fprintf(f,"job=%s samples=%lu failures=%lu largest_request=%lu heap_bytes=%lu sampler_scope=after_ag_memory_attempts\n",
        job?job:"(direct)",(unsigned long)samples,(unsigned long)failures,
        (unsigned long)largest_request,(unsigned long)((uintptr_t)__heaptop-(uintptr_t)__heapbot));
    print_snapshot(f,"peak",peak);if(failures)print_snapshot(f,"failure",failure);
    print_snapshot(f,"final",final);fflush(f);if(f!=stderr)fclose(f);
}
