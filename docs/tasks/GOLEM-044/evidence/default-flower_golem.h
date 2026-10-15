/* Generated from compiler metadata; task-local C interface prototype. */
/* golem_NAME submits a call. It does not wait for VDP completion. */
#ifndef FLOWER_GOLEM_GENERATED_H
#define FLOWER_GOLEM_GENERATED_H
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <float.h>
#ifdef __cplusplus
extern "C" {
#define GOLEM_ASSERT static_assert
#else
#define GOLEM_ASSERT _Static_assert
#endif
GOLEM_ASSERT(sizeof(uint16_t)==2 && sizeof(uint32_t)==4, "Golem integer storage");
GOLEM_ASSERT(sizeof(float)==4 && FLT_RADIX==2 && FLT_MANT_DIG==24 && FLT_MAX_EXP==128, "Golem binary32 storage");
typedef enum { GOLEM_BUFFERED, GOLEM_NATIVE } golem_backend;
/* Implement this tiny transport adapter in the host application. */
void golem_host_call(uint16_t entry_buffer);
#define GOLEM_STATE_BUFFER 1u
#define GOLEM_STATE_BYTES 142u
#define GOLEM_ERROR_OFFSET 14u
#define GOLEM_NATIVE_HANDLE_OFFSET 24u
static inline void golem_put16(uint8_t *out,uint16_t n) { out[0]=(uint8_t)n;out[1]=(uint8_t)(n>>8); }
static inline void golem_put32(uint8_t *out,uint32_t n) { for(unsigned i=0;i<4;++i)out[i]=(uint8_t)(n>>(8*i)); }
#define GOLEM_ROOT_MAGIC_OFFSET 0u
#define GOLEM_ROOT_SCHEMA_OFFSET 2u
#define GOLEM_ROOT_HEADER_BYTES_OFFSET 4u
#define GOLEM_ROOT_ROOT_BYTES_OFFSET 6u
#define GOLEM_ROOT_SESSION_OFFSET 8u
#define GOLEM_ROOT_INTERFACE_STATUS_OFFSET 12u
#define GOLEM_ROOT_LANGUAGE_STATUS_OFFSET 14u
#define GOLEM_ROOT_RUNTIME_RESULT_OFFSET 16u
#define GOLEM_ROOT_LAST_SUCCESS_GENERATION_OFFSET 20u
#define GOLEM_ROOT_LAST_SUCCESS_ENTRY_OFFSET 24u
#define GOLEM_ROOT_DESCRIPTOR_COUNT_OFFSET 26u
#define GOLEM_ROOT_REQUESTED_ENTRY_OFFSET 28u
#define GOLEM_ROOT_LAST_SUCCESS_SESSION_OFFSET 32u
#define GOLEM_ROOT_ENTRY_COUNT_OFFSET 36u
#define GOLEM_ROOT_GLOBALS_BYTES_OFFSET 38u
#define GOLEM_ROOT_CONTRACT_TAG_OFFSET 44u
#define GOLEM_ROOT_DESCRIPTORS_OFFSET 48u
#define GOLEM_COMMON_INSTALL_BYTES 156u
#define GOLEM_BUFFERED_INSTALL_BYTES 8451u
#define GOLEM_NATIVE_INSTALL_BYTES 104u
#define GOLEM_BUFFERED_CLEANUP_BYTES 660u
#define GOLEM_NATIVE_CLEANUP_BYTES 18u
#define GOLEM_petals_OFFSET 112u
static inline void golem_set_petals(uint8_t *state,float value) {
uint32_t bits;memcpy(&bits,&value,4);
golem_put32(state+GOLEM_petals_OFFSET,bits);
}
#define GOLEM_vectors_OFFSET 116u
static inline void golem_set_vectors(uint8_t *state,float value) {
uint32_t bits;memcpy(&bits,&value,4);
golem_put32(state+GOLEM_vectors_OFFSET,bits);
}
#define GOLEM_depth_OFFSET 120u
static inline void golem_set_depth(uint8_t *state,float value) {
uint32_t bits;memcpy(&bits,&value,4);
golem_put32(state+GOLEM_depth_OFFSET,bits);
}
#define GOLEM_periods_OFFSET 124u
static inline void golem_set_periods(uint8_t *state,float value) {
uint32_t bits;memcpy(&bits,&value,4);
golem_put32(state+GOLEM_periods_OFFSET,bits);
}
#define GOLEM_shrink_OFFSET 128u
static inline void golem_set_shrink(uint8_t *state,float value) {
uint32_t bits;memcpy(&bits,&value,4);
golem_put32(state+GOLEM_shrink_OFFSET,bits);
}
#define GOLEM_radius_scale_OFFSET 132u
static inline void golem_set_radius_scale(uint8_t *state,float value) {
uint32_t bits;memcpy(&bits,&value,4);
golem_put32(state+GOLEM_radius_scale_OFFSET,bits);
}
#define GOLEM_theta_init_OFFSET 136u
static inline void golem_set_theta_init(uint8_t *state,float value) {
uint32_t bits;memcpy(&bits,&value,4);
golem_put32(state+GOLEM_theta_init_OFFSET,bits);
}
#define GOLEM_batches_OFFSET 140u
static inline void golem_set_batches(uint8_t *state,uint16_t value) {
golem_put16(state+GOLEM_batches_OFFSET,(uint16_t)value);
}
#define GOLEM_reset_LOGICAL_ID 0u
#define GOLEM_reset_BUFFERED 110u
#define GOLEM_reset_NATIVE 113u
static inline void golem_reset(golem_backend backend) {
golem_host_call(backend==GOLEM_NATIVE?GOLEM_reset_NATIVE:GOLEM_reset_BUFFERED);
}
#define GOLEM_step_LOGICAL_ID 1u
#define GOLEM_step_BUFFERED 111u
#define GOLEM_step_NATIVE 114u
static inline void golem_step(golem_backend backend) {
golem_host_call(backend==GOLEM_NATIVE?GOLEM_step_NATIVE:GOLEM_step_BUFFERED);
}
#undef GOLEM_ASSERT
#ifdef __cplusplus
}
#endif
#endif
