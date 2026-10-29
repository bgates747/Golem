# GOLEM-054 — Resident float32 text formatting

Make floating-point results visible using the same `print str(value)` syntax as
integers, entirely in resident buffered commands on stock VDP. Preserve existing
integer output and application ownership. Fixed-point types are still proposed
in the maintained calculation profile; their formatting follows implementation
of those types rather than introducing a print-only type system.

## First tranche

The Author releases implementation after recording this task and publishing the
pending work. Use these ordinary, deterministic defaults for review:

1. `str(float32-expression)` formats the actual binary32 value to nine significant
   decimal digits, rounded to nearest with ties to even. Nine digits preserve
   binary32 information; this is not a shortest-representation formatter.
2. Omit trailing fractional zeros and an unnecessary decimal point. Use ordinary
   decimal notation for a rounded decimal exponent from -4 through 8 inclusive;
   otherwise use lowercase scientific notation with an explicit exponent sign
   and at least two exponent digits. Zero prints `0`; no negative zero.
3. Keep existing integer syntax/output unchanged. Each expression evaluates once;
   consecutive print statements insert no spaces or newline. No concatenation,
   print lists, stored strings, formatting arguments or native printing.
4. Respect existing float admission: finite normal inputs and canonical zero;
   reject nonfinite/subnormal external inputs as before, and retain existing
   expression normalization/error behavior. Preserve source variables and Bounds.
   Earlier successful text is not rolled back if a later expression fails.
5. Use bounded exact integer/decimal conversion of the binary32 representation,
   avoiding repeated rounded float scaling or a 16-bit conversion range limit.
   Reuse existing stock buffer adjustment, comparisons and resident helpers.
   Shared formatter scratch and per-site text are allocated at compile time.
   No firmware, VDPlet, eZ80 numerical conversion or readback is required.

```golem
float32 value = 1.5
Sub show
    print "value=" print str(value)
EndSub
```

Expected text is `value=1.5`. An assembly/C application installs once, updates
the declared input bytes and calls the exported resident entry by name.

## Work and acceptance

- FMT-001 [ ] Prove decimal capacity, rounding, notation boundaries and scratch
  lifetime; retain an independent exact-rational reference under GOLEM-054/.
- PAR-001 [ ] Admit float32 through str without changing integer parsing,
  target selection or old no-print artifacts; update expression metadata/model.
- VDP-001 [ ] Implement one shared bounded resident formatter, preserve inputs,
  clear per-site output storage and return normally after every finite call.
- CHK-001 [ ] Check zero/sign, binary32 extrema, decimal carries/ties, values on
  either side of powers of ten, fixed/scientific transitions, repeated calls,
  expressions, failed admission and relocated buffers against an independent
  oracle and actual unchanged stock VDP. Run maintained checks and sanitizers.
- MOS-001 [ ] Rebuild the native Agon compiler; qualify an actual on-Agon float
  printing example and refresh the source/distribution package and how-to.
- DOC-001 [ ] Update language reference and initialized examples. Record exact
  package/scratch costs and the formatter's work bound without speed claims.
- REV-001 [ ] Prepare a dedicated stopped Oryx visual test in this tranche,
  identifying Golem's output and the host's role. The Author reviews the text
  and follows the single onscreen keyboard instruction; the agent retrieves
  saved results. Keep new implementation uncommitted pending explicit approval.

Task artifacts belong in docs/tasks/GOLEM-054/. TODO.md owns overall status.
Use actual Fab for stock review while Flexe's audio investigation proceeds;
preserve existing profiles. Physical flashing is outside this tranche.

## Follow-up

Fixed-point formatting, configurable precision/notation and native-backend text
output remain separate decisions. They need not delay useful resident float32
output or expand this tranche into a general string library.
