# Protocol1 bounded-reader timeout clarification

The wire adapter uses the existing stock timed stream readers. A truncated
header or payload has no sink effects and emits no reply; already consumed
operand bytes are not replayed through the command interpreter. It does not
introduce persistent framing recovery after the stock command timeout.

If missing bytes arrive after that timeout, the stock outer interpreter can
consume them as later input. A producer must send a complete small request as
one uninterrupted command, and treat a transport timeout/disconnect as lost
framing requiring controlled application/VDP recovery. Do not append the rest
of an expired request or blindly retry WRITE. This is a transport limit shared
with existing stock variable-length commands, not a claim of stream quarantine.

This note changes no frozen byte layout, command allocation or service effects.
The implementation report and target matrix retain truncation evidence. A new
resumable UART framing layer is outside this optional small adapter.
