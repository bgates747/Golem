# Maintained integration and validation sequence

The next useful implementation tranche is an additive portable compiler mode
containing the accepted general implementation. This keeps its working parser,
emitters and root interface together, delivers something usable immediately,
and avoids rebuilding the old sequence of task forks. Existing raw/resident
behavior remains available. This is a proposed implementation boundary; the
released tranche so far performed only the source/evidence audit.

## Proposed first implementation tranche

1. Promote the four pinned generic files together with their existing behavior.
   Rewrite local include paths and adapt the driver entry point; do not redesign
   the AST, root rewriting mechanism or emitters during this move.
2. Dispatch explicit `--portable` from `golemc` before the existing raw/resident
   argument handling. Convert its named options to the same validated compiler
   inputs. Keep the new mode's initialized typed grammar separate from legacy
   profile grammar.
3. Emit the same shared storage/root/array/assembly and dual-backend artifacts
   as the accepted general compiler. Native output is C source at this stage;
   the SDK's optional packaging step is not part of `make -C src`.
4. Adapt useful grammar/reference/emission checks into maintained tests. Freeze
   small generic fixture inputs/expected artifacts from the accepted baseline
   with provenance, rather than including task code or requiring old ignored
   build directories. Update README/maintained contract and handoff together.
5. Complete focused host/sanitizer/generated-C/parity/assembler checks. If
   executable bytes/layouts change, or a new native package is deployed, perform
   the corresponding focused target qualification and fully stage any required
   human review before returning. Avoid emulator work where unchanged emitted
   bytes plus existing target evidence establish the relocation-only change.

This moves shared scalar syntax/types/layout first **as part of its actual
cumulative implementation**, rather than deleting later accepted cases from
that implementation. Root layout depends on the backend layer and both emitters
consume the common AST; artificial feature removal would add refactoring risk.

## Exact proposed source boundary

| Existing accepted source | Proposed maintained destination | Change in first implementation |
| --- | --- | --- |
| `docs/tasks/GOLEM-036/right-shift/portable.hpp` | `src/portable/portable.hpp` | Copy with provenance; include `../token_reader.hpp` and `../buffer_allocator.hpp`. Keep parser, typed model, layout/reference/JSON helpers intact initially. |
| `docs/tasks/GOLEM-036/right-shift/backends.hpp` | `src/portable/backends.hpp` | Copy with provenance; include `../ordinary.hpp`; retain stock/native emitters and byte helpers. |
| `docs/tasks/GOLEM-036/right-shift/root.hpp` | `src/portable/root.hpp` | Copy with provenance and local includes; preserve guarded generated-C transformations and root-v1 fields. |
| `docs/tasks/GOLEM-036/right-shift/compiler.cpp` | `src/portable/driver.hpp` | Adapt `main` to a callable CLI function in `golem::portable_cli`; keep generation pipeline, output filenames, wrappers and metadata. Replace scratch positional prefix with named options. |
| Existing `src/golemc.cpp` | Same path | Add explicit mode dispatch; leave legacy dispatch/body intact. |
| Existing `src/Makefile` | Same path | Track new headers and add maintained portable checks; host compilation must work without SDK/PlatformIO/emulator. |
| Existing maintained helpers | Same paths | Reuse `token_reader.hpp`, `buffer_allocator.hpp`, `ordinary.hpp` unchanged. Do not extract/rewrite byte helpers merely to make names prettier. |

Keep historical tasks and their source identities unchanged. No renderer helper,
native-binding config parser, firmware loader, Rally fixture, profile or emulator
adapter moves into `src/portable/` in this tranche.

## Proposed command and package contract

```text
golemc --portable [--reserve FIRST:LAST ...]
                 [--bind ROLE ID OFFSET BYTES ...]
                 [--array-data NAME=PATH ...]
                 SOURCE OUTPUT_DIRECTORY
```

1. No new `--target` filtering in this first integration: emit both executable
   descriptions exactly as the selected general compiler does today. Stock-only
   users can install only buffered outputs; native users can package/install
   native C using the SDK. No runtime auto-detection or firmware flashing occurs.
2. With no extra reservations/bindings, preserve baseline allocation and its
   implicit 60000–60031 reservation. Document that compatibility rule. Tests
   should cover an explicit `--reserve 1:254` equivalent to the historical
   prefix argument, plus disjoint reservations and collisions. Removing the
   implicit range later requires a deliberate policy/fixture update.
3. `--bind` retains the four optional roles and access/layout rules from root-v1.
   Absent bindings remain unbound; no Rally asset IDs, filenames or default
   game resources are added. Use strict numeric parsing, range checks and clear
   duplicate/collision errors for the new public option surface.
4. `--array-data` maps build-time packed initial bytes, with exact lengths and
   unknown/duplicate/conflicting initializer rejection. The program does not
   load those files at runtime.
