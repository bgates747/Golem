# Golem on Agon — start here

Write a Golem source file, compile it on the Agon, then run the resulting program
on the VDP. This guide takes you through those steps without needing another
manual. The supplied compiler and runner need no PC, network, Python or LLM for
everyday use. They work with stock VDP firmware.

## Install the package

1. Unzip the distribution into a temporary directory on your computer. Copy the
   **contents of its `sdcard` folder** into your Agon SD card's root, merging
   directories. For an emulator, use its virtual SD card directory. Do not copy
   the enclosing folder: you do not want `sdcard/sdcard/`.
2. Check that the card now contains `/mos/golemc.bin`, `/bin/golemrun.bin` and
   `/golem/`. The `/golem/compiler/` directory contains four compiler workers;
   leave them there. The small `golemc` command chooses the right worker for you.
3. Use MOS 3.0.2 with stock VDP. A VDPlet-enabled VDP also works for these stock
   examples; custom firmware is not required. Files copied from Linux become
   available in the running emulator without restarting it.
4. At the **MOS prompt inside the Agon or emulator**, enter:

```text
cd /golem
golemc --help
```

You should see the compiler's command forms. From here onward, enter commands
in MOS, not a Linux terminal. Every exercise below starts with `cd /golem`, so
you can try them independently. Use an Agon text editor to read or edit the
`.golem` examples. In an emulator you can also edit their files from Linux.

## What compiles, and what runs?

`golemc` reads Golem source and writes output files to the SD card. Compilation
does not execute your program or put it into VDP memory.

`golemrun` reads that output, sends it to the VDP, runs it once and returns to
MOS. The eZ80 runs these tools; the compiled Golem work runs on the VDP.

A **profile** selects an implemented part of the language and its output format.
The current profiles share syntax conventions, but you cannot freely mix all
of their features in one source file.

| Compiler option | Meaning | Output and runner |
| --- | --- | --- |
| No profile option | Simple text and counted-loop programs | A `.bin` command stream; run with `golemrun file.bin` |
| `--resident` | A program designed to stay installed in VDP buffers and expose named `Sub` procedures for repeated calls | A directory of installation, call, cleanup and layout files; try once with `golemrun --resident directory` |
| `--portable` | Resident calculations: typed arithmetic, conditions, loops, integer arrays and printing | A directory of state/backend files; run its stock implementation with `golemrun --portable directory` |
| `--stock-matrix` | Matrix and vertex-buffer operations using stock VDP commands | A directory for integration into your own application; this runner has no matrix-profile option |

**Resident means retained on the VDP, not running forever.** A `Sub` is a named
procedure: it does a finite piece of work and returns. An application can install
a resident package once, update its inputs and call its Subs repeatedly. Global
variables keep their values while installed. However, the supplied standalone
`golemrun --resident` deliberately installs the package, calls its first Sub
once, then removes it. Run it again to start afresh. Keeping it installed between
calls is something your own ASM/C application controls.

The `--resident` switch on the runner tells it which files to load; it is not a
request to leave them installed. Match the runner option to the compiler option.

**Portable means the same source can target stock buffered commands or native
ESP32 code.** The stock path works on normal VDP firmware; the native path needs
a separately built VDPlet and supporting firmware. `--portable` normally emits
both implementations for supported operations; `golemrun --portable` chooses the
stock one. Portable programs can also stay resident under your application's
control: portable and resident are not opposite lifetimes. Today the two flags
select different implemented language profiles. Printing currently has a stock
implementation only, so programs using print produce stock output by default.
You do not need a VDPlet or custom firmware to print numbers.


## Exercise 1: Hello, world

The entire source in `examples/hello.golem` is:

```golem
print "Hello, world!"
```

Enter these three commands in MOS:

```text
cd /golem
golemc examples/hello.golem output/hello.bin
golemrun output/hello.bin
```

`cd` selects the working directory. The next command compiles the source into
`output/hello.bin`. The last command runs it. You should see **Hello, world!**
and then the MOS prompt. Print does not append a newline, so the prompt can
follow the greeting on the same line.

Change the greeting in the source, save it, then repeat the compile and run
commands. Running without recompiling uses the previous compiled version.
Recompiling the same path replaces its previous output.

The compiler also writes `output/hello.bin.cleanup.bin`. Keep it beside the
program: the runner uses it to release the program's VDP buffers after running.
It does not erase the text already displayed. You can rerun the example.

## Exercise 2: Draw a cube

Enter:

```text
cd /golem
VDU 22 19
golemc --resident examples/cube.golem output/cube
golemrun --resident output/cube
```

