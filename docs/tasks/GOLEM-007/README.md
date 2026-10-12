# GOLEM-007 — Stock emulator resident qualification

The integrated resident scalar job passes **855 checks across 150 calls** on
official Fab 1.2.5's stock VDP through MOS 3.0.2 and the emulated eZ80 UART.
The guest returned 9,204 bytes matching independent expectations. Numeric and
ownership/lifecycle behavior are qualified for this emulator build. The Author
completed the Oryx review and confirmed MOS keyboard response. The Author explicitly approved committing this tranche and proceeding with
production promotion.

## What passed

| Coverage | Observation |
|---|---|
| Four installations | Pure floor and mixed uint16/floor, each with default and relocated IDs |
| 132 fresh-input jobs | 33 binary32 values per layout, including signed limits, adjacent fractional values, signed zeros, smallest subnormals and tiny negatives |
| Repeated calls | Invoke without input update retains input and advances ordinary state |
| Persistent uint16 globals | `calls` and `previous` agree after every job; explicitly seeded 65534 advances through 65535, 0, 1 |
| Relocation | Reservations 1..254 and 260..264 force real buffer operands across 255/256; default and relocated numeric/state results agree |
| Program contents | Full entry/helper bytes match installation before and after jobs and after live reinstallation |
| Reinstallation | Both live reinstall and reinstall after cleanup reset globals and restore the declared tiny-negative input; the first invocation produces -1 |
| Cleanup | Every owned ID is readable before cleanup and fails a controlled copy afterward, twice per layout |
| External resources | Root buffer 0, external canaries and the diagnostic byte asset survive owned cleanup |
| Arbitrary bytes | All 256 values round-trip once initially and after each layout's cleanup |

No scalar calculations run on the eZ80. [guest.asm](guest.asm) is a generic
transport for a host-generated tape containing raw VDU sends and byte-read
requests. It saves observations through MOS; [qualify.py](qualify.py) checks
them against binary32-rounded `math.floor` and exact expected state/program
bytes. It does not import the host execution model. Each GP response follows
the job on the same UART stream, establishing parser progress after a finite
call. This is not raster-completion or physical ESP32 evidence.

For cleanup probes only, after all numeric checks, each owned data buffer's
first byte is set to `0xA6`. Program buffers retain their known first byte.
The query packet preloads `0x55` before fetching a byte, so a missing source
leaves that tag. This controlled before/after check avoids treating a legitimate
`0x55` result as evidence of absence. It checks readable resources, not the
firmware's entire heap or metadata balance.

## Evidence and identity

[emulator-checks.json](emulator-checks.json) contains every check's observation
span, manifests, source/payload/runtime hashes, execution command and host
monotonic setup/execution durations. [observed.dat](observed.dat) and
[expected.dat](expected.dat) preserve the complete raw evidence. The runner
also observes `/proc` to verify which native Fab executable and VDP module
actually ran, then checks runtime/source hashes again after execution.

[provenance.json](provenance.json) identifies official Fab tag 1.2.5 at
`c22876d1f7ad903537c308cf65afd6553b37cdb8`, its submodules, and the assembler.
The stock emulator's bundled VDP is
`1056ee38ec1cc68ed327bd32ee47d04b1c30597c`; this is distinct from the canonical
ESP32 source revision consulted by GOLEM-006's host model. No firmware source
or upstream checkout was changed. The native build is ARM64 because the Pi's
inherited reference binaries were x86-64.

The final automated run took about 23 seconds, including native process
identity observations and result retrieval. Earlier comparable runs without
that observation cost took about 12 seconds. These host wall times are not
VDP execution benchmarks. Initial runtime/assembler build time was not measured.

## Reproduce

From `/Agon/mystuff/golem`, use the project Python environment:

```sh
.venv/bin/python docs/tasks/GOLEM-007/qualify.py --smoke
.venv/bin/python docs/tasks/GOLEM-007/qualify.py
```

