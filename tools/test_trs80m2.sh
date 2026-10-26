#!/bin/bash
# test_trs80m2.sh - boot-level tests of the BetterCP/M TRS-80 Model II disk
# under trs80gp (headless through xvfb-run when no display is available).
ROOT=$(cd "$(dirname "$0")/.." && pwd)
IMAGE="$ROOT/build/trs80m2/BetterCPM-Model2-77T-SS-System-584K.imd"
EMU="${TRS80GP:-trs80gp}"
PT=""
KEEP=""

usage() {
    cat <<EOF
usage: test_trs80m2.sh [options]
  -image FILE  Model II system disk to test (default: the build output)
  -emu PATH    trs80gp binary (default \$TRS80GP or trs80gp on PATH)
  -pt FILE     also test read access to a Pickles & Trout CP/M disk image
  -keep DIR    keep work files (disk images, screens) in DIR
  -help        this list
Builds nothing; run BETTERCPM_PLATFORM=trs80m2 tools/build_complete_system.py first.
EOF
}
while [ $# -gt 0 ]; do
    case "$1" in
        -help)  usage; exit 0 ;;
        -image) IMAGE="$2"; shift 2 ;;
        -emu)   EMU="$2"; shift 2 ;;
        -pt)    PT="$2"; shift 2 ;;
        -keep)  KEEP="$2"; shift 2 ;;
        *)      echo "unknown option $1 (try -help)" >&2; exit 2 ;;
    esac
done
[ -f "$IMAGE" ] || { echo "missing image: $IMAGE" >&2; exit 2; }
command -v "$EMU" >/dev/null || [ -x "$EMU" ] || { echo "trs80gp not found: $EMU" >&2; exit 2; }

WORK=$(mktemp -d); [ -n "$KEEP" ] && { mkdir -p "$KEEP"; WORK=$(cd "$KEEP" && pwd); }
[ -z "$KEEP" ] && trap 'rm -rf "$WORK"' EXIT
XVFB=""; [ -z "$DISPLAY" ] && XVFB="xvfb-run -a"
PASS=0; FAIL=0

