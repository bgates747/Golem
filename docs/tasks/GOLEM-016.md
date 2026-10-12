# GOLEM-016 — Generated assembly interface

The Author accepted named assembly wrappers and authorized the next implementation
tranche. Add optional assembly output for current resident packages, without
changing the existing VDP command bytes. Table implementation remains later work.

- OUT-001 [x] Emit named storage/interface symbols, payload bytes and an ADL
  wrapper callable as call golem_<entry>.
- CLI-001 [x] Add an explicit optional resident output flag; reject assembly
  symbol collisions before publishing any files.
- TST-001 [x] Assemble a consumer and compare included bytes, symbol locations
  and wrapper instructions across normal/relocated packages; run maintained
  compiler checks and sanitizers.
- REV-001 [x] Record the contract and evidence, then stop for review.

Keep fixtures/reports under GOLEM-016/. No table parser, runtime loader or new
emulator fixture is authorized by this output tranche.

Implemented and host/assembler-qualified. [Usage and evidence](GOLEM-016/README.md).
Accepted by the Author before GOLEM-017 was authorized. No emulator fixture was
changed or staged by GOLEM-016. Later table work reuses the assembly emitter;
GOLEM-017 separately records actual wrapper execution.
