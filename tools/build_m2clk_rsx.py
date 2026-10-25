#!/usr/bin/env python3
"""Build the TRS-80 Model II TIME provider (M2CLK.RSX) as a BRSX carrier.

The clock itself lives in the Model II BIOS (src/platform/trs80m2/m2clock.inc);
this provider links to its fixed addresses from the BIOS listing, so build the
BIOS for BETTERCPM_PLATFORM=trs80m2 first (build_system does).
"""
from __future__ import annotations

import argparse
import hashlib
import re
from pathlib import Path

import build_platform
if not build_platform.MODEL2:
    raise SystemExit("set BETTERCPM_PLATFORM=trs80m2")
from build_cpx_module import relocation_offsets
from build_ccp import assemble
from build_rsx_module import make_module

from component_registry import module_version

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/platform/trs80m2/m2clk.mac"
BIOS_LISTING = ROOT / "build/bios/bios.lst"
BUILD = ROOT / "build/rsx"
LINK_BASE = 0x8000
ALTERNATE_BASE = 0x8101
BIOS_SYMBOLS = ("MC_CLK", "MC_TICK", "MC_RUN", "MC_VEC")


def bios_symbols() -> str:
    listing = BIOS_LISTING.read_text(errors="replace")
    lines = []
    for name in BIOS_SYMBOLS:
        match = re.search(rf"^([0-9a-f]{{4}})\s+.*?\b{name}:", listing, re.M | re.I)
        if not match:
            raise SystemExit(f"{name} not in {BIOS_LISTING}; build the Model II BIOS first")
        lines.append(f"{name:<16}EQU     0{match.group(1).upper()}H")
    hz = re.search(r"^\s*([0-9a-f]{4})\s+.*?\bMC_HZ\s+EQU", listing, re.M | re.I)
    if not hz:
        raise SystemExit(f"MC_HZ not in {BIOS_LISTING}")
    lines.append(f"{'MC_HZ':<16}EQU     0{hz.group(1).upper()}H")
    return "\n".join(lines) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--assembler", type=Path,
                        default=Path("/Users/nathanael/bin/z80asm"))
    args = parser.parse_args()
    BUILD.mkdir(parents=True, exist_ok=True)
    source = bios_symbols() + SOURCE.read_text(encoding="ascii")
    text = source.replace("        CSEG\n        .PHASE  ",
                          "        ASEG\n        ORG     ").replace(
                              "        .DEPHASE\n", "")
    code = assemble(args.assembler, text, BUILD / "m2clk.bin",
                    BUILD / "m2clk.lst", LINK_BASE)
    alternate = assemble(
        args.assembler,
        text.replace("RSXBASE         EQU     08000H",
                     "RSXBASE         EQU     08101H"),
        BUILD / "m2clk-alt.bin", BUILD / "m2clk-alt.lst", ALTERNATE_BASE)
    relocations = relocation_offsets(code, alternate,
                                     ALTERNATE_BASE - LINK_BASE)
    listing = (BUILD / "m2clk.lst").read_text(errors="replace")
    service = int(re.search(r"^([0-9a-f]{4}).*\bMK_SERVICE:", listing,
                            re.M | re.I).group(1), 16) - LINK_BASE
    carrier = make_module(name="M2CLK", version=module_version("M2CLK"), services=[],
                          linked_base=LINK_BASE, code=code,
                          relocations=relocations, entry_offset=8,
                          format_version=2,
                          callable_services=[(b"TIME", 1, 0, service, 1)])  # 1: SET
    allocation = int.from_bytes(carrier[14:16], "little")
    output = BUILD / "M2CLK.RSX"
    output.write_bytes(carrier)
    print(f"{hashlib.sha256(code).hexdigest()}  build/rsx/m2clk.bin")
    print(f"M2CLK.RSX bytes: {len(code)}; allocation: {allocation}; "
          f"relocations: {len(relocations)}; carrier: {len(carrier)}")


if __name__ == "__main__":
    main()
