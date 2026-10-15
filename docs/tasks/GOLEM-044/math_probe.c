/* Link/admission experiment only; not the Flower implementation. */
#include <math.h>
#include "vdplet.h"

static volatile float values[5] = {0.5f, 2.0f, 0.0f, 0.0f, 0.0f};

__attribute__((visibility("default")))
int32_t vdplet_entry(const vdplet_host_v1 *host, const vdplet_request_v1 *request) {
    if (!host || !request || host->abi_major != VDPL_ABI_MAJOR) return VDPL_E_ABI;
    if (request->operation != VDPL_RUN) return VDPL_OK;
    values[2] = values[0] / values[1];
    values[3] = sinf(values[0]);
    values[4] = cosf(values[0]);
    return VDPL_OK;
}
