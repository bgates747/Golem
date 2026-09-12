# Stock resident admission transport

This MOS program uploads opaque Golem compiler output, sends a bounded fixture
stream, and reads back active state/status/result buffers through tagged GP bytes.
It performs no tested validation, record selection or arithmetic on eZ80.
The real admission program and fixture generator are in the Rally execution
worktree's `docs/tasks/RALLY-19/admission.golem` and `probe_admission.py`.

Run there with its project Python:

```sh
.venv/bin/python docs/tasks/RALLY-19/probe_admission.py UNIQUE_OVAL
.venv/bin/python docs/tasks/RALLY-19/probe_admission.py UNIQUE_FUJI --track fuji
```

Every run is stock, normal-clock, isolated and headless. The fixture format is
`G19A`, LE u16 action count (1..512), then action records: u8 kind (0 UART bytes,
1 reload), LE u16 byte length (0..97), LE u16 diagnostic idle ticks (0..600),
then exactly that many bytes. Reload actions have no payload. Trailing bytes,
short reads or invalid bounds fail. Results are 96 bytes/action: active80,
status10, expected-sequence2, numerical-proof4. `exit.txt` records the guest code.
Raw timeout fixtures deliberately pause; this is diagnostic recovery, not the
production protocol. GP is not a rendering-completion fence.
