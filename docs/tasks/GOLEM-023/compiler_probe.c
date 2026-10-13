/* Research only: inspect the pinned compiler's instructions/imports/stack.
 * This is not compiler output, a VDPlet, or a proposed production library. */
#include <stdint.h>

#define PROBE __attribute__((noinline, visibility("default")))

PROBE float scalar_arithmetic(float x, float scale, float bias) {
    return x * scale + bias;
}
PROBE float scalar_subtract(float x, float y) { return x - y; }
PROBE int scalar_compare(float x, float y) { return x < y; }
PROBE float scalar_divide(float x, float y) { return x / y; }
PROBE float scalar_sqrt(float x) { return __builtin_sqrtf(x); }
PROBE float scalar_floor(float x) { return __builtin_floorf(x); }
PROBE float from_uint16(uint16_t x) { return (float)x; }
/* No admission checks here: this is an instruction probe, not a safe cast. */
PROBE uint16_t to_uint16(float x) { return (uint16_t)x; }
PROBE int checked_uint16_add(uint16_t left, uint16_t right, uint16_t *out) {
    uint32_t candidate = (uint32_t)left + (uint32_t)right;
    if (candidate > UINT16_MAX) return 1;
    *out = (uint16_t)candidate;
    return 0;
}
PROBE int checked_uint16_multiply(uint16_t left, uint16_t right, uint16_t *out) {
    uint32_t candidate = (uint32_t)left * (uint32_t)right;
    if (candidate > UINT16_MAX) return 3;
    *out = (uint16_t)candidate;
    return 0;
}
