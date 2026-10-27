# GOLEM-053 — Complete the integer bitwise operator family

Add bitwise AND, OR and complement beside existing XOR and right shift. The
primary output is resident buffered commands for bone-stock VDP; retain matching
semantics in generated native C and the on-Agon compiler distribution.

## Language contract

1. `&` is bitwise AND, `|` is bitwise OR, `^` remains bitwise XOR and unary `~`
   complements every bit in the operand's declared width. These are integer
   operations, separate from short-circuit Boolean `And`, `Or` and `Not`.
2. Support uint16, sint16, uint32 and sint32. Binary operands must have the same
   type; no implicit widening or Boolean/float conversion. Result type and width
   match the operands. Signed integers use their two's-complement bit patterns.
   Complement is width-masked; it does not perform checked numeric negation.
3. Precedence, highest first: atoms/conversions, unary `~`, multiplication/division,
   addition/subtraction, existing `>>`, `&`, `^`, `|`, comparisons, Boolean `Not`,
   Boolean `And`, Boolean `Or`. Binary operators associate left; prefix `~`
   associates right. Parentheses override grouping. Existing expressions retain
   their grouping. At most 16 consecutive complement operators are admitted.
4. Operands evaluate once, left to right. Bitwise operators do not short-circuit.
   Operation itself cannot overflow; ordinary input admission, expression faults,
   checked conversions and destination Bounds still apply. Failed assignments
   preserve the destination; existing status propagation remains intact.
5. These operators are expression syntax. Declaration initializers retain their
   current literal-only rules. No `&&`, `||`, compound assignment or left shift
   is introduced. General strings/concatenation and numeric printing are separate.

## Plan and acceptance

- SYN-001 [x] Record syntax, types, precedence and errors; extend typed parser,
  constant evaluation, reference execution and expression metadata.
- VDP-001 [x] Lower integer &, | and ~ using stock bytewise buffer adjustment;
  preserve source values and existing resident ownership/call/cleanup contracts.
- NAT-001 [x] Match width-defined semantics in generated C without signed C
  overflow or implementation-defined complement behavior.
- CHK-001 [x] Check all integer types, constant/runtime parity, signed boundaries,
  precedence, arrays, Bounds, short-circuit distinction, rejection and repeated
  invocation. Execute generated C and stock VDP output; run maintained regressions
  and sanitizers. Preserve existing reviewed output bytes.
- MOS-001 [x] Rebuild the AgonDev compiler and qualify actual MOS compilation
  against desktop output for the added operators; package current sources/binary.
- DOC-001 [x] Update language reference, example and distribution how-to; rebuild
  and verify the standalone ZIP. Prepare a visible review without launching the
  Author's emulator or changing the active walkthrough session.
- REV-001 [x] Author screenshot confirms expected results, PASS and working MOS
  dir/keyboard. Retrieved fresh results match all 4,968 bytes. Publication is
  a separate explicit approval; no commits, pushes or flashes performed.


## Results

Maintained checks pass, including 230 value/folding checks, 20 new rejection
cases and the existing 52 frozen portable artifact comparisons. The bitwise
contract suite and generated native C pass AddressSanitizer/UndefinedBehaviorSanitizer.
Fifty-four generated-C executions compare 4,968 result bytes against an
independent Python bit-pattern oracle, including relocated IDs and failed
assignment preservation. The Agon library adapter passes five accepted and
three rejected cases with its documented ASan stack-recovery configuration.

Four small programs, one per integer type, compile on emulated Agon and match
desktop artifacts. The combined stress fixture is PC-compiled and executed as
stock commands; the native compiler's existing finite-RAM limits remain.
An initial direct-runbin combined compilation reported MOS Out of memory;
that command is not a qualified native compilation witness. Final compilation
checks use the supported dispatcher and individual type programs.

Pi Fab/MOS and Oryx Flexe with unchanged stock VDP2.16 each pass 54 calls and
all 4,968 observed bytes. Oryx results were retrieved and independently compared.
The stock path requires no SDK service or firmware modification. The desktop
and AgonDev compiler, source ZIP, how-to and language reference are updated.
No commits, pushes or physical flashes were performed.

The separate stopped Oryx review is at:
`~/Agon/mystuff/golem/.emulator/golem-053`. Launch `./flexe-agon-emulator`
from that directory. It compiles the four small programs, runs the stock fixture
and displays actual uint16 results: 1,7,6,65530. The screen explicitly identifies
Golem calculation and C diagnostic readback/display. A PASS line reports
4,968 observations. Type dir to confirm MOS keyboard response; close and report.
The agent retrieves saved results. Repeating the review overwrites its own
result files and is supported.

Evidence: host-evidence.json, adapter-evidence.json, stock-evidence.json,
oryx-evidence.json and deployment.json. Build/qualification scripts and the
review source/client are task-local; maintained implementation/tests live in src
and tests. Numeric printing and string work remain GOLEM-052/separate discussion.

Human review passed on 2026-10-27; see human-evidence.json. The screenshot
shows the actual expected results and a successful MOS dir command.

## Publication checkpoint — 2026-10-27

The Author explicitly authorized committing and pushing all pending Golem work
in logical groups after this human review. GOLEM-053 is accepted and complete.
