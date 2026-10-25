#!/bin/bash
# m2run.sh - boot an image in trs80gp (Model II, headless) and print the screen as text
usage() {
    cat <<EOF
usage: m2run.sh [options] IMAGE
  -wait N      60 Hz frames to run before capturing (default 8000)
  -d1 IMAGE    image for drive 1
  -keys TEXT   type TEXT after -wait, then wait again (\\r = Enter)
  -emu PATH    trs80gp binary (default \$TRS80GP or trs80gp on PATH)
  -help        this list
EOF
}
WAIT=8000; D1=""; KEYS=""; EMU="${TRS80GP:-trs80gp}"; IMG=""
while [ $# -gt 0 ]; do
    case "$1" in
        -help) usage; exit 0 ;;
        -wait) WAIT="$2"; shift 2 ;;
        -d1)   D1="$2"; shift 2 ;;
        -keys) KEYS="$2"; shift 2 ;;
        -emu)  EMU="$2"; shift 2 ;;
        -*)    echo "unknown option $1 (try -help)" >&2; exit 1 ;;
        *)     IMG="$1"; shift ;;
    esac
done
[ -n "$IMG" ] || { usage; exit 1; }
IMG=$(realpath "$IMG"); [ -n "$D1" ] && D1=$(realpath "$D1")
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
ARGS=(-m2 -batch -turbo -itime 0 -d0 "$IMG")
[ -n "$D1" ] && ARGS+=(-d1 "$D1")
ARGS+=(-id "$WAIT")
[ -n "$KEYS" ] && ARGS+=(-i "$KEYS" -id "$WAIT")
ARGS+=(-it -ix)
( cd "$TMP" && xvfb-run -a timeout 300 "$EMU" "${ARGS[@]}" >emu.log 2>&1 )
SNAP="$TMP/trs80-text-0.bin"
[ -f "$SNAP" ] || { echo "no screen snapshot; emulator log:"; cat "$TMP/emu.log"; exit 1; }
perl -e 'local $/; open my $f,"<:raw",$ARGV[0] or die; my $s=<$f>;
  for my $r (0..23) { my $l=substr($s,$r*80,80); $l=~tr/\x80-\xff/\x00-\x7f/;
    $l=~s/[^\x20-\x7e]/./g; $l=~s/\s+$//; printf "%02d|%s\n",$r,$l }' "$SNAP"
