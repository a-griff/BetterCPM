#!/usr/bin/env python3
"""Build the BetterCP/M TRS-80 Model II 8-inch system disk (.IMD).

Run with BETTERCPM_PLATFORM=trs80m2 after the resident system and utilities
have been built for that platform (tools/build_complete_system.py does both).
The layout is defined in docs/platform/TRS-80 Model II 8-inch System Disk.md:

  cylinder 0       FM 26 x 128 boot track: system package records 0-7,
                   DIAG stub in sector 13
  cylinders 1-3    system package records 8-159, sixteen 512-byte sectors each
  cylinders 4-76   CP/M filesystem (2K blocks, 128 entries, DSM 291)
"""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import tempfile
from pathlib import Path

import build_platform
if not build_platform.MODEL2:
    raise SystemExit("set BETTERCPM_PLATFORM=trs80m2")
from build_system_package import compose, BOOT_BUILD
from build_trs80_boot import install_files
from startup_record import build as startup_record
from system_layout import LAYOUT, expand_layout

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/platform/trs80m2"
BUILD = ROOT / BOOT_BUILD
MANIFEST = ROOT / "metadata/distribution.json"

CYLINDERS = 77
SECTOR_BYTES = 512
CYLINDER_BYTES = 16 * SECTOR_BYTES
BOOT_TRACK_BYTES = 26 * 128
BOOT_RECORDS = 8                     # package records kept on the FM boot track
RESERVED_CYLINDERS = 4               # OFF 4
BLOCK_COUNT = 292                    # DSM 291
STAGE1_ADDRESS = 0x1000              # ROM 'BOOT' signature address
DIAG_OFFSET = 0x600                  # 1400H within the boot track
DIAG_STUB = b"DIAG\xc3\x04\x10"      # 'DIAG' then JP 1004H; written at format time


def assemble_stage1(assembler: Path) -> bytes:
    text = (SOURCE / "stage1.mac").read_text(encoding="ascii")
    text = text.replace("        CSEG\n        .PHASE  ", "        ASEG\n        ORG     ")
    text = text.replace("        .DEPHASE\n", "")
    output = BUILD / "stage1.bin"
    with tempfile.TemporaryDirectory(prefix="bettercpm-m2stage1-") as temporary:
        staged = Path(temporary)
        (staged / "stage1.mac").write_text(expand_layout(text), encoding="ascii")
        for name in ("hardware.inc", "m2disk.inc"):
            shutil.copy2(SOURCE / name, staged / name)
        subprocess.run([str(assembler), "-fb", f"-o{output}",
                        f"-l{BUILD / 'stage1.lst'}", "stage1.mac"],
                       check=True, cwd=staged, stdout=subprocess.DEVNULL)
    data = output.read_bytes()
    if len(data) > STAGE1_ADDRESS and data[:STAGE1_ADDRESS] == bytes(STAGE1_ADDRESS):
        data = data[STAGE1_ADDRESS:]
        output.write_bytes(data)
    if not data.startswith(b"BOOT") or len(data) > SECTOR_BYTES:
        raise SystemExit(f"stage one is {len(data)} bytes; must start with BOOT and fit 512")
    return data


def build_boot_components(assembler: Path) -> bytes:
    """Assemble the platform BOOT.BIN and STAGE1.BIN package components."""
    BUILD.mkdir(parents=True, exist_ok=True)
    stage1 = assemble_stage1(assembler)
    (BUILD / "boot.bin").write_bytes(bytes(SECTOR_BYTES))  # records 0-3: startup record only
    return stage1


def manifest_files() -> tuple[list[tuple[str, bytes, int]], str]:
    manifest = json.loads(MANIFEST.read_text())
    files = []
    for component in manifest["system_disk"]:
        if "trs80m2" not in component["targets"] or component["status"] != "implemented":
            continue
        data = (ROOT / component["artifact"]).read_bytes()
        if not data:
            raise SystemExit(f"empty artifact: {component['artifact']}")
        files.append((component["name"], data, component["user"]))
    return files, manifest.get("cold_start_command", "")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--assembler", type=Path, default=Path.home() / "bin/z80asm")
    parser.add_argument("--output", type=Path,
                        default=BUILD / "BetterCPM-Model2-77T-SS-System-584K.imd")
    parser.add_argument("--include-as", action="append", default=[], metavar="NAME=PATH",
                        help="also install PATH as user-zero NAME (repeatable)")
    parser.add_argument("--no-startup", action="store_true",
                        help="leave the cold-start command empty")
    args = parser.parse_args()
    BUILD.mkdir(parents=True, exist_ok=True)

    stage1 = build_boot_components(args.assembler)
    package, _ = compose()
    payload = bytearray(package[128:])
    files, command = manifest_files()
    if args.no_startup:
        command = ""
    payload[128:384] = startup_record(command, system_base=LAYOUT["SYSTEM"])
    for spec in args.include_as:
        name, sep, path = spec.partition("=")
        if not sep or not Path(path).is_file():
            raise SystemExit(f"invalid --include-as {spec}")
        files.append((name.upper(), Path(path).read_bytes(), 0))

    # Cylinder 0: boot-track records, then the format-time DIAG stub.
    boot_track = bytearray(b"\xe5" * BOOT_TRACK_BYTES)
    boot_track[:BOOT_RECORDS * 128] = payload[:BOOT_RECORDS * 128]
    boot_track[DIAG_OFFSET:DIAG_OFFSET + len(DIAG_STUB)] = DIAG_STUB

    # Cylinders 1-76: the rest of the package, then the filesystem.
    disk = bytearray(b"\xe5" * ((CYLINDERS - 1) * CYLINDER_BYTES))
    rest = payload[BOOT_RECORDS * 128:]
    if len(rest) > (RESERVED_CYLINDERS - 1) * CYLINDER_BYTES:
        raise SystemExit("system package exceeds the reserved cylinders")
    disk[:len(rest)] = rest
    install_files(disk, files,
                  directory=(RESERVED_CYLINDERS - 1) * CYLINDER_BYTES,
                  block_count=BLOCK_COUNT)

    (BUILD / "boottrack.bin").write_bytes(boot_track)
    (BUILD / "cylinders.bin").write_bytes(disk)
    subprocess.run(["perl", str(SOURCE / "mkm2imd.pl"),
                    "-boot", str(BUILD / "boottrack.bin"),
                    "-sys", str(BUILD / "cylinders.bin"),
                    "-cyl", str(CYLINDERS), "-interleave", "3",
                    "-out", str(args.output)],
                   check=True, stdout=subprocess.DEVNULL)
    image = args.output.read_bytes()
    print(f"{hashlib.sha256(image).hexdigest()}  {args.output}")
    print(f"stage one: {len(stage1)} bytes; package: {len(payload)} bytes; "
          f"{len(files)} files; cold start: {command or '(none)'}")


if __name__ == "__main__":
    main()
