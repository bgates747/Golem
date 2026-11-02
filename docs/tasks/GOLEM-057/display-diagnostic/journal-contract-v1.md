# Display allocation journal: candidate contract1

Prepare a consumer for a separate diagnostic firmware, collecting a few
foreground allocation and palette-selection observations. This adds no public
VDU command or production service. The original controller/allocator decisions
remain; observation can perturb memory/timing and is not a repair. Firmware/ELF
pins and a fresh execution grant are still required before any target run.

## Frozen schema

- Header g057_diag.h:fd398c438eca0ae1313bce0e23443e50e6de2d8b2ffb90d7c49a677c502bfa47.
- Readable parser parse_journal.py:8441b7720f42b717fd041db769cc23c8f3e7995250d03edd3b86863f11b4d1e8.
- Records are64raw little-endian bytes (`<16I`) at arbitrary offsets of the
  retained UART0 stream, mixed with existing debug text. No text decoding first.
- Word0:0x47570100|event;1=DEFAULT_PLANE,2=INITIAL_LIST,3=PREACTIVATE.
- Word1:low8ordinal1..8. Bits8allocation-called,9lookup-required,10map-present,
  11raw-skip,12existing-null,13existing-nonnull,14selection-peek,15saturated.
- Word2:requestsequence. Word3bytes:requested,previous,attempted,tag. Taglow2
  phase1requested/2restoreprevious/3finalfallback; taghigh6attemptordinal.
- Words4..6:controllerpointer,nativeformat,tablebytes. Formats0mono/1rawSBGR2222/
  2RGB565BE/3PALETTE2/4PALETTE4/5PALETTE8/6PALETTE16.
- Words7..10:actualallocationrequestbytes,caps,returnpointer,defaultmap0pointer.
  Allocationcalled/map-present flags distinguish a zero observation from an
  allocation notcalled/absentkey. No extra allocator call is introduced.
- Words11..13:listhead(or newlyallocatednode),currentnode,peek-or-actualsignals.
  INITIAL_LIST is a nonmutating selection peek; PREACTIVATE is actual selected
  state immediately before GPIO/IRQ activation. It does not prove activation.
- Word14:paletteIDlow16/listcount8/row8. Word15:viewportwidthlow16/heightupper16.

The first requestedMODE20 transaction alone arms capture, including existing
requested/restore/finalfallback attempts. Group by requestsequence,attemptordinal
and controllerpointer, never a pointer or PC borrowed from another run. Ordinals
are monotone and never overwritten; records emit once before activation. Eight
records or saturation is conservatively incomplete. A short/absent/malformed
record set is not success. There is no completion footer; even fewer than8records
requires independent MODE/consumer completion evidence. An error before the
preactivation emission may leave the journal absent/incomplete.

## Consumer preparation

1. Preserve original module85aa1ce9/clientd316, ROM/PROM, mode11 baseline,
   INIT/COLD/64ROMinstructions205cycles, request20, GP/threefreshframe observation,
   historical interpreter-origin backend140e, and45s OR6e9primarycycles/normal
   15sjoin. No AppleRENDER or surfaceprobe is added. New firmware/ELF pins will
   replace only those per separately frozen control/diagnostic arm.
2. Prepare TWO fresh processes, matched macrooffcontrol first then macroonjournal.
   Originalb42/a10 and alloldpartials remain immutable reference, not another
   automatic rerun. No original numericalPC/controlleraddress is a newrunidentity.
3. Reuse existing independent usable/strict/metadata/cleanup/heap statuses.
   Collect rawUART0 after normalstop/join along with originmetadata/sourceELF.
   No extra firmware query to collect records, no ISR logging or readzeroaliasfix.
4. Validate/parser-check the journal on host before source/input/command freeze.
   Do not require flags/events from controllers that never existed. Distinguish
   expectedrawskip, actualallocnull, existingnull, laterlostselection, unmatched
   ISRcontroller and perturbation/nonreproduction. No physicalOOM inference.
5. Macrooff must expose no journal. Targetlease/grant and finalsource/build
   artifactreceipts are later requirements; this contract releases source/host
   preparation only. Stop/report unexpected transport/schema/source failure;
   no automatic retry, repair, cohort expansion or product/publication adoption.
