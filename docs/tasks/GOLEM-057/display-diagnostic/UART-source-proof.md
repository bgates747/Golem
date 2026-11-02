# UART0 binary capture: source evidence

The unchanged interpreter backend already writes UART0 bytes to a binary file,
including zero and high-bit bytes. No emulator modification is required to
collect this diagnostic. This is source evidence only; a future authorized run
must still verify the retained file and transaction context.

The read-only inspected closure is
`/Agon/mystuff/flexe-agon-emulator/build/fx072/origin-build/origin-r01-bundle-r02.tar`.

| Member/artifact | SHA-256 |
| --- | --- |
| Bundle | `8df65029497cb86e623fd4deb413c94e9c9e9523b518f387ab5c7775b0b04b48` |
| `candidate-interp-origin.so` | `140e86bc5bef907996f93e067f94eec1ce5ac85d77169fad4e7a12777597e734` |
| `adapter/flexe_vdp.c` | `40a2d87afcfe0b575a2d0a45f87fba6993125f4ec18239adc9a40520b2e6aece` |
| `core-source/src/flexe_session.c` | `4598855158f5499d2238c1d1edcafc746ae821f3d8f31cfd0a5d4e008eed5dc3` |
| `core-source/src/peripherals.c` | `9905994d4b65ec50c45f6da63e2e3f7ba821d9d6554d612c57842ed60d06564e` |

`flexe_session.c:209–210` installs the configured UART callback.
`peripherals.c:16616–16617` selects UART0; `:2844–2852` passes each byte to the
callback independently of its capped 4,096-byte internal retained TX buffer.
The adapter selects `AGON_FLEXE_ARTIFACTS/esp32-uart0.log` at `:253–258`, opens
the file with `"wb"` at `:100–108`, and uses `fputc` per byte at `:160–163`.
Debug logging controls only the additional stderr copy. Ordinary loop checks
flush after 250 ms (`:345–351`), and normal teardown closes the stream (`:380`).

For the eventual run, use a fresh **existing** artifact directory; preserve and
hash the complete `esp32-uart0.log`; read it as bytes after normal stop/join.
Scan for complete journal records at arbitrary offsets, retaining unrelated
boot/console bytes. The parser in this folder never establishes transaction
completion from a UART prefix alone.

Missing `AGON_FLEXE_ARTIFACTS` silently disables capture. The adapter does not
create the directory and truncates an existing log. Write/flush/close errors are
unchecked; the available UART statistics count UART2, not UART0. Abrupt stopping
can lose buffered suffix bytes. Peripheral transmission is modeled as immediate
through a zero-depth FIFO: the proposal's 115200-baud wire-time estimate is not
an emulator elapsed-time bound. No new emulator execution was performed.
