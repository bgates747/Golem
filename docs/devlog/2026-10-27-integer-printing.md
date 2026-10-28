# Resident integer printing

GOLEM-052 adds decimal integer output entirely on stock VDP. Literal print and
print str(integer-expression) work in the calculation profile; consecutive
statements compose output without a new operator. All four integer types and
signed minima are covered. Source values are preserved, and checked expression
faults stop before that print's text, retaining earlier output.

A fixed per-site text temporary and a shared formatter for each used width
reuse existing divmod and stock offset calls. No string heap, concatenation,
eZ80 formatting/readback, native text service or firmware change is introduced.
Printing selects stock-only output; incompatible native/both requests reject.
No-print dual output bytes remain unchanged. The Agon compiler and distribution
examples/how-to are updated together.

Maintained contracts/sanitizers and independent decoder checks pass. Actual MOS
compilation and 66 Pi stock pixel pairs match; the Oryx stock ESP32 run matches
all 66 printed sequences and 1,320 state bytes. Headless human-mode verification
passes 200 state bytes.

The Author's Oryx screenshot confirms correct integer limits, retained totals of
55 and 110, overflow/conditional output, PASS and working MOS dir. Fresh saved
results match all 200 expected bytes, and the staged compiler hash matches.
GOLEM-052 human review passed. Publication remains pending explicit approval;
the implementation and updated distribution sources remain uncommitted.

The Author explicitly approved committing and pushing all pending work on
2026-10-28. GOLEM-052 is complete and removed from the active register.
