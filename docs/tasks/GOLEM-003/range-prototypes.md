# Range syntax prototypes

Use `Range(start:end:step)` to describe an inclusive integer sequence, with
step defaulting to 1. The same description could support iteration, membership
and indexed dispatch without constructing an array. Colon spelling and For/In
were selected in the 2026-10-11 review; they are not implemented grammar.
The accepted first jump-table target remains consecutive
step-1 slots; stepped dispatch and the other uses below are proposed extensions.
Numeric bounds need a separate meaning from sequence membership.
Each code sample is independent and includes its required declarations and
initial state. Loop headers assign the counter's initial value; they do not
implicitly declare it.

## Sequence meaning

| Expression | Values |
|---|---|
| `Range(0:20)` | 0 through 20, inclusive |
| `Range(2:10:2)` | 2, 4, 6, 8, 10 |
| `Range(10:2:-2)` | 10, 8, 6, 4, 2 |
| `Range(0:5:2)` | 0, 2, 4; do not force the unaligned endpoint into the sequence |
| `Range(3:3)` | 3 once |

Proposed initial rules: arguments are compile-time integer constants, step is
nonzero, and its direction must reach toward the endpoint. With distinct bounds,
reject a positive step on a descending range or a negative step on an ascending
range. Do not infer a negative default step: `Range(10:2)` would be an error;
write `Range(10:2:-1)`. Equal bounds produce one value with either nonzero step.
These edge-case rules are proposals, not previously settled decisions.

Endpoints are limits, included only when reached by stepping from the start.
Compute cardinality and intermediate arithmetic in a sufficiently wide type;
never allow iteration or normalization to wrap. Avoid incrementing beyond the
final value when that would overflow the counter. Values must fit the consuming
integer type. Floating steps, runtime range expressions, stored range objects
and automatic materialization into arrays are outside this first prototype.

## Iteration prototype

```golem
sint16 i

For i In Range(2:10:2)
    print "ascending tick"
Next i
```

Descending iteration:

```golem
sint16 i

For i In Range(10:2:-2)
    print "descending tick"
Next i
```

`In` is selected spelling. Keep declaration separate from iteration and retain
the named `Next i`. Evaluate the range once at loop entry. This replaces the
earlier For/To/Step design; the current compiler still accepts its older header.
Compiler-generated counters and termination checks must preserve finite
execution; new loop restrictions and variable-mutation rules need an agreed
implementation contract.

## Membership prototype

```golem
sint16 value = 6

If value In Range(2:10:2) Then
    print "member"
Else
    print "outside or between steps"
EndIf
```

Membership checks both bounds and alignment with the start. Here 6 is a member
and 7 is not, although both lie between the limits. Descending ranges use the
same sequence meaning. Implement checks directly; do not allocate an array or
scan all values. How best to implement general divisibility on the stock buffer
machine remains backend work; this notation makes no speed guarantee.

## Indexed switch prototype

```golem
uint16 direction = 6

Switch direction Range(2:10:2)
Case 2
    print "first"
Case 6
    print "third"
Case 10
    print "fifth"
Default
    print "unhandled"
EndSwitch
```

The five table slots correspond to 2, 4, 6, 8, 10. Missing cases 4 and 8 route
to `Default`; 7 also routes there because it is not a member. Without `Default`,
fallback continues after `EndSwitch`. Check bounds and alignment before using
`(selector - start) / step` as the slot index. The same rule handles descending
sequences with a negative step, using checked arithmetic.

Evaluate the selector once, do not fall through, and reject duplicate cases,
case values outside the sequence (including misaligned values), and tables
exceeding the supported resource budget. Do not silently substitute comparison
dispatch for an explicit table request. Stepped normalization adds work and
requires qualification; the currently selected initial implementation target
is still `Range(start:end)` with step 1.

## Numeric bounds are a different concept

The maintained scalar specimen still uses legacy `float32 x Range(-1, 1)`
(comma-separated numeric bounds). Current proposals use a separate Bounds name:

```golem
float32 x Bounds(-1, 1) = 0.25
```

Bounds describes numeric values, not discrete sequence membership. Renaming the
parser and implementing runtime enforcement are separate changes. Existing
executable floor.golem keeps its current spelling and host-admission contract.

## Review status

Colon-separated inclusive Range and For/In are selected design. Stepped Switch,
membership implementation, compile-time-only initial arguments and loop-counter
mutation/final-value policies are covered in the
[SEM-001 review](../GOLEM-009/semantics-review.md#ranges-and-control-flow).
The selected initial Switch target remains step 1. No parser/backend changes are
authorized by this writeup; examples remain terminator-free.
