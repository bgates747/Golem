# Compound Boolean expressions on both backends

GOLEM-029 adds Not/And/Or, short-circuit evaluation and parentheses to the
accepted task-local Boolean frontend and both emitters. Host checks pass;
target evidence and human review are tracked in the task report. No production
compiler or generic VDPlet SDK/runtime changes were needed.

Precedence is arithmetic, comparison, Not, And, Or. Short-circuiting preserves
left-to-right evaluation and avoids runtime errors in skipped operands. All
external fields are still validated at entry. Boolean grouping and numeric
operand parentheses coexist; unary/group nesting is bounded to 16 levels.
Constant-invalid arithmetic continues to be rejected during compilation,
regardless of branch reachability, as in the earlier arithmetic slice.

A focused test caught error returns from buffered RHS helpers resuming their
caller. Logical helper calls now propagate the private arithmetic error status
through every calling frame before any destination write or following statement.
The shared public status remains the existing finite error contract. Invalid
external Boolean values still fail entry admission with status 32.

ASan/UBSan, independent command-byte model, generated-C host behavior and ELF
admission pass 5,284 cases; 59 grammar/reference cases and 74 rejection cases
pass, along with maintained compiler checks. Full host coverage retains the
prior arithmetic, comparison, Range and Boolean fixtures. Host checks took
61 seconds on the Pi. Actual-target tests select 45 representative invocations
so the finite guest tape stays within 64 KiB; both targets use identical inputs.

The traffic-light review keeps its known appearance intentionally. Golem now
computes amber using conjunction, green using a negated grouped disjunction,
and moving using conjunction/negation. Assembly still draws/paces/selects the
backend; neither backend switching nor rendering supplies the source decisions.
State remains 20 bytes; buffered installation is 5,155 bytes and the native
package is 1,552 bytes, including 640 text bytes and four data bytes. No board
was flashed and emulator times make no physical performance claim.

Stock qualification passes 45 compound/Boolean calls and 24 lamp calls. Both
actual Xtensa engines pass 90 calls/823 observations/8,230 bytes at default and
relocated IDs, plus 600-frame review runs with 533 observations/5,249 bytes and
56,320 final pixels. Oryx visual staging is verified/stopped, with stale results
removed. Human review and explicit commit/push approval remain pending.

The Author accepted the review with “checks out”. Retrieved human results match
all 533 observations/5,249 bytes; runtime identities pass. Keyboard success was
not separately stated. The Author explicitly approved commit/push on 2026-10-13.

GOLEM-029 is accepted and removed from the authoritative TODO register.
