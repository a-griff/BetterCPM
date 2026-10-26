# TRS-80 Model II: 8-inch System Disk

Status: Model II platform definition
Date: 2026-10-25

This document defines how BetterCP/M uses the TRS-80 Model II hardware and its
8-inch floppy disks: what the boot ROM requires, where the BetterCP/M system
package lives, the default filesystem, and the interchange format for Pickles &
Trout CP/M disks. Platform code lives in `src/platform/trs80m2`; build with
`BETTERCPM_PLATFORM=trs80m2`.

## Compatibility rules

Every Model II change is checked against these rules, in priority order:

1. **General CP/M 2.2 programs come first.** The CP/M archives hold far more
   software than was ever sold for the Model II, so standard CP/M 2.2
   behaviour outranks everything else. The BIOS jump table, page zero (0000H
   warm-boot jump, 0005H BDOS entry, IOBYTE, default FCB, command buffer), BDOS
   behaviour and the logical devices CON:, RDR:, PUN: and LST: through the
   IOBYTE never change.
2. **Pickles & Trout programs come second.** P&T is the standard CP/M for the
   Model II, so software written or installed for it (P&T disks, console codes)
   should also run, as long as that never costs general compatibility.
3. **The program area never shrinks.** New resident code goes above the system
   (for example F400H-F7FFH), not into the TPA.
4. **New features are extras that programs can ignore.** They are offered as
   separate services, and anything that changes machine state is off until a
   BetterCP/M feature turns it on: the clock's interrupts start on the first
   SET, and a serial driver will leave the SIO and CTC untouched until the
   console is redirected or a BetterCP/M program opens the port. Programs that
   drive the hardware themselves then find it as they expect.
5. **A program may break a BetterCP/M extra, never the reverse.** A program that
   sets its own interrupt mode may stop the clock while it runs; that is
   acceptable. BetterCP/M must not make a working CP/M program fail.
6. **Screen personalities are loadable, not forced.** ADM-3A, VT52 and ANSI/VT100
   (the codes most archive software can be installed for) and a P&T-compatible
   console all draw through one Model II screen driver; none is imposed on a
   program that does not expect it.

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
| Cylinders 1–76 | MFM, 16 × 512, sector IDs 1–16, 3:1 interleave |
| CP/M logical track | One cylinder; SPT 64 |
| Reserved area | OFF 4 (cylinders 0–3) |
| Blocks | 2 KiB, 16-bit pointers, DSM 291 (584K) |
| Directory | 128 entries (AL0 C0H), CKS 32 |

Physical order on each MFM track is 1, 12, 7, 2, 13, 8, 3, 14, 9, 4, 15, 10, 5,
16, 11, 6: consecutive sectors are three slots apart, which gives the BIOS two
sector times (about 19 ms) to finish one transfer and ask for the next before
it passes the head. Sector IDs and logical order are unchanged, so the
interleave only affects speed; a disk laid out 1:1 still reads and writes
correctly. The boot track stays sequential.

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
- MFM tracks are laid out 3:1 (`ILEAVE`), as described above; the FM boot
  track is written in order.
- Track capacity follows the drive: 5,208 bytes FM and 10,416 bytes MFM per
  revolution for 8-inch drives at 500 kbps and 360 rpm (`SYSINCH EQU 8`).
- DUP's write-track stream starts at 7000H instead of 8000H on 8-inch
  platforms (`LONGTRK`), because an 8-inch MFM track plus the index tail does
  not fit the 10K buffer used for 5¼-inch drives.

The control overlay streams each track to the controller with the Z80 DMA
(`IFDEF DMAPORT`); the Model 4 keeps its polled loop. The resident disk module
selects FM for cylinder 0 side 0 and the binding's density everywhere else, so
reads, writes and formatting all follow the same rule.

## Building and testing

```sh
BETTERCPM_PLATFORM=trs80m2 python3 tools/build_complete_system.py
TRS80GP=/path/to/trs80gp tools/test_trs80m2.sh -pt /path/to/PT22MHA.IMD
```

