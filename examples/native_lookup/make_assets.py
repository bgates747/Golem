"""Deterministic editable fixture for native resident-record lookup."""
from pathlib import Path
import struct
ROOT=Path(__file__).resolve().parent
(ROOT/'records.dat').write_bytes(b''.join(struct.pack('<ii',a,b) for a,b in [(7,9),(-4,6),(31,2),(80,1)]))
(ROOT/'bytes.dat').write_bytes(bytes(range(256)))
