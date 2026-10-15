# Interactive Flower — first tranche

Flower can use a normal C++ application for input and readable Golem for its
curve calculations. The original algorithm is audited, the interaction is
specified, and real compile/link probes establish the C++ and native-library
routes. The next implementation slice is floating division, sin/cos and floor;
the complete interactive application is still ahead.

1. Preserve the original formula order and inclusive loop: defaults draw 396
   segments. Keep the original logical scale so radius 512 fits the display.
2. Keep exact parameter strings, with blank Enter retaining each current value.
   Redrawing clears after validation; returning to input preserves the curve.
   Escape restores the saved mode, with clearing explicitly permitted.
3. A generated C header calls named Golem Subs. A real AgonDev C++ consumer
   compiles/links at default and relocated buffer IDs; host C/C++ packing and
   dispatch checks pass under sanitizers.
4. Linking ordinary libm/libgcc makes a 5296-byte math probe that the unchanged
   VDPlet loader accepts. Actual Xtensa math execution, stack and accuracy still
   need qualification. No firmware/math-service ABI change appears necessary
   for this candidate route.
5. Float32's default curve drifts by at most 0.108 model pixels from binary64;
   a 60001-segment case drifts substantially. Long-run precision needs deliberate
   treatment. Stock division also needs further investigation; a rounded
   reciprocal multiply is not automatically the same operation.

Read the [initialized Flower source prototype](flower.golem), [algorithm/precision audit](reference-audit.md),
[interaction contract](interaction.md), and [integration evidence/decisions](integration.md).
The prototype labels its unimplemented operations. The compile-only client is
not a runnable Flower demo. No emulator profile or physical deployment changed.

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
