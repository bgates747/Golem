# Native scalar quantization

This hosted kernel accepts a raw binary32 scalar in buffer1000 and returns
truncation toward zero and nearest-integer rounding with half ties away from
zero as binary32 values at output1001 offsets0 and4. Both operations occur inside
the stock VDP; the eZ80 loader transports input and tagged-byte readback.

```sh
make -C src
build/golemc --hosted examples/native_quantization/kernel.golem build/quantization.bin
.venv/bin/python tests/run_hosted.py
cd /home/smith/Agon/mystuff/AgonArcade-rally19
.venv/bin/python docs/tasks/RALLY-19/probe_rounding.py UNIQUE
```

The native driver generates an equivalent diagnostic layout, sends122 varied
values and compares actual returned bytes. It includes both signed zeros, small
magnitudes, half-integer neighbours, signed word and large integer boundaries.
`evidence/golem-rounding/signed-boundaries` passes on stock Fab1.2.4/MOS3.0.2 at
normal clock/UART with dummy SDL. GP means parser echo, not raster completion.
All emulator work is headless.

The compiler copies magnitude/sign bits, sends only nonnegative values to stock
unsigned fixed conversion, reloads the integer as float and restores the sign.
MatTrunc accepts finite magnitude<=16777215. MatRoundAway accepts<=8388607; it
adds the predecessor of binary32 0.5 to the magnitude before conversion. This
prevents nextafter(0.5,0)+0.5 rounding prematurely to1. The host test checks every
half-integer discontinuity and its adjacent binary32 values across that entire
domain; monotonic addition covers intervening values. Distinct scalar matrices
are required. Neither operation invokes a negative-float-to-unsigned cast.
Use typed field Copy or StoreI16Floor for an appropriate destination.

These operations were needed because deferring four integer truncations in
Rally's evaluator passed <=1px centre checks but failed a thin shoulder's separate
native pixel metric. Full-section/full-game evidence remains in Rally's R19 task
bucket; this small example is not frame-rate proof.
