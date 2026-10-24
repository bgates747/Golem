#pragma once
/* Link agnb_host.cpp once; include generic AGNB api/ on the compiler path. */
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int golem_host_load_agnb(const char *filename);
void golem_host_call(uint16_t buffer);
void golem_host_clear(uint16_t buffer);
void golem_host_write16(uint16_t buffer,uint16_t offset,uint16_t value);
#ifdef __cplusplus
}
#endif
