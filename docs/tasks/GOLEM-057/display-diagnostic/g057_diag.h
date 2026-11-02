#ifndef G057_DISPLAY_DIAG_H
#define G057_DISPLAY_DIAG_H

// Task-local source-copy diagnostic. The preparation script selects 0 or 1.
#define G057_DIAG_ENABLED 1

#if G057_DIAG_ENABLED
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum : uint32_t {
  G057_DEFAULT_PLANE = 1, G057_INITIAL_LIST = 2, G057_PREACTIVATE = 3,
  G057_REQUESTED = 1, G057_RESTORE_PREVIOUS = 2, G057_FINAL_FALLBACK = 3,
  G057_ALLOC_CALLED = 1u << 8, G057_LOOKUP_REQUIRED = 1u << 9,
  G057_MAP_PRESENT = 1u << 10, G057_RAW_SKIP = 1u << 11,
  G057_EXISTING_NULL = 1u << 12, G057_EXISTING_NONNULL = 1u << 13,
  G057_SELECTION_PEEK = 1u << 14, G057_SATURATED = 1u << 15,
  G057_MAGIC = 0x47570100u,
};
struct G057Record { uint32_t w[16]; };
static_assert(sizeof(G057Record) == 64, "journal record must be 64 bytes");

extern "C" void g057_uart_emit(const uint8_t *, size_t);
void g057_begin(uint8_t requested, uint8_t previous);
void g057_attempt(uint8_t attempted, uint8_t phase);
void g057_finish();
bool g057_initial(const void * controller);
G057Record * g057_record(uint32_t event, const void * controller,
                         uint32_t format, uint32_t table_size, uint32_t flags);
void g057_flush();
inline uint32_t g057_ptr(const void * p) { return uint32_t(uintptr_t(p)); }
inline uint32_t g057_plane_flags(bool lookup, bool raw, bool present,
                                const void * existing, bool allocated) {
  return (lookup ? G057_LOOKUP_REQUIRED : 0u) |
         (raw ? G057_RAW_SKIP : 0u) |
         (present ? G057_MAP_PRESENT : 0u) |
         (allocated ? G057_ALLOC_CALLED : 0u) |
         (present && !allocated ? (existing ? G057_EXISTING_NONNULL :
                                             G057_EXISTING_NULL) : 0u);
}

#ifdef G057_DIAG_IMPLEMENTATION
// Exactly 512 journal bytes plus 32 context bytes. No per-controller members.
G057Record g057_records[8];
uint32_t g057_control[8];
enum : uint32_t { G057_SEEN = 1, G057_ARMED = 2, G057_FULL = 4 };
// control: request seq, attempt seq, packed modes, used, emitted, state,
//          initial-list controller for current attempt, reserved.

void g057_begin(uint8_t requested, uint8_t previous) {
  if (requested != 20 || (g057_control[5] & G057_SEEN)) return;
  memset(g057_records, 0, sizeof(g057_records));
  g057_control[0] += 1;
  g057_control[1] = g057_control[3] = g057_control[4] = 0;
  g057_control[2] = uint32_t(requested) | uint32_t(previous) << 8;
  g057_control[5] = G057_SEEN | G057_ARMED;
  g057_control[6] = g057_control[7] = 0;
}
void g057_attempt(uint8_t attempted, uint8_t phase) {
  if (!(g057_control[5] & G057_ARMED)) return;
  ++g057_control[1];
  const uint32_t tag = (phase & 3u) | ((g057_control[1] & 63u) << 2);
  g057_control[2] = (g057_control[2] & 0xffffu) |
                    uint32_t(attempted) << 16 | tag << 24;
  g057_control[6] = 0;
}
void g057_finish() { g057_control[5] &= ~G057_ARMED; }
bool g057_initial(const void * controller) {
  if (!(g057_control[5] & G057_ARMED) ||
      g057_control[6] == g057_ptr(controller)) return false;
  g057_control[6] = g057_ptr(controller);
  return true;
}
G057Record * g057_record(uint32_t event, const void * controller,
                         uint32_t format, uint32_t table_size, uint32_t flags) {
  if (!(g057_control[5] & G057_ARMED)) return nullptr;
  if (g057_control[3] == 8) {
    g057_control[5] |= G057_FULL;
    return nullptr;
  }
  const uint32_t ordinal = ++g057_control[3];
  auto * r = &g057_records[ordinal - 1];
  r->w[0] = G057_MAGIC | event;
  r->w[1] = ordinal | flags;
  r->w[2] = g057_control[0];
  r->w[3] = g057_control[2];
  r->w[4] = g057_ptr(controller);
  r->w[5] = format;
  r->w[6] = table_size;
  if (ordinal == 8) {
    r->w[1] |= G057_SATURATED;
    g057_control[5] |= G057_FULL;
  }
  return r;
}
void g057_flush() {
  while (g057_control[4] < g057_control[3]) {
    const auto & r = g057_records[g057_control[4]++];
    g057_uart_emit(reinterpret_cast<const uint8_t *>(&r), sizeof(r));
  }
}
#endif
#endif
#endif
