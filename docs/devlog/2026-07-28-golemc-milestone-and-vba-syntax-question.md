# golemc: first working toy compiler, plus a VBA syntax detour

Date: 2026-07-28

## Milestone: first real compiler code, verified end to end

`src/` had been a placeholder (`.gitkeep`) tree since the kickoff devlog.
`src/golemc.cpp` is the first actual compiler code: a small C++17 program
implementing a deliberately minimal grammar - a sequence of zero or more

```
print "string literal";
```

statements (trailing `;` optional, `//` line comments, `\\`/`\"`/`\n`
escapes inside string literals) - and compiling it directly to a flat
binary blob. Codegen is equally minimal: each `print` statement emits its
decoded literal's raw bytes followed by CR LF (`13, 10`), mirroring the
"printable ASCII already means print this character" VDU-byte-stream
reasoning that `examples/hello_golem/program.asm` had previously
hand-documented (see that file's header comment, and
`docs/devlog/2026-07-28.md`'s notes on why a string-literal-only `print`
degenerates to "emit the literal bytes" with no real instruction encoding
needed).

`.golem` was chosen as the source file extension over the shorter `.glm`,
specifically to avoid collision with the well-known GLM (OpenGL
Mathematics) C++ library and other established `.glm` model/survey
formats.

Build: `src/Makefile` builds to the (gitignored) `build/golemc`.

**Verification, in order:**

1. Compiling `examples/hello_golem/hello.golem` with `golemc` produces an
   87-byte output that is byte-for-byte identical to the previously
   hand-authored `examples/hello_golem/program.bin` (confirmed via `xxd`
   and `diff`).
2. `program.bin` was regenerated for real via `golemc` (no longer
   hand-assembled from `program.asm`), then copied into
   `examples/hello_golem/emulator/sdcard/` alongside a freshly rebuilt
   `loader.bin`.
3. Launched `fab-agon-emulator` against that SD card image. Verbose VDP
   logs confirmed the expected sequence:
   ```
   bufferWrite: storing stream into buffer 0, length 87
   bufferWrite: stored stream in buffer 0, length 87, 1 streams stored
   bufferCall: buffer 0
   ```
   (length 87 matching `golemc`'s own reported output size), and the
   Author visually confirmed in the emulator window that the string
   rendered correctly.

This is the first time a real (if toy) Golem compiler has taken `.golem`
source and produced a working, correctly-executing compiled program, start
to finish - as opposed to every prior example in this repo, which was a
hand-authored stand-in for what a compiler would eventually produce.
Committed as `9a2fa72`; `examples/hello_golem/program.asm` was updated to
note it is now superseded by `golemc` and is kept only as documentation of
what `program.bin`'s bytes mean.

Obvious next steps (not started): more statement types (variables,
expressions, control flow), real instruction encoding beyond "raw literal
bytes," and a proper test harness instead of one hand-verified example.

## Detour: would VBA-style syntax fit Golem's leanings?

The Author raised BASIC/VBA (their first "real" language) as a syntax
touchstone, and asked whether it would also fit Golem's design leanings
beyond just syntax - and whether VBA was even a compiled language to begin
with. Worth a short record since it sharpened the type-system rationale in
[docs/design/language-type-proposals.md](../design/language-type-proposals.md).

**Was VBA compiled?** Not ahead-of-time to native code. Classic VBA
compiles to p-code, a bytecode interpreted by a runtime engine embedded in
the host application (Word/Excel/etc.); it cannot run without that hosting
runtime present. (VB6 proper could optionally emit native x86 code, but
VBA specifically always used p-code.) Golem needs the opposite: an
ahead-of-time compiler emitting final buffer-command bytes, with no
runtime engine of any kind resident on the target.

**Does the rest of VBA's model fit Golem's current lean?** Worth
separating VBA's *syntax style* from its *runtime/type semantics* - they
pull in opposite directions:

- **Paradigm** - fine. VBA is imperative/procedural (with some OOP bolted
  on), which matches Golem's leaning toward C/Pascal-like imperative code.
- **Type system** - a real mismatch. VBA defaults to `Variant`, a
  dynamically-typed, runtime-tagged box with implicit coercions and
  runtime dispatch. Golem's current lean is the opposite: Pascal-style
  static, range-checked typing, specifically *because* the VDP is
  write-only and hard to introspect at runtime - the whole point of
  leaning static is to push error detection to compile time, since you
  can't cheaply debug a live buffer machine. A `Variant`-style runtime
  type tag is also awkward to represent at all in a buffer-block machine
  (there's no natural place for the tag to live, nor an instruction to
  check it).
- **Memory model** - VBA relies on COM reference-counting/GC-ish lifetime
  management and dynamic arrays (`ReDim`); Golem's lean is static/arena-
  only, no dynamic allocation. Another mismatch.
- **Abstraction level** - VBA assumes a rich hosted object model
  (`Application`, `Workbook`, `Range`, automation objects) with no analog
  here; Golem's v1 lean is a thin "structured assembler," much lower-level
  than VBA was ever designed to express.

**Conclusion:** VBA itself is too high-level and the wrong semantic shape
(dynamic typing + GC-ish objects) for Golem's machine model. But the
*syntax taste* isn't irrelevant - classic BASIC's statement style (`Dim x
As Integer`, `Sub`/`End Sub`, `If`/`Then`/`End If`) is separable from
VBA's `Variant`/COM baggage. Older statically-typed BASIC dialects
(QuickBASIC-style, later FreeBASIC) pair that same keyword-pair syntax
with explicit static types and no runtime type tag - that combination
would actually fit Golem's current lean reasonably well, if a BASIC-style
keyword syntax (`End If`/`End Sub` over C-style braces) is ever preferred
over the working C/Pascal-flavored sketch. Not a decision, just a noted
option alongside the axes already tracked in
[docs/design/language-type-proposals.md](../design/language-type-proposals.md).
