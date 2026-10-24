#include "vdplet.h"
__attribute__((visibility("default")))
int32_t vdplet_entry(const vdplet_host_v1 *h,const vdplet_request_v1 *r) {
    if (!h || !r || h->struct_bytes!=36 || h->abi_major!=1 || h->abi_minor!=0 ||
        h->capabilities!=3) return VDPL_E_ABI;
    return VDPL_OK;
}
