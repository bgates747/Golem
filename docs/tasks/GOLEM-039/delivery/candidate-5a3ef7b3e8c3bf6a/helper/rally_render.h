#ifndef RALLY_RENDER_HELPER_V1_H
#define RALLY_RENDER_HELPER_V1_H
#include "vdplet.h"
#include <stddef.h>

#define RALLY_RENDER_SCHEMA 1u
#define RALLY_RENDER_NORMAL 0u
#define RALLY_RENDER_R24_REFERENCE 1u
enum {
    RALLY_RENDER_REASON_OK=0, RALLY_RENDER_REASON_ARGUMENT=1,
    RALLY_RENDER_REASON_STATE=2, RALLY_RENDER_REASON_ASSET=3,
    RALLY_RENDER_REASON_ALIAS=4, RALLY_RENDER_REASON_CAPACITY=5,
    RALLY_RENDER_REASON_ACQUIRE=6, RALLY_RENDER_REASON_RELEASE=7,
    RALLY_RENDER_REASON_NOT_BOUND=8
};
typedef struct RallyRenderRangeV1 {
    uint16_t buffer,access;
    uint32_t offset,bytes;
} RallyRenderRangeV1;
typedef struct RallyRenderResourcesV1 {
    uint16_t player_bitmap_base,traffic_bitmap_base,sign_bitmap_base;
    uint16_t traffic_matrix_base,player_mirror_matrix,sign_matrix;
} RallyRenderResourcesV1;
typedef struct RallyRenderBindingsV1 {
    uint16_t struct_bytes,schema;
    uint32_t session;
    RallyRenderRangeV1 asset,output[3];
    uint32_t track_identity,asset_checksum;
    RallyRenderResourcesV1 resources;
} RallyRenderBindingsV1;
typedef struct RallyRenderOpponentV1 {
    int32_t station,lateral_q8;
    int16_t heading_turn16;
    uint16_t active;
} RallyRenderOpponentV1;
typedef struct RallyRenderStateV1 {
    uint16_t struct_bytes,schema;
    uint32_t session,request_generation,simulation_tick;
    int32_t player_station,player_lateral_q8;
    uint16_t phase;
    int16_t steering;
    uint16_t flags,presentation_profile;
    /* Historical fixture presentation only; NORMAL requires both zero.
       These do not select the new game's recovery rules. */
    uint16_t reference_crash_remaining,reference_invulnerable;
    RallyRenderOpponentV1 opponents[6];
} RallyRenderStateV1;
typedef struct RallyRenderCompletionV1 {
    uint16_t struct_bytes,schema;
    int32_t result,service_result;
    uint32_t session,request_generation,simulation_tick,output_bytes[3];
    uint16_t bands,cars,signs,reason,ready,reserved;
} RallyRenderCompletionV1;
typedef struct RallyRenderActorScratchV1 {
    int32_t bitmap,matrix,scale,xx,translation,x,y;
} RallyRenderActorScratchV1;
typedef struct RallyRenderContextV1 {
    uint32_t struct_bytes,bound;
    RallyRenderBindingsV1 bindings;
    /* Entry-local values retained as private storage, never leased pointers. */
    RallyRenderStateV1 snapshot;
    RallyRenderActorScratchV1 cars[6],signs[2];
    int32_t order[6],distance[6];
} RallyRenderContextV1;

#if defined(__cplusplus)
#define RALLY_ASSERT static_assert
extern "C" {
#else
#define RALLY_ASSERT _Static_assert
#endif
RALLY_ASSERT(sizeof(RallyRenderRangeV1)==12,"range ABI");
RALLY_ASSERT(sizeof(RallyRenderResourcesV1)==12,"resource ABI");
RALLY_ASSERT(sizeof(RallyRenderBindingsV1)==76,"bindings ABI");
RALLY_ASSERT(offsetof(RallyRenderBindingsV1,resources)==64,"resource offset");
RALLY_ASSERT(sizeof(RallyRenderOpponentV1)==12,"opponent ABI");
RALLY_ASSERT(sizeof(RallyRenderStateV1)==108,"state ABI");
RALLY_ASSERT(offsetof(RallyRenderStateV1,opponents)==36,"opponent offset");
RALLY_ASSERT(sizeof(RallyRenderCompletionV1)==48,"completion ABI");
RALLY_ASSERT(offsetof(RallyRenderCompletionV1,ready)==44,"ready offset");
RALLY_ASSERT(sizeof(RallyRenderActorScratchV1)==28,"actor scratch ABI");
RALLY_ASSERT(sizeof(RallyRenderContextV1)==464,"context ABI");
RALLY_ASSERT(offsetof(RallyRenderContextV1,snapshot)==84,"snapshot offset");

int32_t rally_render_bind_v1(const vdplet_host_v1 *,RallyRenderContextV1 *,
                           const RallyRenderBindingsV1 *,RallyRenderCompletionV1 *);
int32_t rally_render_prepare_v1(const vdplet_host_v1 *,RallyRenderContextV1 *,
                              const RallyRenderStateV1 *,RallyRenderCompletionV1 *);
int32_t rally_render_stop_v1(RallyRenderContextV1 *,RallyRenderCompletionV1 *);
#if defined(__cplusplus)
}
#endif
#undef RALLY_ASSERT
#endif
