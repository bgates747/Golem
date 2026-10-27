# Stock perspective compiler support

The maintained compiler now encodes the stock perspective sequence without a
private compiler or raw-command escape. Both supplied mesh sources compile
unchanged. Existing package bytes remain unchanged; host, sanitizer and
assembler checks pass. Wireframe owns actual application execution and human
review. This extension remains uncommitted pending that review and publication
approval.

1. [Plan](../GOLEM-049.md) records scope and authority before implementation.
   [Maintained contract](../../design/stock-matrix.md) describes every supported
   primitive, ownership rule, record format and bound.
2. [example.golem](example.golem) is a complete initialized two-vertex source.
   Its comments give the exact borrowed bytes the host must install. Build with:

   ```bash
   make -C src check
   build/golemc --stock-matrix --emit-load-plan --reserve 1000:1005 \
     docs/tasks/GOLEM-049/example.golem docs/tasks/GOLEM-049/build/example
   ```

3. New support includes3x3 matrices/inversion, literal matrix initialization,
   aligned typed float reads, XYZF32/XY16 transformations, static borrowed-buffer
   byte copies and finite parameterless Sub calls. Host loading/pacing/fences
   remain explicit. No new VDPlet/runtime/firmware implementation is involved.
4. [peer-proof.json](peer-proof.json) freezes the independent unchanged-stock2.16
   primitive proof:93commandbytes, six final conversion cases. The maintained
   test emits that exact sequence. Negative output clamps to zero in this proof;
   the application must stay in its agreed positive-depth/positive-screen domain.
5. [legacy-baseline.json](legacy-baseline.json) records six prior packages,
   including load-plan output. All68files retain exact hashes after extension.
   [cli-evidence.json](cli-evidence.json) records two actual eZ80 assembler
   consumers, relocated ID ownership, corrected3x3/float-record load-plan sizes,
   rejected-output preservation and a2685-vertex source-only compile.
6. The large source uses21chunks/22Subs and290447bootstrapbytes; its largest
   entry is13824bytes. This verifies finite encoding capacity, not speed or
   memory availability in a complete application. Host regression tests include
   26legacy and25new rejection cases. ASan/UBSan passes; ignored build/ holds logs.
7. [peer-source-evidence.json](peer-source-evidence.json) records read-only
   compatibility compilation of wireframe's heavy-tank/Corsair source hashes.
   Their3865/291005byte bootstrap outputs were written only under this task's
   ignored build directory. Actual integration must reserve every other package's
   private IDs as well as the borrowed buffers used by that check.
8. [compiler-pins.json](compiler-pins.json) records the executable/source hashes
   handed off through the canonical mailbox. Reproduce host/CLI checks with
   `.venv/bin/python docs/tasks/GOLEM-049/check_cli.py`. No target or graphical
   emulator was launched by this compiler tranche. Independent consumer results,
   human acceptance, publication approval and physical qualification are separate.

The peer's [independent generated-stream target run](peer-target-evidence.json)
now passes both meshes/six poses:32,580XYcomponents within one logical unit of
the double reference, with raster agreement under its one-pixel boundary
tolerance. Full MOS still stalls on Pi during the large Corsair setup, before
projection executes. The runtime-pair guidance identifies the preserved Oryx
stock profile as the next same-fixture comparison; no new runtime implementation
is requested or performed. Human review/publication remain pending.


## Full MOS consumer checkpoint — supersedes the transport follow-up

Fsim deployed-eight reports successful full applications on Pi and Oryx, both
meshes, all controls/reset/repeat, clean exits and handled missing-setup exits.
The same stock firmware/runtime remains; bounded opaque96-byte acknowledged
installation avoids continuous ingress without modifying the projection stream
or claiming a diagnosed UART repair. Prior orthographic/runtime inputs verify
unchanged; both Oryx initial images match Pi. GOLEM-049/peer-mos-evidence.json
retains source provenance and Oryx observations. Human visual acceptance and
direct publication approval remain pending. No compiler/runtime change follows
from this evidence update.
