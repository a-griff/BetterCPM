#!/usr/bin/env python3
"""Assemble the TRS-80 Model II BOOT.BIN and STAGE1.BIN package components."""
from __future__ import annotations

import argparse
from pathlib import Path

from build_trs80m2_image import BUILD, build_boot_components

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--assembler", type=Path, default=Path.home() / "bin/z80asm")
    args = parser.parse_args()
    stage1 = build_boot_components(args.assembler)
    print(f"stage one: {len(stage1)} bytes  {BUILD / 'stage1.bin'}")
