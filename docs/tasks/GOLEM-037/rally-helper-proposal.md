# Rally renderer helper proposal — R25-RENDER v0.1

Rally proposes one linked native helper behind generated Golem `Render30`, using the accepted root-v1 envelope and generated retained-buffer bindings. This is a proposal for Golem review, not a delivered compiler binding or new player wire format. The renderer remains finite and synchronous; firmware owns scheduling and serializes calls/drawing. Golem's scalar D0 bundle is verified independently in `D0-RECEIPT.json`; arrays/wide math and linking remain later deliveries.

## Proposed C boundary

```c
int32_t rally_render_bind_v1(const vdplet_host_v1 *host,
                           const RallyRenderBindingsV1 *bindings);
int32_t rally_render_prepare_v1(const vdplet_host_v1 *host,
                              const RallyRenderBindingsV1 *bindings,
                              const RallyRenderStateV1 *state,
                              RallyRenderCompletionV1 *completion);
void rally_render_stop_v1(void);
```

These are logical proposed names; linking/signatures require peer acceptance under TRF-001-14. All scalar members are explicit uint16/uint32/sint16/sint32, with native compiler layout assertions, struct byte length and schema fields. Structures are native typed call arguments with entry-scoped lifetime; they are not an implicit packed UART packet. No parameterized source-language functions or general Golem pointers are required: the compiler may expose an explicitly declared intrinsic/binding that marshals generated state into these native arguments. Unsupported buffered targets must reject this native-helper requirement explicitly at compile time. Do not silently lower to an empty draw or substitute a second resident module.

`RallyRenderBindingsV1` declares track identity, immutable asset directory/view, and road/actor/player output IDs plus offsets/capacities and completion storage. Values come from generated installation descriptors/manifests; no production IDs are assigned here. Root role 3 supplies simulation tables as agreed; role 4 is asset-directory metadata, whose entries may address multiple retained buffers. The four-role D0 envelope is retained. Completion storage may be generated root globals or a directory-declared record; it must not consume collision-event role 2 by convention. Exact directory encoding and root offsets await joint agreement.

`RallyRenderStateV1` is one coherent already-completed simulation state: session, render request/simulation generation, normalized player station/road-material phase, player lateral Q8/steering, camera/presentation flags and six VDP-owned active opponent station/lateral/route-heading records. It is assembled inside the native module from generated globals/SoA, rather than sending opponent state from eZ80. Validate the frozen track/pose ranges before rendering. Recovery/effect presentation needs a later agreed extension; retained R24 effect-age cases remain historical reference fixtures and do not freeze the new countdown or respawn rule. HUD/sky/page-history values remain separate integration responsibilities until their boundary is agreed.

`RallyRenderCompletionV1` reports a helper result, matched session/request/simulation generation, three command lengths and optional bounded counts (bands/cars/signs). It is separate from interface status, Golem language status, host-owned runtime_result and root successful-invocation generation. The helper publishes a ready marker last only after every output is complete and terminated. Helper failure must suppress logical Render30 success, mapped through an explicitly separate helper/runtime channel; do not reuse numeric arithmetic codes for lease/output failures. Golem must specify how an intrinsic failure propagates before this binding is frozen. Current root-v1 has no dedicated helper status field; its generated source globals can declare one or the composition layer can publish a reviewed extension without changing the header silently.

## Invocation and failure rules

Bind validates asset identity/checksum, directory ranges/capacities and disjoint writable ranges while quiescent. It retains IDs/identity only; it caches no raw pointer/lease across entries. Prepare copies the coherent state, validates it, and acquires/releases immutable asset and one writable output view at a time. Including the generated root lease, plan for at most three live views; actual compiler/helper composition must measure the peak. Offset/length checks use subtraction to avoid unsigned overflow. Failed acquisition, stale identity, malformed input or output exhaustion yields failure, releases every acquired view and leaves ready false; already overwritten bytes are not rolled back.

The invocation owner clears completion readiness **before** attempting RUN, including runtime rejection before native entry. After return, it requires successful runtime/interface/language/helper results and matched completion identity before consuming outputs. A success generation from an earlier invocation cannot authorize drawing. Root success-generation remains modulo 2^32; production session/wrap policy is a separate agreed lifecycle matter. R24's old ready-byte comparison workaround is reference evidence, not an unreviewed new root wrapper.

Output buffers are complete ordinary VDU command lists, never partially executable streams. The sole drawing owner consumes them before reuse; helper return does not establish Canvas completion or scanout. Retain scene order: scenery/viewport, road plus finish line, depth-merged opponents/signs, player/effects, panel/HUD, swap/page-history update. Existing sky and HUD histories belong to each displayed page. Stop drains invocation/drawing ownership in firmware before helper invalidation or unload; `rally_render_stop_v1` only invalidates helper-private binding state and does not implement that drain itself.

## Questions for Golem and acceptance examples

Confirm whether a declared native-only binding/intrinsic is the preferred compiler surface, and how helper failure prevents root success publication while preserving earlier source effects. Confirm generated declaration access to root globals/SoA and retained descriptors, without relying on private compiler offsets. Propose the generated manifest section for binding signatures/capabilities and helper status/completion locations. Names/types/layout assertions then become the D2 sub-contract; implementing a general foreign-function interface is not required.

Qualification must cover changed/relocated IDs, unknown helper/schema/unsupported buffered target, invalid asset/snapshot/capacity/alias, runtime rejection before entry, no stale draw after any failure, one linked dispatcher, bounded leases and stop-before-unload. The deterministic host fixtures compare old native B commands against the separately frozen R22/R24 oracle; pixel exceptions remain explicit and physical behavior remains unqualified. Resource totals, capacity admission and 120/30-Hz performance require later combined target evidence.
