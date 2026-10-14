# Renderer composition resource gate — 2026-10-13

GOLEM-037's released first tranche reached a measured resource blocker. The exact
accepted Rally helper linked with a tiny generated Golem source uses 8,224 text
bytes, over the unchanged 8,192-byte SDK limit, before production binding glue.
A resolved helper/minimal-dispatcher stack path uses 544 bytes against declared
512; the measurement-only combined dispatcher uses 576. The SDK builder rejects
the combined image. Author review of a resource/ownership decision is next.

All 14 entries in Rally's accepted helper manifest were verified; four unchanged
source inputs and source/tool pins are siloed under GOLEM-037. Measurements use
actual Xtensa GCC 8.4.0, existing SDK flags and one public dispatcher. Data/image
remain below their current limits. The small helper image passes SDK structural
validation, which neither measures stack nor qualifies execution. The final
build/receipts reproduce identically; full evidence and limitations are in
[the task report](../tasks/GOLEM-037/README.md).

The early combined measurement exposed both renamed internal entries publicly,
creating unsupported PLT references and 8,272 text bytes. Final internal visibility
is hidden, removing PLT references and reducing text to 8,224; the final evidence
supersedes that diagnostic figure. The measured code overage is 32 bytes, while
real field marshaling, completion/error publication and Rally logic remain absent.

FIT-001–003 and the earlier BND-001 agreement are complete. ABI/linking/qualification
and visual acceptance remain open. No capacity policy, generic SDK/firmware,
production compiler, Rally source or emulator profile was changed. No application
was deployed and no physical hardware flashed. The measurement source deliberately
does not implement the accepted D2 success/lease/publication semantics. This is
the resource-failure review branch of the released plan, not a completed D2.

Rally's subsequent route-kernels-twenty-four was read and acknowledged. All 20
manifest files were hash/size verified; its reported 319,072 generated-C host
checks are peer evidence, not a Golem rerun or target claim. Rally has read and
acknowledged Golem's resource-gate-sixteen; both sides agree the next resource
decision belongs to the Author. The shared coordination index now leads with
the latest bidirectional read status, above historical negotiations.

The Author then clarified that increasing limits had already been approved and
reaffirmed that direction. The pending increase-versus-reduce decision is
superseded; numerical replacement budgets are not selected or implemented. The
Author asked to record further investigation, without execution. GOLEM-002 now
contains CAP-001–009 for substantial single VDPlets, realistic Wolf-inspired and
Rally workloads, graduated code/data/stack/load pressure and lifecycle testing,
generic service gaps, hardware-versus-emulator evidence and upstream suitability.
The deployment aim is a generic upstream runtime enabling separately downloadable
renderers/engines through compact calls, not application-specific firmware flashes.
Wolf source review establishes inspiration and drawing-lifetime requirements,
not module feasibility/capacity or authority to migrate that project. No new
runtime changes or tests were performed by this planning update.

Rally budget-increase-twenty-five asked for exact numerical limits, stack scope,
SDK/tool identities and qualification/re-measurement plans. Read/acknowledged and
answered in Golem budget-direction-protocol-seventeen. The reply separates the
approved increase direction (including the stack blocker), unchanged implemented
policy and still-absent new admission evidence; it supplies the clean SDK commit/
source/tool hashes and planned sequence without inventing numerical budgets.

Rally twenty-six accepted that clarification; twenty-seven requested the concrete
plan under existing GOLEM-002. Added GOLEM-002/capacity/README.md and marked CAP-003
complete as proposed documentation only. It specifies independent/mixed-pressure
ladders, repeat counts, lifecycle/admission/failure/stop criteria, memory/stack
reserve, generic services and exact implementation ownership, receipts and ordered
real-composition/host/Xtensa/human/separate-physical stages. Ladder values are
proposed test coordinates, not replacement admission limits or executed cases.
Returned exact path/hash in Golem capacity-plan-review-eighteen and stopped for
Author review. No new builds, stress runs, SDK/runtime changes or deployment.
