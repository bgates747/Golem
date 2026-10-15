# Full-scene startup stall — offline runtime correlation

Rally's graphical launch stalls before native module installation. Its captured
adapter-ingress bytes are an exact prefix of a successful run. The exact
firmware ELF contains the UART receive-recovery ordering already demonstrated
as defective by FX-006. This is the strongest current lead, not a proven cause
of this particular closed run: live FIFO, interrupt-mask and driver-ring state
were not saved. No Golem compiler/helper change is indicated by this evidence.

## Bounded investigation

- UAR-001 [x] Preserve Rally fifty-one through fifty-three and inspect original failed/pass captures without rerunning the profile.
- UAR-002 [x] Verify ingress-prefix/header identity and correlate exact firmware ELF with existing FX-006/MOS receive-recovery findings.
- UAR-003 [x] Return the specific missing diagnostic state and smallest repair direction to Rally. No functional fix or new run is included.

Author constraint relayed in fifty-three: any agent-run session must set both
`SDL_VIDEODRIVER=dummy` and `SDL_AUDIODRIVER=dummy`. A visible/audible reproduction
requires an Author launch. No emulator session, build, graphical/audio action,
firmware modification or other-owner profile change was performed here.

## Independently checked evidence

The failed stream is 121,112 bytes and equals the first 121,112 bytes of Rally's
successful `headless02/mos-to-vdp.bin`. Its command at offset 120,688 is the
stock buffer WRITE header for buffer 63,800, declaring 2,048 payload bytes.
Only 416 payload bytes reached adapter ingress; the expected command end is
122,744. These are transport capture offsets, not proof of how many bytes the
firmware consumed. Original files, hashes and exact ELF sites are recorded in
[evidence.json](evidence.json). Original captures remain Rally-owned.

Firmware ELF SHA256 is
`b41e4cb37bd44633d14b2e6eea615cbbbdd29a1d4b49c3258b4f49f2e215e567`.
Independent disassembly shows:

| Exact instruction site | Operation |
| --- | --- |
| `0x401287de` | ISR stores the full flag at UART object offset 28 |
| `0x401287ee` | ISR subsequently calls the UART critical-section entry |
| `0x4012880a` | ISR masks RX-full/RX-timeout bits using `~0x101` |
| `0x40128dc6` | Reader clears the full flag while recovering the stash |
| `0x40128d62` | Reader's re-enable routine writes the RX interrupt mask |

The indirect lock/unlock literals resolve through this ELF to
`xPortEnterCriticalTimeout` and `vPortExitCritical`. See the retained
[ISR](uart-isr.disassembly.txt), [reader](uart-recovery.disassembly.txt) and
[re-enable](uart-enable.disassembly.txt) dumps. DWARF independently places
`p_uart_obj` at `0x3ffc6230`; this is an array of pointers, not the UART2 object
itself. [Layout](uart-layout.txt) gives fields needed for a later live capture.
Addresses were derived from this ELF, not transplanted from a different image.

FX-006's separate controlled execution proves the failure sequence: ISR
publishes full, reader recovers and re-enables RX, then the delayed ISR masks
RX with full already clear. Pending FIFO bytes are stranded despite empty
software receive storage. Its evidence uses another firmware image and does
not retrospectively prove Rally's interleaving. See
[Flexe findings](/Agon/mystuff/flexe-agon-emulator/docs/tasks/FX-006/trace/findings.md)
and MOS's independent
[correlation request](/Agon/mystuff/agon-dev-env/cross-agent/agentcoms/messages/2026-10-14_03-28-03_agon-mos_uart0-recovery-hardware-one.md).

The adapter's CTS predicate uses readiness, failure state and the sum of queued
ingress plus sampled UART2 FIFO occupancy, requiring that sum to be below 128.
A receiver that stops draining can therefore correctly hold the sender off;
CTS blockage alone does not identify an eZ80 transmit or adapter defect. Source:
[CTS and pump](/Agon/mystuff/flexe-agon-emulator/adapter/flexe_vdp.c:377).

## Missing observation and next discriminating capture

If this exact failure recurs, preserve the profile's run files before another
launch and capture two read-only live snapshots from the same stalled process:

1. Adapter readiness/failure, ingress queue size, sampled `uart_pending`, actual
   UART2 FIFO count and RX raw/enabled interrupts.
2. Dereferenced `p_uart_obj[2]`: full flag, buffered count, stash length,
   requested RX mask and ring occupancy, using exact-ELF DWARF layout.
3. Both Xtensa PCs/cycle counts and the eZ80 CTS/IER/IIR, TX FIFO/shift and
   execution state, if available; distinguish a transient interrupt-disabled
   instruction sequence from a persistent transmit wait.

FIFO data plus disabled RX bits and an empty software ring/full=false is the
known failure signature. A bounded mask-write/flag history distinguishes the
late ISR disable from another cause. Existing FX-006 snapshot/history tooling
can inform this capture but must be adapted/identity-checked for the exact
adapter, host ABI and ELF; its hard-coded replay PC must not be reused blindly.
The closed graphical run cannot supply these missing observations now.

Do not override RX masks, insert sender sleeps, relax timeouts or lower clocks
as a claimed fix. The candidate repair from FX-006 belongs in the firmware
driver: acquire its existing lock before publishing full, keeping publication
and RX-disable in the same critical section. That requires a separately scoped
owned firmware/SDK build and regression, not mutation of a shared PlatformIO
installation or the generic VDPlet loader. Natural hardware recurrence and a
controlled emulator ordering counterexample are different acceptance evidence.

The maintained emulator's concurrent FX-006 work and all consumer profiles were
left untouched. Further capture/repair must preserve those ownership fences.
