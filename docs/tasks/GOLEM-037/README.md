# GOLEM-037 — First resource gate

The accepted renderer and a tiny generated Golem workload exceed the current
code and declared stack budgets. The final single-entry measurement uses
**8,224 text bytes against 8,192**, before required D2 marshaling/publication.
Even the helper with a minimal dispatcher has a **544-byte module call path
against 512**, excluding firmware/service frames. The released first gate is
complete; bindings and emulator staging remain pending. The Author subsequently
reaffirmed increasing limits; that choice is settled. [GOLEM-002 capacity research](../GOLEM-002.md#further-investigation--substantial-single-module-applications)
records the substantial-module stress protocol and numerical budget investigation.
No SDK/firmware limits, Rally sources, production compiler or emulator profiles
were changed. Nothing was deployed, committed or pushed.

## Measured builds

Actual Xtensa GCC 8.4.0 / esp-2021r2-patch5, unchanged SDK `-Os`/PIC/windowed ABI
flags, no libc, no LTO and one public `vdplet_entry` were used. The helper's four
build inputs are exact copies from Rally's accepted 14-file manifest, SHA256
`3fc5d32f16d84f8ecc53aabe2d418e317a5323aee1ea2543c85fd657b1d6f62c`.
All 14 entries were checked before copying; [helper-pin.json](helper-pin.json)
records the source, SDK and Golem inputs. Rally retains source ownership.

| Resource | Current limit | Helper + minimal dispatcher | Tiny Golem + helper |
| --- | ---: | ---: | ---: |
| Resident text, rounded to 4 bytes | 8,192 | 7,600 | 8,224 |
| Resident rodata/data/BSS, rounded by section | 4,096 | 864 | 1,132 |
| Image including 64-byte envelope | 16,384 | 9,796 | 10,776 (would-be image) |
| Measured module stack path, excluding callbacks | 512 | 544 | 576 |
| Relative relocations | Format checked | 36 | 42 |

The SDK builder packages the minimal helper build and its image validator passes
structure/relocations. This does **not** qualify its stack or execution. The
builder rejects the combined build with `module exceeds admitted text/data
envelope`; there is no combined `.vdpl`. Its diagnostic ELF remains in ignored
`build/`. No unsupported PLT relocations remain in either final build.

The [tiny Golem source](fit_source.golem) declares one initialized uint32 and
three named Subs: reset, checked increment, reset. It is compiled by the accepted
GOLEM-036/right-shift compiler. [fit_combined.c](fit_combined.c) sequences that
generated dispatcher and the helper only to measure their linked resources.
Internal entry visibility is mechanically changed to hidden so only the combined
SDK entry is public. The helper also needs a small private `memcpy`: GCC emits
that call for native structure copies, and the SDK links without libc.

This measurement **does not implement D2**: no source-field marshaling, retained
completion publication/clear, early argument release or success/error integration.
Golem returns before helper execution and its existing success suffix therefore
runs too early for the accepted contract. Its existing argument/root leases are
released before this measurement calls the helper; there is no retained root
lease during rendering. Static typed inputs minimize wrapper
stack for measurement. These files must not be deployed as an application. The
full Rally source and real glue will have different resources; these figures are
observations of the pinned builds, not universal lower bounds or hardware ceilings.

## Stack evidence

The actual linked path is:

`vdplet_entry (32) → rally_render_prepare_v1 (352) → preflight (128) → acquire (32)`

That totals 544 before entering the buffer-service callback. The measurement-only
combined dispatcher adds another 32-byte live frame, totaling 576. These are
compiler static frame sizes on a resolved call chain, not the sum of unrelated
functions. [fit.json](evidence/fit.json) records the exact instructions/literal
targets; full ELF inspection/disassembly is retained beside it. No target runtime
high-water measurement is claimed. The package's declared 512-byte stack field
is an admission policy, not a runtime sandbox or a measurement.

## Reproduce

From the repository root, with the pinned SDK/toolchain available:

```bash
.venv/bin/python docs/tasks/GOLEM-037/measure_fit.py
```

The script checks frozen sources, rebuilds the existing compiler and both native
measurement builds, verifies the call paths/relocation form, validates the small
image and records the expected combined admission rejection. Build products stay
in `build/`; source hashes, generated C/layout, measurements and diagnostic
disassembly stay in `evidence/`. No emulator is involved. The final build/receipts
were repeated with identical hashes.

## Resource decision and further investigation

The Author reaffirmed that increasing limits was already approved. Do not ask
again whether to increase limits or reduce the renderer. GOLEM-002 records the
broader capacity/service/stress investigation; choose justified numerical budgets
before finishing real bindings and target qualification. This update is planning
only, with no new execution or runtime change. Increasing text alone does
not resolve stack use, and shaving 32 text bytes would still leave no useful
allowance for the omitted glue or Rally logic. No exact replacement budgets are
proposed from this tiny fixture alone.

Visual staging remains pending because the released plan explicitly returns a
measured resource failure for review. No valid combined visual program exists yet.

Rally was notified through canonical mailbox message
`2026-10-13_21-04-47_golem_rally25-d2-resource-gate-sixteen`, with the exact final
evidence hash and explicit limits/ownership boundary. No peer changes were requested.

Rally's reply `2026-10-13_21-22-33_rally_rally25-route-kernels-qualified-twenty-four`
confirms reading that report and the shift publication. Golem read/acknowledged
the reply, verified all 20 files in the route-kernels manifest and read its
README/results. Rally reports 319,072 generated-C host invocations passing; Golem
did not rerun peer game checks or claim independent target qualification. The
peer receipt predates the Author's reaffirmation above. See [receipt](rally-route-kernels-receipt.json)
and the shared coordination index's current exchange status.

Rally's subsequent budget questions (twenty-five) were read/acknowledged and
answered in `2026-10-13_22-10-12_golem_rally25-budget-direction-protocol-seventeen`:
the agreed increase addresses code and stack, numerical replacements are not
selected, and implementation/current policy/admission evidence remain distinct.
[Unchanged SDK/tool identities](budget-baseline.json) and the planned protocol/
qualification sequence were supplied. No new runtime pin is offered.
