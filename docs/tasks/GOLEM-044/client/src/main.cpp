// Compile/link proof only. This is not the Flower client or an installed demo.
#include <agon/mos.h>
#include <agon/vdp.h>
#include <stdint.h>
#include "generated/flower_golem.h"

static_assert(sizeof(int)==3, "AgonDev eZ80 int is 24 bits");
static_assert(sizeof(void*)==3, "AgonDev eZ80 pointer is 24 bits");
static_assert(sizeof(VDU_ADV_CMD)==6, "AgonDev buffered-call command size");

extern "C" void golem_host_call(uint16_t entry_buffer) {
    vdp_adv_call_buffer(entry_buffer);
}

uint8_t flower_abi_state[GOLEM_STATE_BYTES] = {};

// Deliberately uncalled: these entries must be installed and their fresh native
// handle bound before submission. Keeping a real consumer proves C linkage.
extern "C" void flower_abi_submission_probe(golem_backend backend, float angle) {
    golem_set_theta_init(flower_abi_state, angle);
    golem_step(backend);
}

int main() {
    // No module, buffer, mode, keyboard or filesystem side effects.
    return 0;
}
