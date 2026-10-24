#!/usr/bin/env python3
"""Agent-only saved-run verification after the Author closes the emulator."""
import json
from profile import BUILD, active, verify

assert not active(), "Close the owned review emulator before retrieving results"
meta = json.loads((BUILD / "profile.json").read_text())
assert meta["variant"] == "review"
result = verify(meta)
print(f"Review matches {result['frames']} frames, {result['result_bytes']} saved bytes, its completed log and runtime pins.")