5. Retain filenames: `root.json`, `interface.json`, `program.json`, `state.bin`,
   `layout.inc`, `common-install.bin`, optional `array-*.bin`/`arrays-install.bin`,
   buffered/native install/cleanup, `backends.json`, `calls.inc` and `native.c`.
   Preserve root schema, type/status tags, logical entry order and wrapper
   names. The general native handle patch offset remains its generated value
   24; the separate renderer profile's 37 is not a replacement universal constant.
6. External SDK location/setup belongs to optional native packaging/checks.
   Pin public ABI and build settings in those receipts. Historical `.vdpl`
   envelopes used 8/4/0.5/16-KiB policy defaults; the current SDK uses different
   defaults. State which settings are used and do not call differing envelopes
   a compiler-byte regression or silently rewrite historical pins.

## Regression and evidence mapping

| Check | Reusable source/evidence | Maintained treatment |
| --- | --- | --- |
| Legacy behavior | `tests/test_compiler.cpp`; prior raw/scalar/table/drawing byte contracts | Keep existing tests; add mode-dispatch/argument failures and ensure legacy mode outputs remain unchanged. |
| Grammar/types/reference | GOLEM-032 `syntax_checks.cpp`; GOLEM-035 frontend cases; GOLEM-036 foundation/multiply/division/shift cases | Adapt representative positive/rejection/evaluation-order cases into `tests/test_portable.cpp`, using maintained headers only. Do not import task builders. |
| Root/layout/allocation/assembly | GOLEM-034 root contract; latest `root.hpp`; latest default/relocated deliveries and frontend assembler consumer | Maintain explicit schema/offset/tag/ownership checks and emitted fixture parity. Configure assembler location instead of requiring GOLEM-007's ignored `build/ez80asm`. |
| Buffered command semantics | Latest `model.py` and stage-specific numeric/array vectors | Promote the standalone model and generic cases under `tests/portable/` as needed. It imports only standard Python modules. Avoid Rally world/render models. |
| Generated native C | Latest `native_host.c` and bounded host vectors; float suite in GOLEM-032 | Promote reusable lease mock/fixtures to `tests/portable/`; obtain the public header from an explicitly configured SDK for optional native-host checks. Keep contraction disabled and use ASan/UBSan where relevant. |
| Source/output parity | Latest division/right-shift prior-package comparisons and three accepted right-shift delivery manifests | Copy small selected inputs/expected bytes into maintained fixtures with source/commit/hash provenance. Compare generated C/layout/commands, not host-path-bearing packaging receipts. Do not require task directories at test runtime. |
| External packaging | SDK tools and prior loader/ELF receipts | Optional integration target, explicit SDK/version/settings. Do not copy the old `build_delivery.py` pin checker, mutable validator binary dependency or historical stack assumption into the compiler. |
| Target behavior | Existing stock and Xtensa JIT/interpreter default/relocated/human receipts | Evidence for unchanged emitted bytes; run focused fresh qualification only for changed behavior/layout/package or unresolved concerns. Preserve accepted profiles and force silent/windowless automated launches. |

Earlier feature suites sometimes use an older two-byte-root layout. They cannot
all be treated as byte-identical golden output for the newer 112-byte-root
compiler. Adapt their **semantic vectors** to the selected root contract;
compare package bytes against a baseline generated by the selected successor.
No old fixture, manifest or saved result should be edited in place.

Normal `make -C src check` should run host C++ grammar/model/layout/CLI contracts
without a network, SDK build or emulator. Optional Python/native-host/assembler
integration checks must document their explicit prerequisites and stay finite.
The promotion acceptance still runs whichever of those checks are relevant;
optional installation for downstream users does not excuse skipping validation.

## Later slices and limits

1. After the coherent promotion/parity checkpoint, address a concrete generic
   release-result propagation gap separately. Carry the tested idea from renderer
   integration only after defining generic exit semantics and negative controls;
   do not silently change generated C as part of a move-only parity claim.
2. Consider target-output filtering and a generic native-binding facility only
   under a separate contract. Portable storage/backend selection must not depend
   on Rally's hooks or resource IDs. Applications continue to own call selection.
3. Reconcile remaining maintained documentation/examples and task statuses with
   the promoted feature matrix. A user should not need to read a task log to
   discover supported syntax. Keep proposed functions/objects/math/drawing
   unimplemented unless separately released.
4. Avoid compiler-internal cleanup, namespace redesign, optimizer work or merging
   the distinct floor/table/drawing languages into the portable parser without
   a concrete need. Further source splitting can follow actual maintenance use.

## Acceptance for the proposed first implementation

The documented additive mode builds through `src/`, emits the selected generic
packages and passes adapted maintained contracts/parity checks without task-code
dependencies. The compiler builds and normal host checks work without native
toolchain installation. Existing raw/resident behavior remains intact. Any new
public CLI behavior is tested, native prerequisites are explicit, and any material
qualification limit is reported. No generic SDK, firmware, Rally source or
emulator fix is part of this implementation proposal.
