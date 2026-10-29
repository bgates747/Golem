# Resident float32 printing

Golem now prints float32 values entirely through stock resident buffered
commands. The same feature compiles on an Agon. Numeric correctness checks and the Author's visual/keyboard review pass;
the Author has explicitly approved publication. No VDPlet or firmware
change is needed, and string concatenation remains separate.

```golem
float32 reading = 1.5
Sub show
    print "reading=" print str(reading)
EndSub
```

This prints `reading=1.5`. Conversion uses nine significant digits, ties-to-even
rounding and ordinary decimal notation for exponents -4 through 8. Otherwise it
uses scientific notation. Trailing fractional zeros are removed; zero is `0`.
It formats the stored value rather than preserving its original spelling:
float32 `0.1` prints `0.100000001`. See the [task contract](../GOLEM-054.md).

The [review instructions](REVIEW.md) give the Oryx launch command. Its source is
[review.golem](review.golem): Golem prints fourteen supplied numbers; the C
client installs, updates inputs, calls the entry and separates rows. Reference
screens belong only to automated qualification. The agent retrieves saved
results after the Author closes the emulator.

## Verification

1. [Exact-rational oracle](oracle.py): 1,934 values, every normal binary exponent,
   signed extrema, rounding ties/carries, notation boundaries and repeated calls.
2. [Independent decoder](model-evidence.json): 3,958 calls across default and
   relocated packages; 197,817,640 decoded stock commands. Checks cover expression
   faults, admission, source preservation, cleared print sites, recovery and
   cleanup. The host reference independently matches all1,934 values.
3. [Pi actual-MOS qualification](stock-evidence.json) and
   [Oryx qualification](stock-oryx-evidence.json): each compiles the one-input
   program on the emulated eZ80 and matches all11 package files to desktop output.
   Each checks72 full-screen text pairs/22,118,400 pixels and576 saved state/error
   bytes, followed by normal MOS keyboard continuation. A separate relocated
   [stock run](stock-relocated-evidence.json) also passes.
4. Maintained checks and [ASan/UBSan](sanitizer-evidence.json) pass. Original
   integer executable outputs are unchanged; [legacy parity](legacy-parity.json)
   records one stale historical schema-only metadata difference that already
   existed before this tranche. Original task evidence was preserved.

## Storage and compilation limits

The exact decimal coefficient needs at most112 digits;113 bytes provide a guard.
Finite tail loops perform at most149 scaling rounds. Shared formatter output
uses16 bytes; each print site copies, consumes and clears its own16-byte output.
There is no general string heap or input-dependent call-stack growth.

A minimal float-print package installs17,571 buffered bytes plus130 common
bytes. Its207 private buffers contain14,666 bytes of code/data payload, including
1,061 data bytes. A second float print site adds401 installation bytes and26
data bytes. These payload measurements exclude firmware allocation/map overhead;
see [resource evidence](resource-evidence.json). Calls use the existing named
resident entry mechanism; conversion needs no numeric readback to the eZ80.

The native Agon compiler worker is319,489 bytes. The first tutorial version
with two float globals, arithmetic and two float print sites exceeded its
remaining working RAM while building stock wrappers. It returned an ordinary
compiler error; [the failed attempt](distribution-initial-memory-limit.json)
is preserved. The distributed example uses the proven one-input form. More
complex programs can use the desktop compiler; the larger expression fixture
passes independent execution checks. No allocator redesign is included here.

The standalone ZIP, how-to and language reference are refreshed together.
Fixed-point types/formatting, configurable precision and native text output
remain follow-ups. Human review passed; the Author explicitly approved commit and push.

The [human review receipt](human-evidence.json) verifies fourteen rows and112
saved bytes. The screenshot confirms correct text, PASS and a working MOS dir;
the emulator is stopped.
