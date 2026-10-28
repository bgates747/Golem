# GOLEM-052 — Resident integer-to-text conversion and printing

Add useful numeric output to Golem programs running entirely as resident buffered
commands on bone-stock VDP. Reuse existing arithmetic and buffer machinery;
no eZ80 value readback, custom firmware or native backend is required.

## Scope and acceptance

- SYN-001 [x] Implement the agreed explicit `print str(integer)` form with literal
  printing in the calculation profile. Cover uint16/sint16/uint32/sint32;
  define expression evaluation and output lifetime before implementation.
- ALG-001 [x] Reuse existing checked quotient/remainder helpers for decimal
  conversion; evaluate repeated division by ten and backward digit construction.
  Handle zero, sign, signed minima and leading-zero suppression without overflow.
- MEM-001 [x] Assign bounded temporary storage at compile time (at most ten
  digits plus sign for supported integer widths). Reconcile temporary lifetime
  with the recorded per-call-site str model; avoid a general heap/string subsystem.
- LIB-001 [x] Install shared resident conversion helpers once per package, including
  only required widths. Preserve caller variables and support repeated invocation.
- CHK-001 [x] Verify decimal bytes for boundaries and representative values against
  an independent host formatter; execute emitted commands on stock VDP, including
  repeated calls and cleanup. Measure installation bytes and command work.
- DOC-001 [x] Replace silent numeric distribution exercises with visible calculated
  output; update the language reference, standalone how-to and native Agon compiler
  distribution together. Stage the Oryx review without launching its GUI.

Floating-point formatting, stored/general strings, concatenation and formatting
options are deferred. Native output parity is not an implementation prerequisite.
The primary product is resident stock buffered commands callable from ASM/C.

## Existing decisions

See language-type-proposals.md Axes 7–8: print accepts text; str performs explicit
conversion. The implementation must reconcile any bounded storage refinements
with those older design notes rather than silently changing them.

## Released implementation contract — 2026-10-27

The Author released this task, excluding concatenation. Print takes exactly
one literal string or `str(integer-expression)`; consecutive print statements
emit adjacent bytes with no implicit spaces or newline. No comma operator or
print-list syntax is added. Formatting is ordinary signed/unsigned base10,
no leading zeros, one zero for zero, and a minus sign only for negative values.
All four integer types are supported; mixed variable types need explicit
conversion. A context-free str argument is parsed in existing integer candidate
order (uint16,sint16,uint32,sint32); literals follow the first valid typed
expression. Bool/float and formatting arguments are rejected.

Each numeric print site owns a fixed 6/11-byte text buffer (5/10 digits plus
possible sign), consumed immediately and zeroed after use; no stored string
value or heap is introduced. One formatter per used width shares existing
quotient/remainder routines. Its first-character offset suppresses leading
zeros; stock offset calls emit only actual text. Signed magnitude is formed
in unsigned scratch, including the minimum signed value. Source globals remain
unchanged. Expression errors occur before that print's output; earlier text is
not rolled back. Entry admission and existing status handling are unchanged.

This extends the calculation (--portable) profile, whose stock output is a
resident package. Print requires stock buffered output for now. Such programs
default to stock-only; explicit --target buffered is also available. Explicit
native/both selections reject before output mutation. Existing no-print default
dual packages retain their exact bytes. Other profiles' grammar is unchanged.
Native printing and general strings remain separate work.


- REV-001 [x] Author reviews the prepared Oryx display and keyboard. The
  2026-10-27 screenshot shows correct numeric output, PASS and successful MOS dir;
  retrieved results match all 200 expected bytes.
- REV-002 [x] Explicit publication approval received on 2026-10-28: the Author
  requested commit/push of all pending work before the next implementation.

## Implementation results

The maintained compiler and AgonDev port implement the released contract.
Stock-only target selection omits native files/calls and rejects incompatible
explicit selections before output mutation. Print programs use typed-program
schema5; old no-print schema4 packages retain their frozen bytes. The current
source ZIP and how-to include visible counter, array and bitwise exercises.

Maintained checks and sanitizers pass. An independent command decoder checks
132 calls / 496 main decimal-value comparisons, source preservation, cleared
per-site text and cleanup. Actual Pi stock execution matches all 66 reference
screens (20,275,200 pixels) and 1,320 state bytes. Actual Oryx stock ESP32/Flexe
matches every one of the 66 printed strings and all state bytes, including
signed minima and large-to-small changes. Four-width conversion is entirely
resident; the eZ80 client performs only input/entry/qualification work.

The dedicated human profile .emulator/golem-052 also passed headless review-mode
checks before staging. The Author's 2026-10-27 screenshot confirms the expected
integer limits, retained totals of 55 and 110, overflow/conditional output, PASS
and working MOS dir. Fresh retrieved results match all 200 expected bytes; the
staged compiler hash also matches. See human-evidence.json in the task silo.
Human review is complete. The Author explicitly approved commit/push on
2026-10-28. Physical devices were not flashed.
