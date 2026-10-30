# Native compiler RAM diagnosis

The short two-float program exhausted the Agon's compiler working memory while
building resident wrappers. The change removes temporary copies of the generated
wrapper code; it preserves the language, emitted packages, available RAM and
reserved stack. The accepted worker and its original failure are retained for
comparison.

## Cause and change

1. The resident root guard contains 3,930 command bytes. Previously its local
   vector, a by-value argument, a copied map value and an encoded write-command
   temporary could coexist while the installation vector grew from 16 KiB to
   32 KiB. Old and new installation storage also coexist during that growth.
2. The wrapper builder now moves completed code into its retained map and
   serializes its clear/write commands directly into the installation vector.
   It retains the same record ordering and block-size admission check. Moving
   one other completed ordinary-program vector removes a smaller copy.
3. This removes approximately 12 KiB of temporary guard payload at that point
   under the native library adapter's allocation rules. That is an explanation
   of the allocations, not a claim that the whole compiler's measured peak
   falls by exactly that amount. Other stages, fragmentation and allocator
   headers affect the observed peak and whether a large request fits.
4. The wrapper-only production worker changes from 319,489 to 319,663 bytes. Its
   worker grows by 174 bytes; the improvement comes from reducing simultaneously
   live data. Desktop comparison matches all eleven files of the unchanged
   two-float package against the frozen reference. The
   [linker-footprint receipt](compiler-footprint.json) records the current final
   worker's heap capacity and unchanged 16 KiB stack reservation.
5. Actual target tests found another avoidable allocation: response files
   retained an 8,193-byte buffer even for a short command. The response reader
   now checks the file size and allocates only its length plus the terminating
   byte. The 8,192-byte limit and existing quoting/argument-count rules remain.
   [Reader checks](response-evidence.json) cover empty files, quoted/empty
   arguments, exact length/count limits and their rejection boundaries. Actual
   MOS validation is recorded separately. With both improvements, the final
   worker is 319,785 bytes, 296 bytes larger than the accepted baseline, with
   122,430 heap bytes. Its stack reservation remains unchanged.

## Measurement method and limits

1. [heap_probe.cpp](heap_probe.cpp) reads the pinned AgonDev allocator's existing
   free list and heap-break pointer after each `ag::memory` allocation attempt.
   It does not replace the allocator, add allocation headers or retain a
   per-allocation registry. Sampling performs no allocation or I/O.
2. Each snapshot records heap extent, currently allocated bytes including the
   allocator's headers, free-list bytes, unused tail, total free space, the
   largest payload that could fit, the triggering request and compiler stage.
   The report retains the highest sampled allocated-byte count and any failed
   request. The free-list walk checks bounds and ordering before use.
3. These are sampled allocator-wide states at compiler allocation attempts.
   They include other live allocations, such as response-file text, but do not
   observe every libc allocation/free event or measure peak stack use. The
   existing 16 KiB stack reservation is unchanged.
4. [build_probe.py](build_probe.py) builds separate diagnostic workers from the
   frozen baseline and the two changed maintained headers. It verifies frozen
   hashes and never overwrites the accepted or distributed worker. The build
   receipts pin the allocator assembly and probe source. Diagnostic workers
   append `heap.log` in their current directory after normal return/recovery.
5. Diagnostic code and static counters reduce available heap. The baseline
   probe has 121,165 heap bytes versus 122,726 in the accepted worker: 1,561
   bytes of overhead. Its binary grows by 1,494 bytes, and diagnostic BSS by
   67 bytes. Diagnostic observations must be distinguished from the separately
   reproduced uninstrumented failure and the production worker's result.
6. The native response-file path retains an 8,193-byte text allocation while
   compilation runs. Compare diagnostic variants using the same invocation;
   direct command-line and response-file measurements are different workloads.

## Actual target observations

The [paired measurement receipt](memory-measurements.json) summarizes the
[baseline log](probe-baseline-direct.log) and
[wrapper-fixed log](probe-fixed-direct.log). Both use the same direct command
line and frozen original response-file reader. Both diagnostic workers fail
the same 32,768-byte installation-vector request while building stock wrappers:

| At that failed request | Baseline probe | Wrapper-fixed probe |
| --- | ---: | ---: |
| Available heap capacity | 121,165 bytes | 120,991 bytes |
| Live allocated bytes, including headers | 84,422 | 72,442 |
| Free-list bytes | 7,842 | 15,982 |
| Uncommitted tail | 28,901 | 32,567 |
| Total free space | 36,743 | 48,549 |
| Largest payload that can fit | 28,894 | 32,560 |

At this matched allocation attempt, live allocated storage falls by **11,980
bytes**. Total free space alone did not guarantee success: neither diagnostic
worker had one sufficiently large block. The fixed probe misses by 208 bytes;
its uninstrumented counterpart has 1,561 additional heap bytes and passes the
direct compilation separately. The diagnostic overhead therefore matters to
this small program and is not included in the distributed compiler.

The highest sampled live allocations are 86,476 bytes in the failing baseline
and 78,695 bytes in the failing fixed probe. These are peaks of those failed
diagnostic jobs, at different stages. They do not establish the peak of a
successful complete compilation. Both final snapshots show zero allocated heap
bytes after error recovery, so these observations do not show a retained-allocation
leak. Production compile/package parity and execution are the acceptance checks.

The optional [baseline response-file observation](probe-baseline-response.log)
corroborates the response-reader issue: its final snapshot retains 8,199 bytes
until the response wrapper returns, comprising its 8,193-byte text allocation
and the allocator's six-byte header. This separate observation is not substituted
for the matched direct-command pair above.
