# Integer XOR across portable backends

The maintained compiler now accepts integer `^` on either backend. Signed
bit patterns, typed arrays and checked destinations follow the same rules
through stock buffered commands and native VDPlets. Qualification passes;
the complete Oryx animation review is staged and stopped.

The Author requested commit/push followed by execution. GOLEM-041 cleanup was
published as `cd930ae`, followed by the GOLEM-042 plan as `ecb981f`, before new
implementation. The new XOR implementation remains uncommitted pending human
review and explicit publication approval.

XOR accepts matching uint16/sint16/uint32/sint32 operands, evaluates left to
right and binds below shifts but above comparisons. It combines same-width
patterns without intrinsic overflow; destination Bounds still reject invalid
results with status 32. Stock uses a copied result and bytewise adjustment;
native C uses unsigned patterns. SDK/runtime/firmware and peer code are unchanged.

Maintained contracts and C++ sanitizers pass. 936 focused host model/generated-C
sanitizer cases and two real assembly consumers pass. All 55 no-XOR output files
retain accepted GOLEM-041 bytes; all 58 frozen provenance files remain intact.
Stock default/relocated and actual Xtensa JIT/interpreter default/relocated
qualification pass. Both engines pass the 300-frame XOR animation, all saved
observations, final playfield pixels and actual MOS return.

See [GOLEM-042 results/source/instructions](../tasks/GOLEM-042/README.md) for
package/resource/traffic receipts and limits. The visual fixture names the Golem
source/entry and assembly's drawing/pacing role. No graphical/audible instance
was launched by the agent and no physical target was flashed.
