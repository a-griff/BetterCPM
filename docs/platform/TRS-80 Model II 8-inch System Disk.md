# TRS-80 Model II: 8-inch System Disk

Status: Model II platform definition
Date: 2026-10-25

This document defines how BetterCP/M uses the TRS-80 Model II hardware and its
8-inch floppy disks: what the boot ROM requires, where the BetterCP/M system
package lives, the default filesystem, and the interchange format for Pickles &
Trout CP/M disks. Platform code lives in `src/platform/trs80m2`; build with
`BETTERCPM_PLATFORM=trs80m2`.

## Hardware used

| Function | Ports / address | Notes |
| --- | --- | --- |
| FD1791 floppy controller | E4H–E7H | Command/status, track, sector, data |
| Drive select latch | EFH | Bits 0–3 drive (active low), bit 6 = side 0, bit 7 = MFM |
| Z80 DMA | F8H | Moves all disk data; FDC DRQ drives DMA READY |
| PIO port A | E0H | Bit 0 = FDC INTRQ (command complete) |
| Boot ROM control | F9H | Bit 0 = 1 maps the boot ROM at 0000H–07FFH |
| Keyboard | FCH (read), FFH bit 7 | ASCII from the keyboard; bit 7 of FFH = key waiting |
| 6845 CRTC | FCH/FDH (write) | Register 10 hides the hardware cursor |
| Control | FFH (write) | Bits 0–3 high-memory page, bit 7 maps video RAM at F800H |

BetterCP/M runs with FFH = 81H: page 1 at 8000H–FFFFH and the 80×24 video RAM
permanently mapped at F800H. The resident system ends below F400H, so nothing
of BetterCP/M is hidden by the video window.

Polled byte transfers cannot keep up with 8-inch double density (one byte
every 16 µs, 64 T-states at 4 MHz). All transfers therefore use the Z80 DMA
in byte mode, programmed as Pickles & Trout CP/M does, and completion is taken
from INTRQ on PIO port A.

## Boot ROM contract

On reset the Model II ROM reads all 26 single-density (FM) 128-byte sectors
of cylinder 0 into 0E00H–1AFFH. It then checks two signatures and refuses the
disk with `BOOT ERROR RS` if either is missing (verified under trs80gp):

| Address | Track 0 sector | Must contain | ROM action |
| --- | --- | --- | --- |
| 1000H | 5 | `BOOT` | jumps to 1004H last |
| 1400H | 13 | `DIAG` | calls 1404H first |

The boot ROM is still mapped when 1404H is called, so the `DIAG` entry must
never return after switching the ROM out. BetterCP/M's `DIAG` stub jumps to
the `BOOT` entry, which switches the ROM out and loads the system.

## Cylinder 0 is always FM

Every Model II floppy (TRSDOS, Pickles & Trout, BetterCP/M) has cylinder 0
side 0 recorded FM, 26 sectors of 128 bytes, IDs 1–26. The Model II disk
module therefore maps CP/M logical track 0 of any format to that FM track:
logical record *r* (0–25) is FM sector *r* + 1, with no skew. Every other
cylinder uses the format's own encoding and geometry.

## BetterCP/M system format (default)

`BetterCP/M M2 SS` — 77 cylinders, single sided, double density:

| Property | Value |
| --- | --- |
| Cylinder 0 | FM, 26 × 128 (boot track) |
| Cylinders 1–76 | MFM, 16 × 512, sector IDs 1–16 in order |
| CP/M logical track | One cylinder; SPT 64 |
| Reserved area | OFF 4 (cylinders 0–3) |
| Blocks | 2 KiB, 16-bit pointers, DSM 291 (584K) |
| Directory | 128 entries (AL0 C0H), CKS 32 |

All four drives default to this format. Any format in the DISK.FDB catalogue
can be selected per drive with CONFIG.

## System package placement

The BetterCP/M system package (SYSTEM.SYS) is 160 records of 128 bytes. On
the Model II the first eight records are boot-track records and the remaining
152 follow from cylinder 1:

| Package records | Content | Disk location |
| --- | --- | --- |
| 0 | (unused) | track 0 record 0 = FM sector 1 |
| 1–2 | Startup record | track 0 records 1–2 = FM sectors 2–3 (0E80H) |
| 3 | (unused) | FM sector 4 |
| 4–7 | Stage one: `BOOT`, entry, loader | FM sectors 5–8 (1000H–11FFH) |
| 8–59 | Resident system | cylinder 1 sectors 1–13 |
| 60–67 | Command reloader + RSX selector | cylinder 1 sectors 14–15 |
| 68–75 | Control overlay | cylinder 1 sector 16, cylinder 2 sector 1 |
| 76–107 | RSX carriers | cylinder 2 sectors 2–9 |
| 108–159 | Command processor | cylinder 2 sector 10 – cylinder 3 sector 6 |

Package record *r* ≥ 8 is at linear record 64 + (*r* − 8): track 1 upward, four
records per 512-byte sector. SYSGEN and CONFIG use this map through two
platform constants the Model II build prepends to the utilities:
`BOOTRECS EQU 8` (package records kept on the boot track) and `SYSINCH EQU 8`
(system drives are 8-inch, which also selects the 2 MHz step-rate table). Both
default to the Model 4 values (0 and 5), so other targets assemble unchanged.
SYSGEN never writes the `DIAG` stub, which belongs to the formatted disk.

The `DIAG` stub (FM sector 13: `DIAG`, then `JP 1004H`) is written when the
boot track is formatted. It does not depend on the system build.

Stage one loads the 13 resident sectors from cylinder 1 into the system area,
sets IOBYTE to 95H and enters BIOS BOOT. The BIOS reloads the command
environment from the sector tables generated from this placement.

## Pickles & Trout interchange format

`Pickles & Trout M2 SS`, derived from the Pickles & Trout CP/M 2.2mH
distribution disk:

| Property | Value |
| --- | --- |
| Cylinder 0 | FM boot track |
| Cylinders 1–76 | MFM, 16 × 512, IDs 1–16 in order |
| SPT / OFF | 64 / 2 |
| Blocks | 2 KiB, DSM 299 (600K) |
| Directory | 128 entries (AL0 C0H), CKS 32 |

Selecting this format for a drive gives full read/write access to P&T data
disks. The P&T system tracks are left untouched.

## Images

The builder emits ImageDisk (`.IMD`) images, which trs80gp boots directly:
mode 0 (500 kbps FM) for cylinder 0 and mode 3 (500 kbps MFM) for the rest.

## Formatting

DUP formats Model II disks with the same write-track streams it builds for
every target, plus three Model II rules:

- Cylinder 0 side 0 is always written as the FM boot track (26 × 128, IDs
  1–26), whatever format is selected, and sector 13 receives the `DIAG` stub.
  DUP verifies this track by reading its 26 records back rather than checking
  for erased data.
- Track capacity follows the drive: 5,208 bytes FM and 10,416 bytes MFM per
  revolution for 8-inch drives at 500 kbps and 360 rpm (`SYSINCH EQU 8`).
- DUP's write-track stream starts at 7000H instead of 8000H on 8-inch
  platforms (`LONGTRK`), because an 8-inch MFM track plus the index tail does
  not fit the 10K buffer used for 5¼-inch drives.

The control overlay streams each track to the controller with the Z80 DMA
(`IFDEF DMAPORT`); the Model 4 keeps its polled loop. The resident disk module
selects FM for cylinder 0 side 0 and the binding's density everywhere else, so
reads, writes and formatting all follow the same rule.
