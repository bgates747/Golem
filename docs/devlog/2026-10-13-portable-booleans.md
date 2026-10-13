# Typed Boolean values and a distinct visual review

The Author approved light thinking and the bounded Boolean tranche after accepting
GOLEM-027. All prior dirt was committed and pushed as f0d200a. GOLEM-028 now
implements bool storage, comparison assignment and If flag on both task-local
backends, with qualification passing. Human review passed and explicit commit/push approval was subsequently given.

The shared model adds a Boolean type and comparison expression. A bool occupies
two canonical 0/1 bytes with distinct type tag 6. Globals initialize from true/false;
Subs can assign literals, copies or uint16 comparisons. Invalid external bool
encodings fail admission with status 32. No implicit conversions, Boolean
arithmetic or compound Boolean operators were added. Comparison failures retain
the Boolean destination and earlier effects. Generic SDK/runtime and production
compiler are unchanged.

Host ASan/UBSan and model/generated-C checks pass 4804 cases, 24 grammar/reference
acceptances and 64 rejections, plus ELF and maintained checks. Earlier arithmetic,
condition and Range fixtures remain covered. Stock checks pass 64 Boolean calls
and 24 traffic-light calls. Each ESP32 engine passes 128 Boolean calls, 128
relocated calls and the 600-frame traffic-light review.

The new visual deliberately avoids the previous moving-square pattern. Its
lights.golem stores red/amber/green comparisons, copies green to moving and uses
Boolean If to set lamp colours. Resident VDU draws three vertical lamps; assembly
selects backend and paces with VSYNC. Final amber-on pixels match all 56320
playfield pixels and 533 observations/5249 bytes, with actual MOS return.

The state is 20 bytes. Buffered installation is 5156 bytes; native package is
1536 bytes including 624 resident text bytes and four data bytes. Host checks
took about 56 seconds on the Pi. Oryx numeric suites took about ten seconds per
engine and visual runs about 19–21 seconds including boot/loading. These are
functional emulator timings, not physical speed measurements. No board was flashed.

Source, report and final-screen reference are deployed beside the dedicated
stopped Oryx profile. The visual review was subsequently accepted.

The Author accepted the visual review with “looked good to me”. Retrieved human
results match all 533 observations/5249 bytes and runtime identities pass.
Keyboard success was not separately stated. The Author explicitly approved commit and push on 2026-10-13. GOLEM-028 is
accepted and removed from the authoritative TODO register.