The native runtime and assembler must already exist at
`.emulator/runtime/fab-1.2.5/` and `docs/tasks/GOLEM-007/build/ez80asm`.
The runner builds the existing GOLEM-006 CLI, generates packages/tape, assembles
the generic guest, and calls the canonical setup tool's `link_shared_runtime`
to generate the project `.emulator/` profile. Only the generated probe binary
is linked into `/golem`; the containing project is never mapped into its nested
SD tree. Startup uses CRLF, tagged MOS and the generated profile-local wrapper.
Fresh runs remove the previous guest results file. Run only one profile instance
at a time and close the review window before rerunning qualification.

For a clean native runtime, clone the read-only local official Fab checkout
into `.emulator/runtime/fab-1.2.5`, detach at tag `1.2.5`, initialize its four
submodules at the recorded revisions, then use its top-level `make`. The
submodule URLs can be directed to the corresponding local reference checkouts
with repository-local Git configuration. Keep all build products in the owned
runtime; do not rebuild the reference checkout. To build the assembler, copy
`/Agon/agon-ez80asm/src` into this silo's `build/assembler/src` and use the
exact GCC command in provenance.json, from this task directory.

Build outputs, native runtime, profile and SD contents are ignored. The report
and source files stay in this task silo. The compiler and scalar emitter did
not change during qualification.

## Graphical review

Human reviews default to **Oryx Linux Lite**, with a dedicated local Golem
instance at `/home/smith/Agon/mystuff/golem/.emulator`. The Pi has no accessible
desktop and is used only for automated headless qualification. Agents prepare
the Oryx fixture but do not launch graphical instances for the Author.

The Oryx runtime, profile, test binary and SD tree are local files. The canonical
setup tool generated its launcher and pinned MOS identity. Its x86-64 runtime
artifacts came from the existing official reference checkout; all shared
libraries resolve on Oryx. Guest, expected-data and verifier hashes match the Pi
fixture. [oryx-review.json](oryx-review.json) records preparation and the later
manual review: saved observations match all 855 checks, and the Author confirmed
that `dir` responds correctly at the emulator's MOS prompt.

Run this **in a Bash terminal on Oryx**:

```bash
cd ~/Agon/mystuff/golem/.emulator
rm -f sdcard/golem/results.dat
SDL_VIDEODRIVER=x11 ./fab-agon-emulator --renderer sw
```

The prepared profile runs the same bounded fixture, prints
`GOLEM-007: transport complete; host verifies results.dat`, and returns to MOS.
The message means transport completion; the host report supplies the numeric
assertions. Leave the emulator window open. In a Bash terminal, verify the fresh
results:

```bash
cd ~/Agon/mystuff/golem
.venv/bin/python docs/tasks/GOLEM-007/verify_review.py
```

Expected: `Review output matches: 855 checks, 9204 bytes.` Then **in the emulator
window**, type `dir` at the MOS prompt and report to the agent that it works.
Close the emulator afterward. Report any different verifier message, missing
completion/prompt or failed keyboard response. Removing the old
result before each launch prevents a failed run from reusing stale observations.

The canonical environment AGENTS.md requires explicit human testing and
approval before committing emulator-related changes. Automated success does
not dispose of that requirement. The Author completed human validation and explicitly approved committing this tranche.

## Serial debug observation

The Author authorized using serial debug output for observations. In this stock
build, `force_debug_log` still calls `DBGSerial.print`, while the userspace
`HardwareSerial::print(const char *)` is empty. Its separate byte `write` path
does forward DBGSerial bytes to stdout. Thus buffered command `0x80` dumps
remain silent; source paths and commits are recorded in provenance.json.

GP echo provides everything needed for this qualification, so no emulator patch
or request to Tomm was necessary. If direct dumps become useful later, forwarding
`print` through the existing DBGSerial write path would address this specific
gap while preserving the MOS UART queue. That change was not attempted here.

## Wire traffic

The application contract remains **1,120 installation bytes and 21 bytes per
fresh-input call** for counted-floor, excluding result readback. The probe's
tagged-nibble diagnostics alone send 102 VDU bytes per observed byte; they are
test traffic. Reported tape bytes also include canaries, setup, reinstalls and
cleanup, and exclude boot/guest completion text. They are not a measured
application traffic total or a performance claim.
