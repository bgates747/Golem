# Native Agon compiler contract

The eZ80 compiler uses the current desktop Golem parsers and emitters through an
AgonDev platform adapter. It supports all implemented profiles, including C
source emission, without changing their language or output bytes. The earlier
subset contract is superseded; its exploratory implementation is retained only
under `exploratory-subset/`.

## Language and outputs

The maintained [language reference](../../language-reference.md) identifies the
implemented raw, resident scalar/table/drawing, portable and stock-matrix
profiles. Their existing restrictions also apply on Agon. Proposed Functions,
general Do/Loop and other unimplemented constructs are not added by this port.
Each profile's normal payload/package files must match the current desktop
compiler byte for byte, including metadata and generated C/assembly. Native C
emission does not provide a C-to-Xtensa compiler on the eZ80.

The raw worker additionally writes `<payload>.cleanup.bin`, clearing its private
buffer IDs after a standalone run. The original raw payload is unchanged.
Packages use the existing layouts and load-plan format; no new binary envelope
or GOB format is introduced. Input/output errors can leave partial output files,
as in the desktop tool. Use a fresh output directory when switching targets.

## Implementation and memory

`port/prepare.py` snapshots and hashes the maintained C++ sources before each
build. It selects the existing CLI branch for one of four workers; it does not
replace a parser or create another source grammar. The adapter supplies only
the C++ library operations these sources use. Explicit recovery replaces the
known C++ exception sites because the installed AgonDev backend cannot emit
C++ exception unwinding. Stack-owned adapter objects register cleanup actions.

The AgonDev ABI uses 24-bit int and 32-bit double. Descriptor offsets/lengths
remain explicitly 32-bit; compile-time binary64 calculations use SoftFloat and
exact decimal conversion. Adjacent binary32 values are calculated locally,
avoiding the installed SDK nextafterf bug. Float32 language operations retain
their existing rounding. Output metadata is spooled to `/golem/cache/backend.tmp` rather than
retained as one large string. Compilation is single-process, not concurrent;
create that directory when installing the compiler.

Workers load at 0x40000 and occupy the main application range below 0xB0000.
Each reserves 16 KiB for its stack; remaining memory holds compiler working
state. The small dispatcher runs as a MOSlet at 0xB0000 and loads one worker at
a time. This leaves more RAM than loading all four backends together. Actual
binary sizes, successful workloads and test results are in the evidence report.
As with any Agon application, a sufficiently large source can exhaust RAM;
errors must not silently change types, truncate wide values or weaken checks.

Normal CLI flags are retained. `@file` reads up to 8192 bytes and 127 arguments;
double quotes group a response-file argument. Ordinary MOS command-line parsing
has the SDK's smaller argument limit. Optional dispatcher `--batch list` is a qualification convenience: each line
names a response file and per-job results go to batch.log. The dispatcher loads
a fresh worker for each job, matching normal command use. Repeated jobs in one
worker process are an internal stress path and can fragment its finite heap.

## Standalone runner and ownership

`golemrun payload.bin`, `golemrun --resident directory` and
`golemrun --portable directory` execute trusted compiler output. Resident and
portable runs invoke their first Sub. The runner is not a Golem interpreter or
a native-C executor. Applications needing persistent state, alternate entries,
borrowed buffers or VDPlet selection use the published layouts/wrappers instead.

The runner uses reserved buffer 0 as its finite transport stream. It sends
complete chunks of at most 96 bytes with ordered GP echoes, consolidates and
calls that stream, and waits for completion before cleanup. A separate stock
pixel query fences drawing. GP is a completion mechanism, not a per-copy byte
verification mechanism. The runner establishes absolute pixel coordinates and
origin without changing screen mode. It preserves the resulting picture.

Raw sidecars and resident cleanup clear owned buffers. Portable cleanup first
clears backend-private resources, then the installed common state/arrays. The
runner never manufactures bitmap assets or borrowed vertex data. It is the
caller's responsibility to reserve IDs and install those resources. A timeout
leaves possibly live resources in place and requests reset before reuse.
The loader is not a sandbox for malicious command streams.

## Qualification and delivery

Qualification compares the same accepted and rejected sources against the
current desktop compiler, the adapted host build with sanitizers, and actual
eZ80 executables running under MOS on a stock-VDP Pi headless emulator.
Standalone examples additionally exercise the actual dispatcher, generated
packages, runner, numerical results and MOS keyboard continuation. Automated
results do not claim physical-hardware or human visual acceptance.

The ZIP contains native binaries, initialized examples, this contract, a how-to,
a language reference, detailed profile references, source/build recipe, required
third-party notices and measured evidence. Everyday compilation needs only the
Agon and SD card. Rebuilding the compiler binaries uses AgonDev on a development
computer. No firmware is flashed and powered-off hosts are not contacted.
