#!/usr/bin/env python3
"""Assemble the TRS-80 Model II platform extension (F600H-F7FFH).

The extension links to the clock handler and console output in the Model II
BIOS, so build the BIOS for BETTERCPM_PLATFORM=trs80m2 first (build_system
does). The system package carries the result in slot 1; stage one loads it.
"""
from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

import build_platform
if not build_platform.MODEL2:
    raise SystemExit("set BETTERCPM_PLATFORM=trs80m2")

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/platform/trs80m2"
BIOS_LISTING = ROOT / "build/bios/bios.lst"
OUTPUT = ROOT / "build/trs80m2/m2ext.bin"
BASE = 0xF600
CODE_LIMIT = 0x1C0          # the receive ring occupies F7C0H-F7FFH
SLOT_BYTES = 512            # one physical sector; package slot 1 holds it
BIOS_SYMBOLS = ("MC_ISR", "MH_OUT")


def bios_symbols() -> str:
    listing = BIOS_LISTING.read_text(errors="replace")
    lines = []
    for name in BIOS_SYMBOLS:
        match = re.search(rf"^([0-9a-f]{{4}})\s+.*?\b{name}:", listing, re.M | re.I)
        if not match:
            raise SystemExit(f"{name} not in {BIOS_LISTING}; build the Model II BIOS first")
        lines.append(f"{name:<16}EQU     0{match.group(1).upper()}H")
    return "\n".join(lines) + "\n"


def build(assembler: Path) -> bytes:
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    text = (SOURCE / "m2ext.mac").read_text(encoding="ascii")
    text = text.replace("        CSEG\n        .PHASE  ", "        ASEG\n        ORG     ")
    text = bios_symbols() + text.replace("        .DEPHASE\n", "")
    with tempfile.TemporaryDirectory(prefix="bettercpm-m2ext-") as temporary:
        staged = Path(temporary)
        (staged / "m2ext.mac").write_text(text, encoding="ascii")
        shutil.copy2(SOURCE / "hardware.inc", staged / "hardware.inc")
        subprocess.run([str(assembler), "-fb", f"-o{OUTPUT}",
                        f"-l{OUTPUT.with_suffix('.lst')}", "m2ext.mac"],
                       check=True, cwd=staged, stdout=subprocess.DEVNULL)
    data = OUTPUT.read_bytes()
    if len(data) > BASE and data[:BASE] == bytes(BASE):
        data = data[BASE:]
    if not 0 < len(data) <= CODE_LIMIT:
        raise SystemExit(f"platform extension is {len(data)} bytes; limit {CODE_LIMIT}")
    data = data.ljust(SLOT_BYTES, b"\0")
    OUTPUT.write_bytes(data)
    return data


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--assembler", type=Path, default=Path("/Users/nathanael/bin/z80asm"))
    args = parser.parse_args()
    data = build(args.assembler)
    used = len(data.rstrip(b"\0"))
    print(f"Model II platform extension: {used} / {CODE_LIMIT} bytes")


if __name__ == "__main__":
    main()
