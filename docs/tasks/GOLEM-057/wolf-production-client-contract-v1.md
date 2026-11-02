# Wolf production client contract, version 1

Use the copied-pixel renderer for the first production integration, behind the
Wolf-owned rollback build switch. It is the faster of the two measured bounded
proofs. Keep direct rows as an optional later variant when their roughly80KiB
workspace saving matters. This contract adds no firmware service or wire change.
The standalone renderer remains the product default until its owner and the
Author accept a cutover.

## Sources and ownership

The maintained wire/lifecycle authority is
`/Agon/mystuff/agon-vdplet/docs/API-reference.md` and
`/Agon/mystuff/agon-vdplet/include/vdplet_protocol.h`; actual dispatch is in
`/Agon/mystuff/agon-vdplet/src/vdplet_runtime.h`.
There is **no promoted general-purpose production assembly client library**.
Wolf's accepted
`/Agon/mystuff/Wolf3dOrig/agonport/vdplet/review/review_guest.asm`
is a working MOS callback/token/loading pattern, not a drop-in production
scheduler. Its failure cleanup sends an unrequested best-effort UNLOAD; production
must use the acknowledged drain rules below. Its12000tick fixture timeout is not
a production requirement. MOS's time counter runs at120Hz; choose explicit units.

Wolf owns loader/input/mailbox integration and host tests. Golem owns runtime
and shared contracts. Source/build/stopped-profile preparation can proceed now;
new Oryx execution requires a new central grant. No hardware or publication
approval is conveyed here.

## Discovery and ABI admission

1. Install the combined application packet handler before sending the operand-free
   discovery bytes `23,0,0xD9`. A stock runtime without VDPlets may produce no
   identification reply; use a bounded discovery wait and retain rollback.
2. Accept IDENT only with the10-byte payload signature `VDPL`, protocol1,
   kind1 and token0. Capability checks are subsets:
   `(available & required) == required`, never exact-mask equality.
3. Initial copied pixels require mask17 (buffers1 plus pixels16), package
   ABI1.3. Direct rows later require mask273 (buffers1, pixels/surface16,
   colour rows256), package ABI1.6. These are two build-selectable modules,
   not an obligation to implement automatic runtime selection now.
4. INFO0 remains wire identity0x0100; it is **not an advertised maximum minor**.
   There is no new minor-version query. The module header declares its required
   minor/capabilities; LOAD authoritatively rejects incompatible firmware before
   execution. The module's existing host-table extent/minor/callback checks
   remain required. A rejected LOAD leaves the client's handle zero.

## Upload, ownership and ordinary operations

1. The eZ80 owns filenames, staging/input/output/work/argument IDs and pacing.
   Clear the staging buffer; send bounded complete ordinary WRITE blocks. If
   multiple blocks are appended, consolidate with stock command14 before LOAD.
   LOAD requires exactly one contiguous block of the exact declared package size.
   Apply the same single-block rule to service buffers/arguments. RUN's argument
   block is at most32bytes. No new chunk size or allocation limit is introduced.
2. Use wire protocol1 and the existing framed command `23,0,0xD8,len:u16`.
   Payload prefix is `protocol:u8,operation:u8,token:u16,flags:u8,reserved:u8`;
   flag1 requests a reply, other flags/reserved zero. All multibyte fields LE.
   LOAD op1 appends staging:u16, package_bytes:u32, reserved:u16=0;
   RUN op2 appends handle:u16, entry:u16, arguments:u16;
   UNLOAD op3 appends handle:u16. Full lengths19/17/13bytes respectively.
3. One active slot, one outstanding requested transaction. Do not take over an
   existing handle discovered by INFO2. Only a matching successful LOADED kind2
   publishes a fresh nonzero owned handle. Reload requires acknowledged UNLOAD
   and a fresh LOAD; patch prebuilt RUN packets with the new handle. Never assume
   handles repeat. Successful generations stop at65535 instead of wrapping.
4. Replies arrive as stock packet0x81/length10 containing
   `VDPL,protocol1,kind,token:u16,value:u16`. Accept the expected token and
   allowed kind: LOADED2/newhandle, DONE3/zero, ERROR5/status. Do not accept an
   unrelated key or stale reply as completion. Snapshot the tiny mailbox atomically
   in foreground; do not transmit commands or perform blocking work in the ISR.
   Tokens must not be reused while a matching old response can still arrive.
5. HANDLE8 means stale/wrong ownership; BUSY7 is not permission to unload another
   owner. Treat native errors as errors. UNLOAD releases native storage even if
   FINI returns an error: after a matching completed UNLOAD transaction the slot
   is released, while the reported FINI status remains an application error.
   Caller-owned ordinary buffers survive unload and require separate cleanup.

## Keyboard coexistence and exit

1. Extend the application's one MOS keyboard hook to recognize/consume VDPL
   packets and dispatch ordinary keyboard traffic through its existing path.
   The accepted fixture copies10payload bytes then neutralizes the fake key
   record before returning to MOS. Preserve callback registers and real keys.
   Do not install two competing hooks or silently replace the game's callback.
2. Escape sets an exit-request flag. Stop scheduling new RUNs. If a requested RUN
   is in flight, keep its input/work/output storage and callback/mailbox alive
   until its matching DONE or ERROR arrives. No cancellation/preemption exists:
   RUN is synchronous native code and must return. An acknowledgement means RUN
   returned, not that queued drawing was presented.
3. Then issue one requested UNLOAD using the owned fresh handle and wait for its
   matching completion/error. Only after this may the host clear native-used
   ordinary buffers, restore the previous callback/key handling, cursor and mode,
   and return to MOS. Before disposing any bitmap backing storage also apply the
   established drawing-consumption fence. Keep display queues and native lifetime
   as distinct completion boundaries.
4. A timeout does not cancel a RUN/LOAD or establish that a truncated command was
   not executed. Do not retransmit ambiguously, reuse the token, clear referenced
   storage, remove the reply hook or claim graceful cleanup. Preserve pending
   ownership and report the failure; a later matching response may complete the
   drain. Nonreturning native code or broken transport needs an explicit recovery
   policy outside this initial finite-module contract. Finite deadlines detect
   failure; they cannot enforce a native-code execution limit.

## First integration gate

Prepare the copied-pixel build and proportional host tests for unavailable
runtime, capability supersets/missing bits, LOAD rejection/fresh ownership,
unrelated keys/stale tokens, native error, timeout/pending ownership and Escape
while RUN is outstanding. Reuse the accepted module and exact bounded scene
before extending the full-game renderer. Send source/module/wire/profile pins
and one finite headless readiness request; Golem will schedule it around6502
and FSIM. Direct-row integration, continuous gameplay performance and physical
qualification remain separately identified follow-ups.
