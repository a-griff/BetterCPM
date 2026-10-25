#!/usr/bin/env python3
"""Compile and independently verify the qualified FDF v1 catalogue."""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import tempfile

from fdf_v1 import compile_file, read_fdb


ROOT = Path(__file__).resolve().parents[1]
import build_platform
# The Model II carries its own 8-inch catalogue; every other target the
# qualified 107-format catalogue.
SOURCE = (ROOT / "src/platform/trs80m2/M2DISK.FDF" if build_platform.MODEL2
          else ROOT / "metadata" / "DISK.FDF")
DESCRIPTORS = 4 if build_platform.MODEL2 else 107
DEFAULT_OUTPUT = (ROOT / "build" / "catalog" / "trs80m2" / "DISK.FDB" if build_platform.MODEL2
                  else ROOT / "build" / "catalog" / "DISK.FDB")


def build(output: Path = DEFAULT_OUTPUT) -> Path:
    payload = compile_file(SOURCE)
    database = read_fdb(payload)
    if len(database.descriptors) != DESCRIPTORS or not all(
            descriptor.supported for descriptor in database.descriptors):
        raise ValueError(f"catalogue is not {DESCRIPTORS} supported descriptors")
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=output.parent, delete=False) as stream:
        temporary = Path(stream.name)
        stream.write(payload)
    try:
        if temporary.read_bytes() != payload:
            raise ValueError("catalogue temporary-file read-back failed")
        read_fdb(temporary.read_bytes())
        temporary.replace(output)
    finally:
        temporary.unlink(missing_ok=True)
    return output


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    path = build(args.output)
    payload = path.read_bytes()
    print(f"{len(payload)} bytes, {DESCRIPTORS} descriptors")
    print(f"{hashlib.sha256(payload).hexdigest()}  {path}")


if __name__ == "__main__":
    main()
