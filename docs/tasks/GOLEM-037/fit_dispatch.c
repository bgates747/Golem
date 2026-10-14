/* Resource measurement only: no Golem marshaling, retained publication or RUN
   qualification. Static inputs deliberately minimize the dispatcher's stack.
   This is not a deployable application and must not be used as one. */
#include "helper/rally_render.c"

/* Xtensa GCC lowers the helper's native struct copies to memcpy. The SDK links
   without libc, so a composed module must carry this bounded-by-caller copy. */
void *memcpy(void *destination, const void *source, size_t bytes) {
    uint8_t *out = destination;
    const uint8_t *in = source;
    for (size_t i = 0; i < bytes; ++i) out[i] = in[i];
    return destination;
}

static RallyRenderContextV1 fit_context = { .struct_bytes = sizeof(RallyRenderContextV1) };
static RallyRenderBindingsV1 fit_bindings;
static RallyRenderStateV1 fit_state;
static RallyRenderCompletionV1 fit_completion = { .struct_bytes = sizeof(RallyRenderCompletionV1), .schema = 1 };

#ifndef FIT_ENTRY_VISIBILITY
#define FIT_ENTRY_VISIBILITY "default"
#endif
__attribute__((visibility(FIT_ENTRY_VISIBILITY)))
int32_t vdplet_entry(const vdplet_host_v1 *host, const vdplet_request_v1 *request) {
    if (!request || request->struct_bytes != sizeof(*request)) return VDPL_E_ARGUMENT;
    if (request->operation == VDPL_INIT || request->operation == VDPL_FINI) return VDPL_OK;
    if (request->operation != VDPL_RUN) return VDPL_E_ARGUMENT;
    switch (request->entry_id) {
        case 0: return rally_render_bind_v1(host, &fit_context, &fit_bindings, &fit_completion);
        case 1: return rally_render_prepare_v1(host, &fit_context, &fit_state, &fit_completion);
        case 2: return rally_render_stop_v1(&fit_context, &fit_completion);
        default: return VDPL_E_ARGUMENT;
    }
}
