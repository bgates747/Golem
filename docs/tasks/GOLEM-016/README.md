# GOLEM-016 — Named assembly calls and generated includes

Resident compilation now supports optional `--emit-asm` output. It emits golem.inc
with named VDP storage locations, the job's eZ80 call wrapper, and labeled command
payloads. Real ez80asm consumers verify ten normal/relocated packages against the
previous compiler: all five existing binary/manifest artifacts are unchanged.
This is host/assembler qualification; no new emulator run was performed.

## Build and include

From the Golem checkout:

```bash
make -C src
build/golemc --resident --emit-asm docs/tasks/GOLEM-016/named-call.golem docs/tasks/GOLEM-016/build/demo
```

[The Golem example](named-call.golem) declares Sub tick. Its generated include
exports `golem_tick` as an actual eZ80 ADL routine, so application assembly uses:

```asm
    call golem_tick
```

That wrapper loads HL with its six-byte VDP call packet, BC with its length,
executes rst.lil $18 and returns. It does not install buffers, update arguments,
read results or wait for a VDP completion acknowledgement. Its return means the
command was transmitted. Assembly controls the application lifecycle and pacing;
VDP commands retain normal ordered-stream execution.

[consumer.asm](consumer.asm) is an assembled MOS application example that sends
the bootstrap, calls the job by name and sends cleanup. Its include is placed
after the program return. It is provided as an integration example, not staged
for emulator launch. A real application consumes any needed results before
cleanup, which removes the installed result storage.

## Published names

| Symbol | Meaning |
| --- | --- |
| golem_tick | eZ80 ADL wrapper for the source job tick |
| golem_tick_buffer | Assigned VDP entry buffer ID |
| golem_tick_call / golem_tick_call_len | Six-byte VDP call packet and its length |
| golem_calls_buffer / _offset / _bytes / _type | Persistent calls global's VDP location, width and type |
| golem_x_buffer / _offset / _bytes / _type | Bounded float input location, width and type |
| golem_result_buffer / _offset / _bytes / _type | Signed output location, width and type |
| golem_runtime_error_buffer / _offset / _bytes / _type | Optional checked-arithmetic status location; omitted when absent |
| golem_package_bootstrap / _len | Complete installation command stream |
| golem_package_update_call / _len | CLI example-input update plus call stream |
| golem_package_cleanup / _len | Owned-buffer cleanup command stream |
| golem_package_owned_count | Number of owned VDP buffers |

Type tags are 1=uint16, 2=sint16, 3=float32. IDs/offsets describe VDP storage;
they are not eZ80 RAM addresses. interface.json continues to publish bounds,
ownership and lifecycle metadata. Unsupported Table/Array/Record layouts are
not exported by this current scalar profile.

The wrapper changes HL/BC; remaining register/flag effects follow MOS RST 18h.
Callers must use ADL=1 and preserve any state their calling convention needs.
The packet length is always six bytes; larger application-selected streams must
be sent using an appropriate bounded transport. Generated includes are designed
for one resident package per application at present. Case-folded label collisions
and ez80asm names over 64 characters reject compilation before file publication.
These restrictions apply only when requesting assembly output.

The flag is explicit and may appear among --reserve options before the source.
Without it, the original five-file package remains unchanged. With it, golem.inc
is an additional sixth file; existing files retain exact bytes. Compilation still
does not install anything on hardware. Source filenames/asset packaging stay in
the build; assembly owns file loading and VDP transfer.

## Qualification

[Host checks](host-checks.json) record source/assembler identities and timings.
The runner compiles the accepted e52a427 baseline and compares pure-floor,
counted-floor, subtraction, multiplication and divmod packages under normal and
relocated allocation. Every current five-file artifact matches. An actual
assembler consumer verifies emitted payload bytes, exported locations/types,
owned count and exact ADL wrapper instructions. The named-call MOS example also
assembles successfully. Semantic symbol errors preserve existing output files;
duplicate flags and raw-mode misuse are rejected. Maintained checks and
ASan/UBSan pass with 46 rejection cases.

Reproduce:

```bash
.venv/bin/python docs/tasks/GOLEM-016/run_checks.py
```

No numerical backend or VDP command byte changed. The existing stock runtime
qualification still covers those bytes; the new eZ80 wrapper has been assembled
and checked, not newly executed in an emulator. Changes await Author review.
The next recommended implementation is a table-only finite resident job, removing
the current mandatory float-floor requirement while retaining scalar compatibility.
