/* Resource measurement only. Sequence a real generated Golem dispatcher and
   the exact helper dispatcher, omitting production marshaling/publication.
   In particular Golem's success suffix runs before the helper here: this file
   deliberately does NOT implement the accepted D2 execution contract. */
#define vdplet_entry golem_fit_entry
#include "build/fit-golem.c"
#undef vdplet_entry
#define FIT_ENTRY_VISIBILITY "hidden"
#define vdplet_entry helper_fit_entry
#include "fit_dispatch.c"
#undef vdplet_entry

__attribute__((visibility("default")))
int32_t vdplet_entry(const vdplet_host_v1 *host, const vdplet_request_v1 *request) {
    int32_t result = golem_fit_entry(host, request);
    if (result) return result;
    return helper_fit_entry(host, request);
}
