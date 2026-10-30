# BetterCP/M for the TRS-80 Model II

**A TRS-80 Model II port of [BetterCP/M](https://github.com/CPMArchives/BetterCPM), developed on the `trs80m2` branch of this fork.**

This fork adds support for the **Radio Shack TRS-80 Model II**, including its 8-inch floppy disk system, machine-specific boot process, console and serial interfaces, and supporting build tools.

It builds on the CP/M 2.2-compatible BetterCP/M operating system developed by [CPMArchives](https://github.com/CPMArchives).

**You are viewing the Model II port.** The upstream project is maintained separately; this fork retains its `main` branch for upstream synchronization.

## Project Status

- **Model II port version:** `v.05`
- **Port source:** [`src/platform/trs80m2/`](src/platform/trs80m2/)
- **Source branch:** [`trs80m2`](https://github.com/a-griff/BetterCPM/tree/trs80m2)
- **Upstream project:** [CPMArchives/BetterCPM](https://github.com/CPMArchives/BetterCPM)

This is a development port, not a claim that the Model II platform has completed upstream release qualification.

BetterCP/M's upstream OS version and its release readiness are independent of this port's `v.05` designation.

## TRS-80 Model II Support

The port provides machine-specific components for:

- Model II boot loading and BIOS integration.
- 8-inch floppy disk access and disk-format definitions.
- Console and display handling.
- Model II clock support.
- Serial port A and B support through the platform extension.
- CP/M device assignments `TTY:`, `UC1:`, `RDR:`, `PUN:`, and `LST:`.
- System-disk image generation.
- Platform-specific build tools and tests.

The port also adapts BetterCP/M system utilities and their supporting build processes for the Model II.

Availability of individual utilities depends on the disk image being used.

## Floppy Disk Format

The supplied Model II system-disk work targets an **8-inch, 77-track, single-sided** disk layout, with an FM boot track and MFM data tracks.

See the disk documentation for exact sector geometry, reserved tracks, and capacity calculations rather than assuming compatibility with every Model II disk format.

- [TRS-80 Model II 8-inch System Disk documentation](docs/platform/TRS-80%20Model%20II%208-inch%20System%20Disk.md)
- [Model II disk format definition](src/platform/trs80m2/M2DISK.FDF)

## Source Code and Build Tools

| Location | Contents |
| --- | --- |
| [`src/platform/trs80m2/`](src/platform/trs80m2/) | Model II boot, disk, console, clock, and extension sources |
| [`src/bios/`](src/bios/) | Shared BIOS and integration code |
| [`src/utilities/`](src/utilities/) | BetterCP/M utilities, including Model II adaptations |
| [`tools/build_trs80m2_image.py`](tools/build_trs80m2_image.py) | Model II image builder |
| [`tools/build_trs80m2_stage1.py`](tools/build_trs80m2_stage1.py) | Stage-one boot build tool |
| [`tools/build_trs80m2_ext.py`](tools/build_trs80m2_ext.py) | Model II extension builder |
| [`tools/test_trs80m2.sh`](tools/test_trs80m2.sh) | Model II platform test script |
| [`src/platform/trs80m2/m2run.sh`](src/platform/trs80m2/m2run.sh) | Model II run helper |

For the exact build sequence, prerequisites, command-line options, and current limitations, consult the scripts and platform documentation in this branch.

**Do not assume the upstream Model 4 build instructions will produce a Model II boot disk unchanged.**

## Getting a Bootable Disk Image

The Model II image is distributed as an **ImageDisk (`.imd`)** artifact.

The versioned `v.05` image is named:

`BetterCPM-Model2-77T-SS-System-584K-v.05.imd`

Check this repository's [Releases](https://github.com/a-griff/BetterCPM/releases) for published images.

If none is attached, the source and build tools are available in this branch. The filename above does not imply that the binary is already downloadable from GitHub.

## Compatibility

BetterCP/M aims to preserve conventional **CP/M 2.2** application and programming compatibility, including:

- Standard CP/M `.COM` programs.
- The CP/M BDOS programming interface.
- Standard 8.3 filenames.
- Conventional CP/M disk and user-area operations.

Some utilities and applications may depend on machine-specific hardware or undocumented behavior and should be tested on the Model II port.

This port does **not** add long filename support. The normal CP/M 8.3 filename convention remains in effect.

## Relationship to Upstream

This repository is a **fork**, not a replacement for the original BetterCP/M project.

- [Original BetterCP/M repository](https://github.com/CPMArchives/BetterCPM) — Core operating-system development.
- [`main` branch](https://github.com/a-griff/BetterCPM/tree/main) — Retained for synchronization with upstream.
- [`trs80m2` branch](https://github.com/a-griff/BetterCPM/tree/trs80m2) — TRS-80 Model II port and machine-specific changes.

Improvements to the shared operating system should be considered for contribution back to upstream.

Model II support can be proposed for inclusion through an upstream pull request.

## Credits

**Original BetterCP/M project:**  
[CPMArchives](https://github.com/CPMArchives/BetterCPM) and its contributors.

**TRS-80 Model II port:**  
[a-griff](https://github.com/a-griff)

For upstream design rationale, specifications, and general utilities documentation, see the [original BetterCP/M project](https://github.com/CPMArchives/BetterCPM).

This README describes the Model II fork specifically.
