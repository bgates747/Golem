# agon-6502 codec qualification

The 6502 owner accepted the buffer/frame service ABI and owns its F2/F3 codec
module. Qualify that separately supplied code as uploaded Xtensa on Oryx, using
the existing loader firmware and a separate stopped profile. Preserve the
GOLEM-021 human review and the other project's source/hardware ownership.

- ABI-001 [x] Publish working service and receive owner confirmation that no additional callback is needed. The owner keeps codec source/packaging under agon-6502/experiments/perf020/vdplet.
- SRC-001 [x] Receive and inspect the pinned source, module, fourteen-byte argument contract and deterministic fixtures; verify imports, compiler stack usage and admitted section/relocation sizes.
- ASM-001 [x] Build a focused eZ80 consumer using the existing transaction runner, with independently derived retained/pixel expectations. Upload the module after boot; no decoder is linked into firmware or replaced by host code.
- TST-001 [x] Test valid retained decoding, repeated application, malformed late/truncated stream with unchanged destination, and scoped framebuffer output. Use selected retained-word observations plus the complete physical-VGA oracle; keep diagnostics out of the production call.
- TST-002 [x] Qualify both execution engines headlessly on Oryx in codec/.emulator and record exact module/firmware/frontend identities, errors and limitations. Leave the graphical emulator stopped.
- XAG-001 [x] Return the evidence and any concrete defect to the codec owner through its existing mailbox thread. Acceptance of the service does not transfer source or hardware authority.

The actual fixture format and error expectations will follow the supplied
module contract. A planned fixture is not evidence that its codec has passed.

The owner supplied `apple-codecs.vdpl` (SHA-256
`fd1aa6a5d1c4d51f0f27fd191de896cee4c7e93480f8a611e6e96ac798fd6092`)
and eighteen cases. Run each mode-11 sequence prefix independently so every
intermediate image receives a complete physical-VGA comparison. Observe
selected retained words after each call, and all 3,840 retained words at each
codec's final prefix and unsupported-mode case. This is qualification traffic,
excluded from the normal invocation cost. A task-local copy of the existing
MOS runner may load up to 192 KiB of tape in RAM 0x050000–0x07ffff and reserve
40,000 observation bytes below 0x050000; verify both bounds at build time.
Keep the existing main consumer unchanged apart from making its saved-result
verifier accept a one-module fixture. Reuse the exact current native firmware,
adapter and frontend; no firmware rebuild or codec-specific host hook.

Fresh results and reporting limits are in [README.md](README.md). All thirty-six Oryx runs pass; the graphical instance remains stopped.
