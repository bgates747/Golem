// Thin Golem generated-header hooks. Generic framing/file loading is in AGNB.
#include "agnb_host.h"
#include <agnb_agondev.hpp>
#include <agon/vdp.h>
namespace { agnb::Workspace workspace; }
extern "C" {
int golem_host_load_agnb(const char *filename) {
    return static_cast<int>(agnb::load_buffers_mos(filename,workspace));
}
void golem_host_call(uint16_t buffer){vdp_adv_call_buffer(buffer);}
void golem_host_clear(uint16_t buffer){vdp_adv_clear_buffer(buffer);}
void golem_host_write16(uint16_t buffer,uint16_t offset,uint16_t value){
    const uint8_t command[]={23,0,160,uint8_t(buffer),uint8_t(buffer>>8),5,0xc2,
                            uint8_t(offset),uint8_t(offset>>8),2,0,uint8_t(value),uint8_t(value>>8)};
    mos_puts(reinterpret_cast<const char*>(command),sizeof command,0);
}
}
