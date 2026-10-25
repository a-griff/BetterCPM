#!/usr/bin/perl
# mkm2imd.pl - build a TRS-80 Model II 8" ImageDisk (.IMD) image
#
# Cylinder 0 is FM, 26 x 128-byte sectors (the ROM boot track).
# Cylinders 1.. are MFM, 16 x 512-byte sectors.
use strict; use warnings;

my %o = (cyl => 77, sides => 1, out => '');
my ($boot, $sys);
while (@ARGV) {
    my $a = shift;
    if    ($a eq '-help')  { usage(); exit 0 }
    elsif ($a eq '-boot')  { $boot    = shift // die "-boot needs FILE\n" }
    elsif ($a eq '-sys')   { $sys     = shift // die "-sys needs FILE\n" }
    elsif ($a eq '-cyl')   { $o{cyl}  = shift // die "-cyl needs N\n" }
    elsif ($a eq '-sides') { $o{sides}= shift // die "-sides needs 1|2\n" }
    elsif ($a eq '-out')   { $o{out}  = shift // die "-out needs FILE\n" }
    else                   { die "unknown argument $a (try -help)\n" }
}
$o{out} or do { usage(); exit 1 };

sub usage {
    print <<"EOT";
usage: mkm2imd.pl -out IMAGE.IMD [options]
  -boot FILE   boot track contents (up to 3328 bytes, loaded by the ROM at 0E00H)
  -sys FILE    bytes written from cylinder 1 sector 1 onward (512-byte sectors)
  -cyl N       cylinders (default 77)
  -sides N     1 or 2 (default 1; cylinder 0 side 1 is MFM like the rest)
  -out FILE    output image
  -help        this list
Unwritten sectors are filled with E5H.
EOT
}

sub slurp { my $f = shift; open my $h, '<:raw', $f or die "$f: $!\n"; local $/; my $d = <$h>; $d // '' }

my $bt = defined $boot ? slurp($boot) : '';
die "boot track is ", length $bt, " bytes; limit 3328\n" if length $bt > 26 * 128;
my $sd = defined $sys ? slurp($sys) : '';

my @t = localtime;
my $img = sprintf "IMD 1.18: %02d/%02d/%04d %02d:%02d:%02d\r\nBetterCP/M Model II\r\n\x1a",
    $t[3], $t[4] + 1, $t[5] + 1900, $t[2], $t[1], $t[0];

sub track {
    my ($mode, $c, $h, $n, $szcode, $getdata) = @_;
    my $out = pack 'C5', $mode, $c, $h, $n, $szcode;
    $out .= pack 'C*', 1 .. $n;
    my $len = 128 << $szcode;
    for my $s (1 .. $n) {
        my $d = $getdata->($s);
        $d .= "\xE5" x ($len - length $d);
        if ($d eq substr($d, 0, 1) x $len) { $out .= "\x02" . substr($d, 0, 1) }
        else                               { $out .= "\x01" . $d }
    }
    $out;
}

my $sysoff = 0;
for my $c (0 .. $o{cyl} - 1) {
    for my $h (0 .. $o{sides} - 1) {
        if ($c == 0 && $h == 0) {
            $img .= track(0, 0, 0, 26, 0, sub { my $s = shift;
                my $p = ($s - 1) * 128; $p < length $bt ? substr($bt, $p, 128) : '' });
        } else {
            $img .= track(3, $c, $h, 16, 2, sub {
                my $d = $sysoff < length $sd ? substr($sd, $sysoff, 512) : '';
                $sysoff += 512; $d });
        }
    }
}
die "system data (", length $sd, " bytes) does not fit\n" if length $sd > $sysoff;

open my $oh, '>:raw', $o{out} or die "$o{out}: $!\n";
print $oh $img; close $oh;
printf "%s: %d cylinders, %d side(s), boot %d bytes, system %d bytes\n",
    $o{out}, $o{cyl}, $o{sides}, length $bt, length $sd;
