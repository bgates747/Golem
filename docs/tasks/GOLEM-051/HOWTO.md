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
| `--portable` | Typed arithmetic, conditionals, finite loops and integer arrays, with stock and native implementations where supported | A directory containing shared state and backend files; try its stock implementation with `golemrun --portable directory` |
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
select different implemented language profiles.


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

## Exercise 3: Compile and run a calculation

Enter:

```text
cd /golem
golemc --portable examples/counter.golem output/counter
golemrun --portable output/counter
```

This example declares `uint16` globals and a `Sub accumulate` that adds 1 through
10. The calculated total is 55. **It prints no numerical result and draws
nothing**: expect the runner's message followed by the MOS prompt. The current
portable profile has no print statement for displaying that variable.

The generated directory contains state/layout descriptions and both stock
buffered commands and native C source. This runner uses the stock implementation,
calls the first Sub once and removes its state afterward. An ASM/C application
can instead own the state and preserve it between calls.

The array example can be tried the same way:

```text
cd /golem
golemc --portable examples/arrays.golem output/arrays
golemrun --portable output/arrays
```

It selects an element of an explicitly typed integer array and applies bitwise
XOR. It also has no visible numerical output. Read its source to see the syntax;
these calculation examples introduce compilation and integration, rather than
an interactive results viewer.

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
4. If the compiler rejects a feature, check the selected profile in `LANGUAGE.md`.
   A feature shown for one profile is not automatically available in another.
5. Large programs can exhaust the Agon's RAM earlier than on a PC. The compiler
   reports exhaustion rather than reducing numeric precision silently.
6. If the runner reports a VDP timeout, reset the Agon or restart the emulator
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