`VDU 22 19` selects screen mode 19: 1024x768. Changing screen mode clears the
screen. `output/cube` is an output **directory**, not a single executable file.

You should see a white wireframe cube: two offset squares joined at their
corners. All twelve lines are specified in `examples/cube.golem`, inside
`Sub drawCube`. The runner supplies pixel coordinates, installs the program,
calls that Sub and cleans up its buffers. The picture and screen mode remain
after it returns to MOS. Repeating the run draws the same lines again.

The supplied `reference-image.png` shows the cube with the optional review
script's labels. Those labels are supplied by the script, not hidden Golem
statements.

## Exercise 3: Calculate and print the result

Enter:

```text
cd /golem
golemc --portable examples/counter.golem output/counter-print
golemrun --portable output/counter-print
```

You should see **i=10 total=55**, followed by the MOS prompt. The program declares
`uint16` globals, adds 1 through 10 in `Sub accumulate`, then prints:

```golem
    print "i="
    print str(i)
    print " total="
    print str(total)
```

`str(i)` converts the integer to decimal text. Print emits that text. Each print
has exactly one operand; consecutive statements put their output next to each
other. There are no automatic spaces or newlines: the space before `total=` is
inside its literal. You may put those four statements on one line if you prefer.
The standalone runner adds its normal final newline before returning to MOS.

Printing supports uint16, sint16, uint32 and sint32, including negative values
and signed minima. Float32 printing is described next; bool is not supported.
Printing itself leaves the variable unchanged. A failed numeric expression emits
nothing for that print; earlier text stays on screen.

This produces a **stock-only resident package**: look for buffered-install.bin,
common-install.bin and calls.inc, rather than native.c. Your own ASM/C application
can install it once and call it repeatedly without reading results back to the
eZ80 for formatting. In that use, a second accumulate call prints total=110.
The standalone runner removes its state, so rerunning these commands starts at
zero and prints 55 again.

The array example also prints its result:

```text
cd /golem
golemc --portable examples/arrays.golem output/arrays-print
golemrun --portable output/arrays-print
```

Expect **value=4294967294**. It selects a uint32 array element and applies XOR
with 1. Read the source to see the explicit typed array and integer expression.
The new output-directory names above also avoid old dual-output packages from
an earlier version of these examples.

## Printing floating-point results

Compile and run the supplied example in the same way:

```text
cd /golem
golemc --portable examples/float-print.golem output/float-print
golemrun --portable output/float-print
```

Expect **reading=1.5 scaled=3**. Its complete source is:

```golem
float32 reading = 1.5
float32 scale = 2.0
Sub show
    print "reading=" print str(reading)
    print " scaled=" print str(reading * scale)
EndSub
```

All conversion and printing run in resident buffers on stock VDP. The eZ80
runner loads and calls them; it does not convert the result to text. Float32
output uses nine significant digits, removes trailing fractional zeros and uses
scientific notation outside decimal exponents -4 through 8. Thus `1.5` stays
`1.5`, while a billion prints `1e+09`. It describes the stored binary32 value:
an initializer of `0.1` prints `0.100000001`. Original input spelling is not
preserved by a floating-point variable. Precision options and fixed-point types
are not available in this profile yet.

## Integer bitwise operators

The calculation profile supports `&` (AND), `|` (OR), `^` (XOR) and unary `~`
(complement). These operate on integer bits, rather than Boolean true/false.
Use matching integer types; no conversion is implicit. The supplied
`examples/bitwise.golem` contains:

```golem
uint16 flags = 5
uint16 mask = 3
uint16 both = 0
uint16 either = 0
uint16 different = 0
uint16 inverted = 0
Sub inspectBits
    both = flags & mask
    either = flags | mask
    different = flags ^ mask
    inverted = ~flags
    print "and=" print str(both)
    print " or=" print str(either)
    print " xor=" print str(different)
    print " not=" print str(inverted)
EndSub
```

The calculated values are 1, 7, 6 and 65530. `~` flips exactly 16 bits for uint16;
it does not mean arithmetic minus. The same operators work with signed and
unsigned 32-bit integers and signed 16-bit integers. Boolean `And`, `Or` and
`Not` remain separate. Full precedence rules are in `LANGUAGE.md`.
The supplied source prints these four results with labels. Try:

```text
cd /golem
golemc --portable examples/bitwise.golem output/bitwise-print
golemrun --portable output/bitwise-print
```

Expect **and=1 or=7 xor=6 not=65530**. All formatting and printing happen on the
stock VDP.

## Exercise 4: Compile a record/table example

Enter:

```text
cd /golem
golemc --resident --table-data samples=examples/records.dat examples/table.golem output/table
golemrun --resident output/table
```

