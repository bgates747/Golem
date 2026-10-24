# GOLEM-046 qualification and review

The compiler's large-data program now loads from disk through the generic AGNB
reader. This tranche proves assembly integration on stock VDP and actual native
VDPlet execution; the generated C++ header exposes the same names, while the
subsequent [C++ continuation](cpp/README.md) supplies the reusable reader and a
separate current review profile.

1. `large.golem` contains nine 60,000-byte integer arrays. Its finite `tick` adds
   their final elements to `total`; successive calls produce45 and90. No entry is
   run during container loading. Both compiled backends share the same state and
   array layout.
2. `review.golem` is the complete visible drawing: a white nine-tier pyramid.
   `guest.asm` owns file loading, native admission/binding, numeric observations,
   display setup, explicit calls, cleanup and MOS return. It does not secretly
   add geometry. The scene and source roles are labelled on screen.
3. Compiler output, assembled guest, SDK image and full-size containers live in
   ignored `build/`. The real application includes only the thin generated
   symbols/wrappers and the generic loader/window, not all resident data.
4. `host_checks.py` independently compares initialized state and stock setup
   effects against the preserved embedded form. It also checks relocation,
   malformed packaging before output mutation, module-staging records and real
   assembly/C++ header consumers. Run it using the project `.venv/bin/python`.
5. `qualify.py` builds and runs the owned project profile under
   `.emulator/golem-046`. Use the canonical stock runtime path with `--runtime`;
   `--native` selects the pinned actual-Xtensa Flexe runtime. Native builds require
   the standalone SDK; `--no-build` consumes the already built fixture on Oryx.
   Native `--engine interp` separately checks the interpreter. Never run preparation
   concurrently with another run or while the Author has this profile open.
6. The guest compares observed bytes against expectations before saying it passed.
   Observations cover initial values/status/generation, two calls, cleanup,
   reload, and the same second cycle. Native runs additionally query slot ownership,
   LOAD fresh handles, use the generated binding helper, check actual runtime
   results, confirm UNLOAD and return to MOS. These are test observations, not
   production post-copy verification. A drawing-completion query fences the image.
7. Each launcher verifies the pinned input hashes and deletes previous result and
   failure files, so rerunning produces fresh evidence. Native headless checks
   allow presentation to settle, inspect five image samples, and check execution
   fault counters. Timing records describe emulator test duration, not hardware
   throughput. No board has been flashed.

The final Oryx review uses **Flexe**, already staged and stopped:

```bash
cd ~/Agon/mystuff/golem/.emulator/golem-046
./flexe-agon-emulator
```

Expect a brief load, the labelled white pyramid and a successful test message.
Follow the single onscreen MOS keyboard instruction, close the emulator and report
whether it worked. The agent retrieves/verifies `results.dat`; the Author does
not run `verify_review.py` or open a second terminal.

The Author accepted this review on2026-10-23. Retrieved output matches all95
expected bytes, runtime pins verify and the profile is stopped. Publication
approval remains pending. Implementation, evidence and
profile changes are uncommitted. Use the main task document for remaining work
and the maintained [package contract](../../design/agnb-packages.md) for integration.
