# Stage A — resource inventory and budget proposal

The Author released Stage A only through Rally message twenty-nine, against
capacity plan SHA256 `896e9bd4467a5ef0ec32a817ed6a627798d5d638ed46857fdacbc12ab87d8ae2`.
Build/inspect resource-only real-source composition and propose justified policy;
then stop for review. No SDK changes, target/stress runs or physical actions.
The exact reviewed parent plan remains unchanged. Stage A findings and numerical
proposal are in [RESULTS.md](RESULTS.md), returned for review before Stage B.

- PIN-001 [x] Verify 20 route/14 helper manifest entries, accepted compiler inputs versus b32c596 and unchanged SDK/toolchain; retain reviewed-plan authorization.
- BIN-001 [x] Rebuild actual Rally code, match all 21 peer artifacts per layout and measure source-only/combined Xtensa ELFs. Combined default/relocated text 25,556/25,620, data 988, image 28,120/28,184; no admission/execution claim.
- MEM-001 [x] Inspect existing firmware source/map/ELF and actual heap capability tables; inventory shared D/IRAM, retained/temporary allocations, u16 chunks/u32 consolidated LOAD. Physical free/contiguous/fragmentation availability remains explicitly unmeasured.
- STK-001 [x] Verify 4,096-byte command task, resolved 800-byte module path, loose 2,016 all-module frame sum and 736 selected one-wrapper outer frames. Propose 2,048 module/2,048 firmware-service split, conditional on final analysis and qualification; no observed high-water claim.
- PRO-001 [x] Deliver [numerical proposal/design/ownership/next qualification](RESULTS.md) and exact source/artifact receipts; send findings through mailbox and stop before Stage B. Proposal is not implemented or accepted runtime policy.
