# Native compiler RAM qualification

The unchanged two-float example now compiles on the Agon and produces exactly
the same package as the desktop compiler. Ordinary commands, direct worker
calls and response files all pass. The resident code prints the inputs and
their product on stock VDP. The dedicated Oryx review is prepared and stopped;
the Author accepted the test and explicitly approved commit/push on 2026-10-30.

The Author's first screenshot and [retrieved results](human-review.json) confirm
the expected calculations and working MOS keyboard, but revealed the review
client left the text cursor disabled. Its common exit path now enables the
cursor on success and graceful errors. [Headless cursor checks](cursor-evidence.json)
confirm blinking after both returns. The Oryx review is refreshed with this fix.

See the [task plan](../GOLEM-055.md), [memory analysis](memory-analysis.md),
[exact source](two-floats.golem) and [review guide](REVIEW.md).

| Observation | Result |
| --- | --- |
| Frozen accepted worker | Fails the original 32,768-byte wrapper allocation; returns to MOS |
| Final production worker | 319,785 bytes; SHA-256 `0e9d835b7c9ac4e7feda2ee280e3bbb9497040ad7ed96171d38857a3482973bf` |
| Ordinary dispatcher, direct worker, optional response file | All eleven files match the frozen desktop reference for each path |
| Mixed uint16/float32 control | Eleven buffered and twelve native-only output files match desktop references |
| Actual stock-VDP execution | Five expected/Golem framebuffer pairs; 1,536,000 pixels and sixty saved input/status bytes match on each host |
| Repeated resident call and changed inputs | Correct products, preserved input bytes and successful status |
| Unchanged distribution runner | Displays `reading=1.5 scaled=3` with an exact text pixel match |
| Rejected mixed-type assignment | Native compiler exit 1, expected diagnostic and working MOS continuation |
| Native response boundaries | An 8,192-byte response compiles; 8,193 bytes reject; quoted paths compile |
| Existing review instance | GOLEM-054 runtime pins remain unchanged |

The numerical/scene checks run on actual Fab Agon Emulator 1.2.5 with tagged
MOS 3.0.2 and the stock native VDP module. Native-only control output is compared
as generated code; no VDPlet is installed or executed by these tests. No
graphical emulator was launched and no physical target was flashed.

The full small proof takes about 45 seconds on both Pi and Oryx; that includes
multiple compiler invocations, framebuffer comparisons, runner installation and
keyboard injection. The separate native response boundary test takes about
8 seconds on Pi. The human-scene headless run takes about 12 seconds on Oryx.
These are complete harness durations, not isolated compiler benchmarks.

Evidence:

- [Original failure reproduction](baseline-evidence.json) and [screen](baseline-failure.png).
- [Pi qualification](stock-evidence.json), [Oryx qualification](stock-oryx-evidence.json)
  and [native response cases](native-response-evidence.json).
- [Headless review scene](review-headless-evidence.json), [reference image](review-reference.png)
  and [final stopped/reset profile verification](staging-verification.json).
- [Earlier optional-response failure](response-before-fix.json), retained separately
  from the final successful result.

The diagnostic heap probes add code and static data. Both measured probes fail
the original large request, while the smaller final production worker succeeds.
Their live-buffer/free-list observations support the diagnosis; they do not
claim a successful fixed-probe run or an exact uninstrumented peak. Finite Agon
RAM still limits larger compilations.
