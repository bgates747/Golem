# Generic AGNB loading dependency accepted

The generic plain-buffer container and bounded MOS loader are implemented,
qualified and accepted. The Author confirmed the Oryx review worked; agent
retrieval matches all66 result bytes and runtime identities. Golem-specific
package/bootstrap integration remains GOLEM-046, with its generic dependency
now satisfied.

AGNB-001 lives in agon-utils/examples/agnb, including formal tasks, public format/
API docs, dependency-free host writer/reader/manifest CLI, additive eZ80 loader,
qualification and human evidence. It loads515648 payload bytes from a516032-byte
file through an8192-byte window in a12091-byte app. Actual stock Pi/Oryx runs
pass, including consecutive loads and malformed/late/truncated cases. Payload
observation is sampled; no runtime readback/checksum mechanism was introduced.

Published branch codex/agnb-plain-buffers has host commit3edf8bc and runtime
commit35dd8ef. Canonical local equivalents are e6f1bcc/24596fc. The isolated
upstream worktree avoids publishing or rewriting unrelated divergent utility
history. Earlier image/audio interfaces and bytes are preserved.

The Author explicitly assigned generic AGNB ownership, documentation and scoped
commit/push duties to this agent. Shared-workflow human review is now complete
for this tranche. Generic API never interprets a payload as a program or does
compiler-specific allocation/setup; those remain with Golem. No physical target
was touched. Existing unapproved GOLEM-045 work remains separate/uncommitted.