`--table-data samples=examples/records.dat` tells the **compiler** to fill the
source's `samples` table with packed records from that file. It is a build-time
input, not a request for the VDP to open a file. Keep `records.dat` with the
example source.

The program selects record 2 into `selected`. It has no print or drawing
statement, so you should see the runner return to MOS without a picture or
numerical result. As with the cube, `--resident` makes an installable package;
the standalone runner performs one call and cleans up.

## Further examples: compile for your own application

These examples produce integration files; they are not additional programs
that the stock runner can display for you.

### Native mathematics

```text
cd /golem
golemc --portable --target native examples/trig.golem output/trig
```

`--target native` requests C source for a native VDPlet instead of stock buffered
commands. This produces `output/trig/native.c`; it does **not** produce a runnable
VDPlet. Float division, `sin`, `cos` and `floor` in this portable example currently
require that native target. Its source converts degrees to radians before `sin`.

To execute it, use a separate ESP32 cross-toolchain to compile/package the C,
then load and call the resulting VDPlet using VDPlet-enabled firmware. That step
is outside this stock-VDP exercise. `golemrun` cannot execute a C source file.

### Matrices and borrowed vertex buffers

```text
cd /golem
golemc --stock-matrix --reserve 1000:1001 examples/matrix.golem output/matrix
```

`--reserve 1000:1001` sets aside both buffer IDs, inclusively, so the compiler
will not allocate them for its own code or state. This example uses those two
buffers for geometry owned by the calling application. Your application must
install their contents before calling the matrix program.

The generated `golem.inc` contains named assembly entry points and buffer-layout
symbols for that integration. Compilation alone does not install the geometry;
the standalone runner has no `--stock-matrix` mode.

## Longer commands and filenames with spaces

A response file is a text file containing compiler options and arguments. It
avoids MOS's small command-argument limit. Try the supplied example:

```text
cd /golem
golemc @examples/build.args
```

The `@` tells the compiler to read arguments from that file. Options and paths
are separated by whitespace; double quotes group a filename containing spaces.
Use an ordinary no-space filename for the response file itself. Paths inside it
resolve against the **current MOS directory**, not the response file's directory.
Put the profile option first, as in the command examples above. Response input
is limited to 8192 bytes and 127 arguments.

## Optional combined drawing demonstration

Enter:

```text
EXEC /golem/examples/review.txt
```

This MOS script compiles and runs Hello World and the white cube. It selects
mode 19 and leaves the picture visible, with MOS text confined to a bottom area
so directory listings do not scroll the cube. When its instruction appears,
type `dir` to check that MOS still responds. The script does not replace your
autoexec file. It can be run again.

## If something goes wrong

1. If MOS cannot find `golemc` or `golemrun`, check the installation paths in
   the first section. The compiler workers belong under `/golem/compiler/`.
2. If a source or data file cannot be opened, enter `cd /golem` and check its
   name with `dir`. Relative paths in this guide start there.
3. If compilation reports an error, correct the source or options before
   running. Files left from an earlier successful compilation may still exist.
4. If the compiler says target selection changed, choose a fresh output
   directory. Adding print changes an ordinary dual-output package to stock-only.
5. If the compiler rejects a feature, check the selected profile in `LANGUAGE.md`.
   A feature shown for one profile is not automatically available in another.
6. The native compiler has finite working RAM. Larger programs that generate
   many resident helpers can exhaust it even when the source is short. The
   two-float example above fits; larger programs may need the PC compiler.
   Exhaustion produces a compiler error without reducing numeric precision.
7. If the runner reports a VDP timeout, reset the Agon or restart the emulator
   before rerunning. A timed-out program may still own active VDP buffers.

## Using the output from ASM or C

For your own application, loading, input updates, calls, timing and cleanup
belong to that application. The compiler publishes entry points, buffer IDs and
state layouts. Reserve buffer IDs used by other parts of your application when
compiling. The standalone runner uses buffer 0 temporarily for loading; do not
use it alongside an application that already owns that buffer.

Finish execution and drawing before modifying or removing live resources. The
standalone runner handles its completion waits for these exercises; custom
applications need their own completion and frame-pacing arrangements.

## Reference and rebuilding

`LANGUAGE.md` is the complete reference for the currently implemented profiles.
The `reference/` directory contains more detailed integration contracts.

`REBUILD.md` at the ZIP's top level explains how to rebuild the compiler and
runner from the included sources with AgonDev on a development computer.
Rebuilding these tools is separate from compiling your own Golem programs on
the Agon; it is not needed for any of the stock exercises above.
