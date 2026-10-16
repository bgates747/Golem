# Interactive Flower — application ready for review

The [ordinary C++ application](app/README.md) now runs the complete Flower
calculation and resident renderer under the emulator. It asks for seven values,
keeps their exact accepted strings, preserves the old curve during re-entry,
and restores the original mode on Escape. Seventeen Oryx headless scenarios
pass; the dedicated interactive profile is staged and stopped for the Author.
New client/profile work remains uncommitted for review.

Read [flower.golem](flower.golem) for the actual mathematics and
[app/src/main.cpp](app/src/main.cpp) for input/transport. The accepted
[native math](native-math/README.md) and [resident renderer](renderer/README.md)
reports establish their earlier qualification. Stock math remains unsupported.
The first feasibility findings below establish the reference/input/C++ contracts.

1. Preserve the original formula order and inclusive loop: defaults draw 396
   segments. Keep the original logical scale so radius 512 fits the display.
2. Keep exact parameter strings, with blank Enter retaining each current value.
   Redrawing clears after validation; returning to input preserves the curve.
   Escape restores the saved mode, with clearing explicitly permitted.
3. A generated C header calls named Golem Subs. A real AgonDev C++ consumer
   compiles/links at default and relocated buffer IDs; host C/C++ packing and
   dispatch checks pass under sanitizers.
4. Linking ordinary libm/libgcc makes a 5296-byte math probe that the unchanged
   VDPlet loader accepts. The subsequent native tranche qualifies generated math
   execution, stack and accuracy; no firmware/math-service ABI change was needed.
5. Float32's default curve drifts by at most 0.108 model pixels from binary64;
   a 60001-segment case drifts substantially. Long-run precision needs deliberate
   treatment. Stock division also needs further investigation; a rounded
   reciprocal multiply is not automatically the same operation.

Read the [initialized Flower source prototype](flower.golem), [algorithm/precision audit](reference-audit.md),
[interaction contract](interaction.md), and [integration evidence/decisions](integration.md).
The Flower math source now compiles with explicit --target native. The
earlier compile-only C++ interface client remains a separate proof; app/ now
contains the actual application. A dedicated Oryx review profile is staged, with
no graphical launch or physical deployment.

Reproduce this tranche from the repository root:

```bash
.venv/bin/python docs/tasks/GOLEM-044/reference.py
.venv/bin/python docs/tasks/GOLEM-044/check_abi.py
.venv/bin/python docs/tasks/GOLEM-044/probe_math.py
```

The ABI probe needs `build/golemc` and installed AgonDev under `/Agon/agondev/release`;
the math probe needs the pinned Xtensa compiler and sibling agon-vdplet SDK.
All generated build products stay ignored under this task. Source identities
and reproducible JSON receipts are in [evidence](evidence/).


### Acceptance and publication — 2026-10-15

The Author accepted the mode 19 white-on-black review and explicitly requested
commit and push of all pending work. REV-001 is satisfied. Retrieval of the
saved human log found no controlled Escape exit, so no separate human exit or
MOS keyboard pass is claimed; the 17 passing headless scenarios retain the
independent lifecycle evidence. Earlier pending-publication statements above
are historical and superseded by this approval. Controlled fault injection
and physical qualification remain open.
