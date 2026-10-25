#!/usr/bin/env python3
"""Build a bootable BetterCP/M TRS-80 Model II 8-inch .IMD system disk.

Run with BETTERCPM_PLATFORM=trs80m2 after build_complete_system.py has been
run under the same setting. Reuses build_trs80_boot's system-area and
filesystem installer with Model II geometry:

  cylinder 0      FM 26 x 128 boot track (src/platform/trs80m2/boot.mac),
                  startup record in FM sectors 2-3
  cylinders 1-3   system area, sixteen 512-byte MFM sectors per cylinder
  cylinders 4-76  CP/M filesystem (2K blocks, 128 entries, DSM 291)
"""
from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

import build_platform
if not build_platform.MODEL2:
    raise SystemExit("set BETTERCPM_PLATFORM=trs80m2")
import build_trs80_boot as m4
from system_layout import expand_layout

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/platform/trs80m2"
BUILD = ROOT / "build/trs80m2"
CYLINDERS = 77
CYLINDER_BYTES = 16 * 512
BOOT_TRACK_BYTES = 26 * 128

# Model II geometry for the shared installer: its raw image starts at
# cylinder 1, so the four reserved cylinders leave three (48 sectors) here.
m4.FILESYSTEM_FIRST_SECTOR = 3 * 16
m4.RAW_SIZE = (CYLINDERS - 1) * CYLINDER_BYTES
m4.BLOCK_COUNT = ((m4.RAW_SIZE - m4.FILESYSTEM_FIRST_SECTOR * 512)
                  // m4.ALLOCATION_BLOCK_BYTES)
m4.build = lambda raw: raw          # no Model 4 DMK encoding
m4.verify = lambda *args, **kwargs: None


def assemble_boot(assembler: Path) -> bytes:
    text = (SOURCE / "boot.mac").read_text(encoding="ascii")
    with tempfile.TemporaryDirectory(prefix="bettercpm-m2boot-") as temporary:
        staged = Path(temporary)
        (staged / "boot.mac").write_text(expand_layout(text), encoding="ascii")
        for name in ("hardware.inc", "m2disk.inc"):
            shutil.copy2(SOURCE / name, staged / name)
        output = BUILD / "boot.bin"
        subprocess.run([str(assembler), "-fb", f"-o{output}",
                        f"-l{BUILD / 'boot.lst'}", "boot.mac"],
                       check=True, cwd=staged)
    data = output.read_bytes()
    if len(data) != BOOT_TRACK_BYTES:
        raise SystemExit(f"boot track is {len(data)} bytes; expected {BOOT_TRACK_BYTES}")
    return data


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--assembler", type=Path, default=Path.home() / "bin/z80asm")
    parser.add_argument("--output", type=Path,
                        default=BUILD / "BetterCPM-Model2-77T-SS-System-584K.imd")
    args, passthrough = parser.parse_known_args()
    BUILD.mkdir(parents=True, exist_ok=True)
    raw_path = BUILD / "system-and-files.raw"
    sys.argv = [sys.argv[0], "--assembler", str(args.assembler),
                "--output", str(raw_path), *passthrough]
    m4.main()
    raw = raw_path.read_bytes()
    boot = bytearray(assemble_boot(args.assembler))
    boot[128:384] = raw[128:384]    # startup record: logical records 1-2 of track 0
    (BUILD / "boottrack.bin").write_bytes(boot)
    subprocess.run(["perl", str(SOURCE / "mkm2imd.pl"),
                    "-boot", str(BUILD / "boottrack.bin"), "-sys", str(raw_path),
                    "-cyl", str(CYLINDERS), "-out", str(args.output)], check=True)


if __name__ == "__main__":
    main()
