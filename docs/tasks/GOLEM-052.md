# GOLEM-052 — Resident integer-to-text conversion and printing

Add useful numeric output to Golem programs running entirely as resident buffered
commands on bone-stock VDP. Reuse existing arithmetic and buffer machinery;
no eZ80 value readback, custom firmware or native backend is required.

## Scope and acceptance

- SYN-001 [ ] Implement the agreed explicit `print str(integer)` form with literal
  printing in the calculation profile. Cover uint16/sint16/uint32/sint32;
  define expression evaluation and output lifetime before implementation.
- ALG-001 [ ] Reuse existing checked quotient/remainder helpers for decimal
  conversion; evaluate repeated division by ten and backward digit construction.
  Handle zero, sign, signed minima and leading-zero suppression without overflow.
- MEM-001 [ ] Assign bounded temporary storage at compile time (at most ten
  digits plus sign for supported integer widths). Reconcile temporary lifetime
  with the recorded per-call-site str model; avoid a general heap/string subsystem.
- LIB-001 [ ] Install shared resident conversion helpers once per package, including
  only required widths. Preserve caller variables and support repeated invocation.
- CHK-001 [ ] Verify decimal bytes for boundaries and representative values against
  an independent host formatter; execute emitted commands on stock VDP, including
  repeated calls and cleanup. Measure installation bytes and command work.
- DOC-001 [ ] Replace silent numeric distribution exercises with visible calculated
  output; update the language reference, standalone how-to and native Agon compiler
  distribution together. Stage the Oryx review without launching its GUI.

Floating-point formatting, stored/general strings, concatenation and formatting
options are deferred. Native output parity is not an implementation prerequisite.
The primary product is resident stock buffered commands callable from ASM/C.

## Existing decisions

See language-type-proposals.md Axes 7–8: print accepts text; str performs explicit
conversion. The implementation must reconcile any bounded storage refinements
with those older design notes rather than silently changing them.