The first command builds the resident system, utilities, catalogue and the
image `build/trs80m2/BetterCPM-Model2-77T-SS-System-584K.imd`. The test script
boots it under trs80gp (headless through `xvfb-run` when there is no display)
and checks cold boot and banner, DIR/STAT, COPY, CONFIG, DUP format of a blank
disk, SYSGEN onto it and booting that disk, loading M2CLK.RSX and setting and
reading the clock, and, with `-pt`, read access to a Pickles & Trout disk.
`tools/test_time_date.py` checks TIME's calendar-date SET against Python's
calendar. Without `BETTERCPM_PLATFORM` the build is the Model 4
build, unchanged. `src/platform/trs80m2/bttest.mac` is a standalone boot-track
probe of the controller, DMA and FM/MFM paths, useful when bringing up real
hardware.

## Sector cache

The Model II BIOS reads each physical sector into a private 1K buffer at F400H
(`PL_SBUF`, free RAM below the video window) and remembers its drive, side,
cylinder and sector. The other records of that sector are then served from
the buffer without touching the disk. On a miss the fresh sector is also
copied to the shared `LY_BUF`, where DUP and CONFIG expect it. Writes go
through to the disk at once; the cache is dropped on a failed write, on a
drive change and on console input (a disk may have been swapped), and the
first read after a write fetches its sector again.

Other targets (no `PL_SBUF`) keep using `LY_BUF` as before. The miss path copies
only the sector actually read: the RSX selector overlay runs from the top of
`LY_BUF` (F3A4H) and a full 1K copy would overwrite it.

Measured under trs80gp with the Model II clock (each figure includes about
4 seconds to load TIME.COM; trs80gp's `-id` waits are 60 Hz frames):

| Copy | Time |
| --- | --- |
| README.TXT (3.5K), drive A to drive B | about 40 s |
| README.TXT (3.5K), drive A to drive A | about 19 s |
| CONFIG.COM (15.6K), drive A to drive B | about 171 s |
| CONFIG.COM (15.6K), drive B to drive A | about 182 s |

Copies between drives are still slow, roughly 90 bytes per second. The cache
and interleave removed the extra revolution per record. What remains has not
been profiled yet; the shared allocation vector rescans the directory on
every drive change, and writes go through one record at a time.

## Software clock

The Model II has no battery-backed clock. Its hardware real-time interrupt
(vertical sync divided by two, 30 Hz, enabled by port FFH bit 5 and cleared by
reading port FEH) is a non-maskable interrupt, so it always enters 0066H. Under
CP/M that address lies inside the default FCB at 005CH, which every command
line overwrites, so BetterCP/M leaves the RTC interrupt disabled.

The clock uses CTC channel 3 instead. That channel otherwise only counts
keyboard interrupts (BetterCP/M polls the keyboard) and has no output pin, so
no serial baud rate is affected:

| Item | Value |
| --- | --- |
| Timer | CTC channel 3 (port F3H), control A7H: interrupt, timer, prescaler 256, automatic start |
| Rate | 4 MHz / 256 / 125 = 125 interrupts per second exactly |
| Interrupts | Mode 2; I = high byte of `MC_VEC`, CTC vector = `MC_VEC` AND F8H (channel 3 adds 6) |
| Record | TIME ABI 1.0: day count (day 1 = 1978-01-01), BCD hour, minute, second |
| Stack use | 10 bytes of the interrupted program's stack |

The tick handler, clock record and vector (`src/platform/trs80m2/m2clock.inc`,
about 70 bytes) live in the BIOS, at fixed addresses that never move.
`M2CLK.RSX` (`m2clk.mac`) is a stateless TIME provider that links to them at
build time, so it can be loaded, moved or unloaded without affecting the clock.
Interrupts stay disabled until the first SET, which loads the record, disables
PIO interrupts, programs the CTC, selects mode 2 and enables interrupts. GET
reports UNAVAILABLE (status 5) before that, and whenever interrupts are found
disabled or the I register no longer points at the clock vector, rather than
return a stopped time.

```text
A0>RSX LOAD M2CLK
A0>TIME /SET 10/25/26 15:30
A0>TIME
```

A program that keeps interrupts disabled for longer than 8 ms loses ticks, and
one that installs its own interrupt-mode-2 table or reprograms the CTC vector
stops the clock until it is set again. Clock interrupts do not disturb disk
transfers, which the DMA performs.

## RSX loading

The Model 4 boot-disk builder rebuilds the RSX overlay selector
(`rsxsel.mac`), whose sector table depends on the platform layout, as one of
its own steps. The Model II build now does this as the `rsxselect` component.
Before that, Model II disks carried a selector with Model 4 sector addresses
and every `RSX LOAD` failed.
