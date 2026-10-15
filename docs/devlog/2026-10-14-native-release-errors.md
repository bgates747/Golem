# Native release failures now propagate

The maintained native emitter now reports failed root/argument buffer releases
while still attempting remaining cleanup. Source-language status, previous
effects, shared layouts and buffered output retain their contract. The change
is qualified and ready for the Author's stopped Oryx review.

The Author requested writing up, committing/pushing, then executing the tranche.
The accepted compiler promotion was published as `b1186be`, followed by the
GOLEM-041 plan as `8e3e815`. Implementation changes remain uncommitted pending
the shared emulator human-validation gate.

GOLEM-041 adds one bounded generated cleanup helper in `src/portable/root.hpp`.
It returns the first root/argument cleanup error, otherwise the primary result;
neither callback is retried. Root bytes are untouched after release is attempted.
Success generation is published before cleanup, so native runtime return must
also gate success. The SDK/runtime and Rally application are unchanged.

Maintained checks pass: 52 non-native-C frozen files retain parity, all frozen
fixtures remain intact, 160 sanitizer fault cases and 766 model/generated-C
cases pass, and real assembly consumes the generated contract. Fresh actual
Xtensa JIT/interpreter checks pass default/relocated arithmetic and a 300-frame
animation, including every final playfield pixel and actual MOS return. One
interpreter startup stall was retained; identical-byte retry passed. This is
emulation evidence, not physical hardware or injected firmware failures.

See [the task report](../tasks/GOLEM-041/README.md) for exact receipts, package
hashes, limits and human instructions. GOLEM-041 remains in the authoritative
TODO until human acceptance and implementation publication approval.
