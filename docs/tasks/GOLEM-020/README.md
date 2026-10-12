# Wolf bitmap animation

Human review passed: the Author reported success, and the retrieved run matches
all 841 checks / 946 bytes. This finite, vblank-paced animation uses the existing
resident Golem drawing backend. Golem clears and draws every frame; assembly advances bouncing positions
and drives presentation. No bitmap pixels are uploaded per frame.

## Run on Oryx

```bash
cd ~/Agon/mystuff/golem/.emulator
rm -f sdcard/golem-020/results.dat
SDL_VIDEODRIVER=x11 ./fab-agon-emulator --renderer sw
```

BJ, the pistol and the inventory icon bounce independently over the familiar
Wolf splash, all at their original sizes. Labels identify Golem and assembly.
The animation runs for 600 frames, then returns to MOS. Qualified Oryx execution
took about 22 seconds including setup and final observations. Follow the onscreen dir instruction, close the emulator and tell the
agent whether the animation and keyboard worked. The agent retrieves the results.
No second terminal or manual verifier command is needed.

## Read the whole program

[animation.golem](animation.golem) is the complete per-frame Golem routine:
clear the playfield, draw the splash, draw the three moving images.
[animation.asm](animation.asm) is the assembly application: load the AGNB once,
install Golem once, reflect positions at the walls, update twelve packed bytes,
call golem_drawFrame and present at VSYNC. The stock screen swap supplies the
vblank pacing; UART flow control preserves the ordered stream.
The source currently uses assembly motion because the resident drawing profile
does not yet implement assignments or conditionals. No unsupported Golem syntax
is implied by this demonstration.

The playfield is 320×192. Stock mode 136 provides 320×240, 64 colours and double
buffering. Labels occupy the bottom 48 pixels of both buffers. Stock VDU 23,0,195
swaps at VSYNC. There is no additional MOS-side vblank wait, completion acknowledgement or
CPU catch-up timer. Using the screen swap as the sole clock avoids waiting
twice for a frame boundary.
The final scene is redrawn in single-buffered mode 8, so MOS/dir output is visible
without further swaps. All image drawing and playfield clearing come from Golem;
assembly also prints labels and performs pixel observations.

Per frame: eleven-byte buffer update header + twelve position bytes + six-byte
Golem call + three-byte swap = **32 wire bytes**, excluding one-time setup and
bounded review observations. Golem installation is 360 bytes, in seven owned
buffers. Assets use the unchanged GOLEM-019 AGNB and are borrowed, not cleared
by Golem cleanup. This is an integration demonstration, not a performance claim.

## Qualification

`prepare.py` builds the exact human program, then runs it headlessly via the
project-local launcher. At frames 1, 90, 180, 270, 360, 450 and 600 it records
actual assembly position state for an independent reflection model. Pixel
observations run only after the animation, comparing its final presented frame
and single-buffered redraw with an independent RGBA2222 model. No pixel-probe
pauses interrupt the moving scene. Setup and final observations add to the
nominal ten-second animation duration. No direct bitmap comparison scenes are shown.

```bash
cd ~/Agon/mystuff/golem
.venv/bin/python docs/tasks/GOLEM-020/prepare.py
.venv/bin/python docs/tasks/GOLEM-020/prepare_oryx.py
```

`checks.json` records local evidence; `oryx-review.json` records staging and remote
headless verification. Both runs leave the graphical emulator unlaunched. Human review passed, and the Author explicitly approved committing and pushing
the reviewed work.
