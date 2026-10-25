"""Select the TRS-80 platform for the resident-system build.

BETTERCPM_PLATFORM=trs80m2 builds for the Model II; anything else (the
default) builds for the Model 4 exactly as before.
"""
from __future__ import annotations

import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NAME = os.environ.get("BETTERCPM_PLATFORM", "trs80m4")
if NAME not in ("trs80m4", "trs80m2"):
    raise SystemExit(f"unknown BETTERCPM_PLATFORM {NAME!r}; use trs80m4 or trs80m2")
MODEL2 = NAME == "trs80m2"
DIR = ROOT / "src/platform" / NAME

# Files the BIOS build stages next to bios.mac.
BIOS_INCLUDES = (("hardware.inc", "m2cons.inc", "m2scroll.inc", "m2disk.inc", "m2clock.inc")
                 if MODEL2 else
                 ("hardware.inc", "m4cons.inc", "m4scroll.inc", "m4disk.inc"))
BIOSPLAT = DIR / "biosplat.inc" if MODEL2 else ROOT / "src/bios/biosplat.inc"
DISK_SOURCE = "src/platform/trs80m2/disk.mac" if MODEL2 else "src/bios/disk.mac"
TABLES_SOURCE = "src/platform/trs80m2/tables.mac" if MODEL2 else "src/bios/tables.mac"


def system_sector(n: int) -> tuple[int, int, int]:
    """Cylinder, side and sector ID of 512-byte system-area sector n."""
    if MODEL2:
        # Package sectors 0-1 (records 0-7) live on the FM boot track; sector
        # n >= 2 follows from cylinder 1, sixteen 512-byte sectors per cylinder.
        if n < 2:
            raise ValueError("Model II package sectors 0-1 are boot-track records")
        n -= 2
        return 1 + n // 16, 0, n % 16 + 1
    order = (1, 3, 5, 7, 9, 2, 4, 6, 8, 10)
    return n // 20, n // 10 % 2, order[n % 10]
