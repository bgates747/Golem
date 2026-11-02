#define G057_DIAG_IMPLEMENTATION
#include "g057_diag.h"
#include <cassert>
#include <cstdio>

static G057Record delivered[8];
static unsigned delivered_count;
extern "C" void g057_uart_emit(const uint8_t * bytes, size_t size) {
  assert(size == 64 && delivered_count < 8);
  memcpy(&delivered[delivered_count++], bytes, size);
}
static void reset() {
  memset(g057_records, 0, sizeof(g057_records));
  memset(g057_control, 0, sizeof(g057_control));
  memset(delivered, 0, sizeof(delivered));
  delivered_count = 0;
}
int main() {
  static_assert(sizeof(g057_records) + sizeof(g057_control) == 544, "fixed budget");
  const auto * controller = reinterpret_cast<const void *>(0x3ffb92d4u);
  const auto * nonnull = reinterpret_cast<const void *>(0x3ffca000u);
  // Intentional raw skip, absent plane allocation failure, and an existing-null
  // map entry remain three different classifications.
  assert(g057_plane_flags(false, true, false, nullptr, false) == G057_RAW_SKIP);
  assert(g057_plane_flags(true, false, false, nullptr, true) ==
         (G057_LOOKUP_REQUIRED | G057_ALLOC_CALLED));
  assert(g057_plane_flags(true, false, true, nullptr, false) ==
         (G057_LOOKUP_REQUIRED | G057_MAP_PRESENT | G057_EXISTING_NULL));
  assert(g057_plane_flags(true, false, true, nonnull, false) ==
         (G057_LOOKUP_REQUIRED | G057_MAP_PRESENT | G057_EXISTING_NONNULL));
  reset();
  g057_begin(11, 11);
  assert(!g057_record(G057_DEFAULT_PLANE, controller, 0, 0, 0));
  g057_begin(20, 11);
  g057_attempt(20, G057_REQUESTED);
  auto * r = g057_record(G057_DEFAULT_PLANE, controller, 0, 0, G057_RAW_SKIP);
  assert(r && r->w[3] == 0x05140b14u && r->w[4] == 0x3ffb92d4u);
  assert(g057_initial(controller) && !g057_initial(controller));
  for (unsigned i = 0; i < 2; ++i) assert(g057_record(i + 2, controller, 0, 0, 0));
  assert(delivered_count == 0); // No byte emission before explicit preactivation.
  g057_flush();
  g057_flush();
  assert(delivered_count == 3); // Never emit a slot twice.
  g057_attempt(11, G057_RESTORE_PREVIOUS);
  assert(g057_initial(controller)); // Address reuse/new attempt is distinct.
  for (unsigned i = 0; i < 3; ++i) assert(g057_record(i + 1, controller, 1, 2048, 0));
  assert(g057_records[3].w[3] == 0x0a0b0b14u);
  g057_flush();
  assert(delivered_count == 6);
  for (unsigned i = 6; i < 8; ++i) {
    for (unsigned word = 0; word < 16; ++word) assert(g057_records[i].w[word] == 0);
  }
  g057_finish();
  assert(!g057_record(1, controller, 1, 2048, 0));
  g057_begin(20, 11); // One-shot: a later request does not rearm or reset.
  assert(!g057_record(1, controller, 1, 2048, 0));
  reset();
  g057_begin(20, 11);
  g057_attempt(20, G057_REQUESTED);
  for (unsigned i = 0; i < 8; ++i) assert(g057_record(1, controller, 1, 2048, 0));
  assert(g057_records[7].w[1] & G057_SATURATED);
  const auto saved_first = g057_records[0];
  assert(!g057_record(2, controller, 1, 2048, 0));
  assert(!memcmp(&saved_first, &g057_records[0], sizeof(saved_first)));
  g057_flush();
  g057_flush();
  assert(delivered_count == 8 && g057_control[3] == 8);
  puts("PASS: classifications, context, one-shot, no early/duplicate emission, unused slots, fixed budget, saturation/no overwrite");
}
