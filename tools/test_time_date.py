#!/usr/bin/env python3
"""Execute TIME /SET calendar-date parsing against Python's own calendar."""
from datetime import date
from pathlib import Path
import re
from test_bios import Z80, require

ROOT = Path(__file__).resolve().parents[1]
EPOCH = date(1977, 12, 31)                      # CP/M day 0


def bcd(value: int) -> int:
    return (value // 10) << 4 | value % 10


def main() -> None:
    names = {}
    for line in (ROOT / "build/utilities/time.lst").read_text(errors="replace").splitlines():
        match = re.match(r"^([0-9a-f]{4})\s+.*\s([A-Z][A-Z0-9_]*):", line, re.I)
        if match:
            names[match[2]] = int(match[1], 16)
    image = (ROOT / "build/utilities/TIME.COM").read_bytes()

    def parse(tail):
        cpu = Z80(b"")
        cpu.mem[0x100:0x100 + len(image)] = image
        cpu.mem[0x80] = len(tail)
        cpu.mem[0x81:0x81 + len(tail)] = tail.encode("ascii")
        cpu.run(names["TM_PARSE"], limit=200000)
        return cpu

    valid = [("01/01/78 00:00", date(1978, 1, 1), 0, 0, 0),
             ("10/25/26 15:30", date(2026, 10, 25), 15, 30, 0),
             ("10/25/2026 15:30:45", date(2026, 10, 25), 15, 30, 45),
             ("12/31/99 23:59:59", date(1999, 12, 31), 23, 59, 59),
             ("01/01/00 00:00", date(2000, 1, 1), 0, 0, 0),
             ("02/29/2000 12:00", date(2000, 2, 29), 12, 0, 0),
             ("02/29/24 06:07:08", date(2024, 2, 29), 6, 7, 8),
             ("12/31/77 01:02", date(2077, 12, 31), 1, 2, 0),
             ("03/01/2100 00:00", date(2100, 3, 1), 0, 0, 0),
             ("06/05/2157 23:59:59", date(2157, 6, 5), 23, 59, 59)]
    for text, day, hour, minute, second in valid:
        for tail in ("/SET " + text, " /set " + text):
            cpu = parse(tail)
            require(cpu.z and cpu.mem[names["TM_MODE"]] == 2, f"valid date rejected: {tail}")
            count = (day - EPOCH).days
            expect = bytes((1, 10, 1, 0, count & 255, count >> 8,
                            bcd(hour), bcd(minute), bcd(second), 0))
            got = bytes(cpu.mem[names["TM_REQUEST"]:names["TM_REQUEST"] + 10])
            require(got == expect, f"{tail}: request {got.hex()} expected {expect.hex()}")

    invalid = ["02/29/2100 00:00", "02/29/23 00:00", "06/06/2157 00:00",
               "12/31/1977 23:59", "13/01/26 00:00", "00/10/26 00:00",
               "10/00/26 00:00", "02/30/24 00:00", "12/32/26 00:00",
               "10/25/26 24:00", "10/25/26 23:60", "10/25/26 23:59:60",
               "1A/25/26 00:00", "10/25/26 00-00", "10-25-26 00:00",
               "10/25/26 00:00:0", "10/25/26 00:00X", "10/25/2026 00:00:00X",
               "10/25/26  0:00", "10/25/26 00:00 ", "1/25/2026 00:00:00"]
    for text in invalid:
        cpu = parse("/SET " + text)
        require(not cpu.z, f"invalid date accepted: /SET {text}")
    print("TIME /SET calendar dates: day counts, leap years, range limits and rejects passed")


if __name__ == "__main__":
    main()
