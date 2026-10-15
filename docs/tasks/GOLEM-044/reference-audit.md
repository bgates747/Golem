# Flower algorithm and precision audit

The original algorithm fits Golem's current globals, conditionals, integer
arrays and finite loops. Its missing mathematical operations are floating
division, sine, cosine and floor. The default curve is well suited to float32;
long curves expose substantial accumulated angle error and need a separate
precision decision before we promise accurate results for arbitrary parameters.

## Pinned logic

The primary source is agon-utils' `examples/flower/bbc/src/flower.asm`, at
commit `ca7dde5052c44d731da4184d662000bc9e7a497e`. File hashes, including
the helper, binary16 implementation and VDP coordinate code, are recorded in
[reference.json](evidence/reference.json). These hashes identify the inspected
bytes even if a checkout subsequently changes.

| Stage | Original behavior | Port consequence |
| --- | --- | --- |
| Inputs | petals, vectors, depth, periods, shrink, radius_scale, theta_init | Preserve exact seven default strings and order |
| Step product | `pv = P*V`, then `Q = pv*T` | Round each operation separately for float32 |
| Angle increments | `2*pi/pv`, `2*pi/V` | Normal floating division; radians internally |
| Initial angle | Convert entered degrees; petal angle starts at zero | Reset on every new curve; normalize input 360 to mathematical zero |
| Initial base radius | `R/(1+D)` | Depth must exceed -1 after input conversion |
| Shrink | `0 - ((S*baseRadius)/Q)` | Use raw Q before integer conversion; 0.8 removes approximately 80% |
| First point | Calculate and MOVE before the loop | No initial line from some unrelated graphics cursor |
| Subsequent points | Add main angle, petal angle and base-radius increments; calculate; LINE | Preserve sequential accumulation and connected segments |
| Loop termination | Decrement floor(Q); repeat while nonnegative | Exactly `floor(Q)+1` segments, including when Q is an integer |
| Radius evaluation | `m = cos(petal)*D`, `r = m*baseRadius + baseRadius` | Do not factor into `baseRadius*(1+D*cos(...))` or contract into FMA |
| Cartesian helper | `y = sin(main)*r`, then `x = cos(main)*r` | Keep trig and multiplication on the VDP |
| Coordinate conversion | BBC `INT` is floor, including negative coordinates | Plain current Golem `sint16(x)` truncation would differ; floor support is needed |

The binary16 port's Q8.16 phases, lookup/interpolation, radius-by-index and
N-segment loop are useful comparison material, not the algorithm selected here.
We are reproducing the original logic with the agreed float32 semantics, not
emulating BBC FPP or claiming identical five-byte-float results.

## Numeric contract for the first implementation

1. Preserve the maintained binary32 contract: each operation rounds separately,
   no fast-math/FMA, finite normal external inputs or signed zero, canonical
   computed zero and the current subnormal policy. Strings are not numeric state.
2. `/` has division precedence alongside `*`, left associativity and once-only
   left-to-right operands. Zero divisor reports language status 4; nonfinite
   results report 33; failed assignment preserves its destination. Division
   must implement the agreed rounded quotient, not silently become reciprocal
   multiplication. Test negative zero divisors and underflow explicitly.
3. `sin(float32)` and `cos(float32)` consume radians and return float32.
   Normalize/check their results like other floating expressions. Choose the
   packaged toolchain's ordinary sinf/cosf for the native candidate, then qualify
   actual-target accuracy. Host double trig rounded to float32 is only a model.
4. `floor(float32)` returns float32; an explicit sint16 conversion follows for
   plotting. Reuse the established resident-floor work where applicable without
   implying that the portable parser already implements it.
5. Input domains are positive P/V/T, D > -1, R >= 0, 0 <= A <= 360 and finite
   S. S outside 0..1 is permitted: negative shrink grows the curve; shrink >1
   crosses the origin. Do not silently clamp either behavior.
6. Propose `0 < Q < 65535`: N+1 then fits uint16. This permits fractional
   periods and Q below 1, which still produce one segment. It avoids adopting
   binary16's unrelated 65504 limit. All derived quantities must remain finite;
   converted logical coordinates must fit sint16. Out-of-range coordinates
   produce a controlled error rather than wrapping or arbitrary clamping.
7. This work bound is not an accuracy guarantee. Initially qualify the defaults,
   cardinal/fractional angles and the documented reference cases. If long curves
   need comparable precision, evaluate float64 or an explicitly reviewed change
   to angle/radius accumulation; do not quietly introduce a phase accumulator
   or radius-by-index optimization.

## Measured model results

[reference.py](reference.py) evaluates 16 cases with independent host binary64
arithmetic and a per-operation float32 model. Its trig is host libm double
rounded to float32. The results are neither Xtensa execution evidence nor a
pixel-perfect raster comparison. Distances use logical-to-display scales 0.5
and 480/1024 before integer quantization.

| Case | Segments | Largest float32/model separation from binary64 |
| --- | --- | --- |
| Defaults | 396 | 0.107779 pixels |
| P=3, V=2, T=10000 | 60001 | 382.247 pixels |
| P=1.99999999, V=1, T=1 | 3 in float32, 2 in binary64 | Input rounds to 2.0 in float32 before counting |

The last case is an explicit consequence of typed input conversion. Exact
decimal strings remain on screen even when their numeric values round for
computation. It is not acceptable to derive Q independently from retained
decimal text on the eZ80 while the VDP uses float32.

## Coordinate mapping

Keep the original 1280x1024 logical coordinate system and set the origin to
(640,512). At 640x480 this scales X by 1/2 and Y by 480/1024, with positive Y
upwards. The inspected VDP computes origin Y as `height-1-240`, giving physical
origin (320,239). It truncates each scaled signed delta toward zero before
adding that origin; preserve the stock behavior rather than adding a guessed
one-pixel correction. PLOT clipping belongs to the VDP.

The proposed Flower renderer explicitly consumes logical coordinates. The
existing resident drawing profile's documented pixel/origin-zero contract is
not being changed. Default radius_scale 512 therefore corresponds to at most
roughly 256 horizontal / 240 vertical pixels initially, rather than a cropped
512-pixel radius. Final qualification must test the actual VDP rasterization.
