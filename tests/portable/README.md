# Portable compiler checks

The normal C++ check target verifies CLI/type/reference behavior and compares
52 non-native-C files with frozen accepted compiler output. It requires no task
implementation, SDK, Python or emulator:

```bash
make -C src check
```

`fixtures/provenance.json` identifies the accepted source commit and hashes.
The expected files are copied immutable compiler artifacts, not imports of a
historical compiler or an executable task script. Update them only for an
explicitly reviewed contract/output change. GOLEM-041 preserves the three
historical native-C fixtures while intentionally changing current native output
to report release errors; the fault suite below covers that change.

Optional integration uses the standalone command/source reference model and
the bounded native lease mock, promoted from the accepted general compiler
qualification at `b32c596`. `numeric.golem` provides initialized generic scalar,
array, conversion, error, branch/loop and short-circuit cases. No Rally game
model, assets or renderer is required.

```bash
.venv/bin/python tests/portable/check_integration.py \
  --compiler build/golemc --sdk ../agon-vdplet --output build/portable-integration
```

This runs 383 cases through the independent buffered model and real generated
C with ASan/UBSan. Add `--relocated` for changed IDs and optionally
`--assembler /path/to/ez80asm` for an actual eZ80 consumer. GCC and the SDK's
public `include/vdplet.h` are required for this optional check; neither a native
toolchain nor a firmware build is performed. Generated files and receipts go
only to the requested output directory. These are host checks, not emulated or
physical target execution.

The focused release suite injects callback failures into actual generated C:

```bash
.venv/bin/python tests/portable/check_release_errors.py \
  --compiler build/golemc --sdk ../agon-vdplet --output build/portable-release
```

It checks 80 cases per layout under ASan/UBSan: root/argument cleanup order and
first-error precedence, acquired-view exits, temporary-array release failures,
partial effects and generation/runtime-result gating. Failed callbacks either
release the view or refuse release; root/argument memory is poisoned after each
release attempt to detect subsequent access. Add `--relocated` for changed IDs.
No SDK implementation or historical task compiler is imported.

Focused integer-XOR checks use initialized `xor.golem`:

```bash
.venv/bin/python tests/portable/check_xor.py \
  --compiler build/golemc --sdk ../agon-vdplet --output build/portable-xor
```

468 cases compare the independent command machine/reference and sanitized
generated C: all four integer types, sign bits/extrema/alternating patterns,
precedence, arrays, Bounds, conversions, operand failure order and skipped
Boolean branches. The suite includes explicit expected syntax results rather
than relying solely on the serialized tree. Add `--relocated` and optionally
`--assembler /path/to/ez80asm`. This is host qualification; GOLEM-042 records
actual stock/Xtensa and human review separately.
