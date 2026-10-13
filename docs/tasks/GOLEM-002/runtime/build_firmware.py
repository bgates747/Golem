#!/usr/bin/env python3
"""Compatibility entry point for the relocated, maintained hardware builder."""
from pathlib import Path
import importlib.util

HERE = Path(__file__).resolve().parent
SDK = HERE.parents[4] / 'agon-vdplet/tools/build_firmware.py'
spec = importlib.util.spec_from_file_location('vdplet_firmware_builder', SDK)
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)
prepare = builder.prepare
PIN = builder.PIN
GL_PIN = builder.GL_PIN
if __name__ == '__main__':
    builder.main()