# run NAME DRIVE0 DRIVE1|- WAITFRAMES KEYS [KEYS2 WAIT2FRAMES] : boot, type KEYS,
# wait, optionally type KEYS2 and wait again, save screen text. trs80gp counts
# waits in 60 Hz frames.
run() {
    local name=$1 d0=$2 d1=$3 wait=$4 keys=$5 keys2=$6 wait2=$7 dir="$WORK/$1"
    mkdir -p "$dir"; rm -f "$dir"/trs80-text-*.bin
    local args=(-m2 -batch -turbo -itime 0 -d0 "$d0")
    [ "$d1" != "-" ] && args+=(-d1 "$d1")
    args+=(-id 9000)
    [ -n "$keys" ] && args+=(-i "$keys")
    args+=(-id "$wait")
    [ -n "$keys2" ] && args+=(-i "$keys2" -id "$wait2")
    args+=(-it -ix)
    ( cd "$dir" && timeout 1200 $XVFB "$EMU" "${args[@]}" >emu.log 2>&1 )
    perl -e 'local $/; open my $f,"<:raw",$ARGV[0] or exit 1; my $s=<$f>;
      for my $r (0..23) { my $l=substr($s,$r*80,80); $l=~tr/\x80-\xff/\x00-\x7f/;
        $l=~s/[^\x20-\x7e]/./g; $l=~s/\s+$//; print "$l\n" }' \
        "$dir/trs80-text-0.bin" > "$dir/screen.txt" 2>/dev/null || : > "$dir/screen.txt"
}

# expect NAME TEXT... : every TEXT must appear on NAME's screen
expect() {
    local name=$1; shift
    local text missing=""
    for text in "$@"; do grep -qF -- "$text" "$WORK/$name/screen.txt" || missing+=" [$text]"; done
    if [ -z "$missing" ]; then
        echo "PASS  $name"; PASS=$((PASS + 1))
    else
        echo "FAIL  $name: missing$missing"; FAIL=$((FAIL + 1))
        sed 's/^/      | /' "$WORK/$name/screen.txt" | grep -v '^      | $'
    fi
}

disk() { cp "$IMAGE" "$WORK/$1.imd"; echo "$WORK/$1.imd"; }
blank() { perl "$ROOT/src/platform/trs80m2/mkm2imd.pl" -out "$WORK/$1.imd" >/dev/null; echo "$WORK/$1.imd"; }

echo "Model II tests: $IMAGE"

run boot "$(disk boot)" - 3000 ""
expect boot "BetterCP/M" "Core OS" "CP/M 2.2 Compatible System" "A0>"

run dir "$(disk dir)" - 4000 $'CLS\rDIR\rSTAT\r'
expect dir "README   TXT" "DISK     FDB" "SYSGEN   COM" "A: R/W, Space:"

run copy "$(disk copy)" - 4000 $'CLS\rCOPY README.TXT X.TXT\rDIR X.TXT\r'
expect copy "A0: X        TXT"

run config "$(disk config)" - 4000 $'CLS\rCONFIG /SHOW\r'
expect config "BetterCP/M M2 SS" "Type of drive (inches):     8" "Step rate (ms):             3"

SYS=$(disk sysgen-src); TARGET=$(blank target)
run format "$SYS" "$TARGET" 60000 $'CLS\rDUP /FORMAT B:\rY'
expect format "Tracks written and verified: 00077" "Format complete."

run sysgen "$SYS" "$TARGET" 9000 $'CLS\rSYSGEN A: B:\rY'
expect sysgen "System installed and verified. The destination is bootable."

run newboot "$TARGET" - 3000 ""
expect newboot "A0>"
if grep -qF "BOOT ERROR" "$WORK/newboot/screen.txt"; then
    echo "FAIL  newboot: ROM refused the new disk"; FAIL=$((FAIL + 1)); PASS=$((PASS - 1))
fi

# The software clock: unavailable until set, then counts real (emulated) time.
# Set 12:00:00, wait 90 seconds (5400 frames) less the time taken to set it,
# then read it back: the minute must have advanced exactly once.
run clock "$(disk clock)" - 5400 $'CLS\rRSX LOAD M2CLK\rTIME\rTIME /PROVIDER\rTIME /SET 10/25/26 12:00\r' \
    $'TIME\r' 1800
expect clock "Clock unavailable" "Provider: M2CLK" "GET/SET" "Clock set." "2026-10-25 12:01:"

# serial NAME KEYS [-b CHATB_ARGS... --] CHATA_ARGS... : boot with serial ports A
# and B listening on TCP, type KEYS, then run m2chat.pl on port A (and, with -b,
# on port B in the background). The port A transcript, without CRs, becomes
# NAME's screen for expect.
CHAT="$ROOT/src/platform/trs80m2/m2chat.pl"
serial() {
    local name=$1 keys=$2 dir="$WORK/$1"; shift 2
    local pa=$((7200 + RANDOM % 1000)); local pb=$((pa + 1))
    local bargs=()
    if [ "$1" = "-b" ]; then
        shift; while [ "$1" != "--" ]; do bargs+=("$1"); shift; done; shift
    fi
    mkdir -p "$dir"
    local image; image=$(disk "$name")
    ( cd "$dir" && setsid timeout 1200 $XVFB "$EMU" -m2 -batch -turbo -itime 0 \
        -rA "@$pa" -rB "@$pb" -d0 "$image" -id 9000 -i "$keys" -id 300000 -ix \
        >emu.log 2>&1 & )
    [ ${#bargs[@]} -gt 0 ] && perl "$CHAT" -port "$pb" -connect 120 "${bargs[@]}" >"$dir/portb.txt" 2>&1 &
    perl "$CHAT" -port "$pa" -connect 120 -timeout 180 "$@" >"$dir/porta.txt" 2>"$dir/chat.err"
    tr -d '\r' <"$dir/porta.txt" >"$dir/screen.txt"; cat "$dir/chat.err" >>"$dir/screen.txt"
    pkill -f -- "-rA @$pa " 2>/dev/null
    while pgrep -f -- "-rA @$pa " >/dev/null; do sleep 1; done
}

# Console on serial port A (STAT CON:=TTY:), and back to the CRT.
serial sercon $'CLS\rSTAT CON:=TTY:\r' -expect '\nA0>' -send 'DIR\r' -expect '\nA0>' \
    -send 'STAT CON:=CRT:\r' -drain 2
expect sercon "CON: is TTY:" "M2CLK    RSX" "STAT CON:=CRT:"

# Punch to port A, then reader from port B, driven from the serial console.
printf 'Line one from Linux\r\nLine two\r\n\032' >"$WORK/in.txt"
serial serpip $'CLS\rSTAT CON:=TTY: RDR:=UR1:\r' -b -drain 25 -pace 30 -sendfile "$WORK/in.txt" -drain 5 -- \
    -expect '\nA0>' -send 'PIP PUN:=STARTUP.SUB\r' -expect '\nA0>' -send 'PIP FROMPC.TXT=RDR:\r' \
    -expect '\nA0>' -send 'TYPE FROMPC.TXT\r' -expect '\nA0>' -send 'STAT CON:=CRT:\r' -drain 2
expect serpip ":VER /BANNER" "Line one from Linux" "Line two"

# The clock runs alongside serial interrupts: set it from the serial console.
serial serclk $'CLS\rSTAT CON:=TTY:\r' -expect '\nA0>' -send 'RSX LOAD M2CLK\r' -expect '\nA0>' \
    -send 'TIME /SET 10/25/26 12:00\r' -expect '\nA0>' -send 'TIME\r' -expect '\nA0>' \
    -send 'STAT CON:=CRT:\r' -drain 2
expect serclk "Clock set." "2026-10-25 12:00:0"

if [ -n "$PT" ]; then
    cp "$PT" "$WORK/pt.imd"
    run pickles "$(disk pickles)" "$WORK/pt.imd" 6000 \
        $'CLS\rCONFIG /LOGICAL=B /PHYSICAL=1 /FORMAT="Pickles & Trout M2 SS"\rCLS\rDIR B:\r'
    expect pickles "B0: " " COM"
fi

echo "$PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
