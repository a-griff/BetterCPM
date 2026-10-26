#!/usr/bin/perl
# m2chat.pl - talk to a TRS-80 Model II serial port over TCP (trs80gp -rA @PORT)
#
# Runs a chat script of -expect and -send steps in order, printing everything
# received. Exits 0 when every step succeeds, 1 on a timeout or lost link.
use strict; use warnings;
use IO::Socket::INET;
use IO::Select;
use Time::HiRes qw(time sleep);

my %o = (host => '127.0.0.1', port => 0, timeout => 60, connect => 60, pace => 0);
my @steps;
while (@ARGV) {
    my $a = shift;
    if    ($a eq '-help')    { usage(); exit 0 }
    elsif ($a eq '-host')    { $o{host}    = shift // die "-host needs NAME\n" }
    elsif ($a eq '-port')    { $o{port}    = shift // die "-port needs N\n" }
    elsif ($a eq '-timeout') { $o{timeout} = shift // die "-timeout needs SECONDS\n" }
    elsif ($a eq '-connect') { $o{connect} = shift // die "-connect needs SECONDS\n" }
    elsif ($a eq '-pace')    { $o{pace}    = shift // die "-pace needs MS\n" }
    elsif ($a eq '-expect')  { push @steps, [expect => unescape(shift // die "-expect needs TEXT\n")] }
    elsif ($a eq '-send')    { push @steps, [send   => unescape(shift // die "-send needs TEXT\n")] }
    elsif ($a eq '-sendfile'){ push @steps, [file   => (shift // die "-sendfile needs FILE\n")] }
    elsif ($a eq '-drain')   { push @steps, [drain  => (shift // die "-drain needs SECONDS\n")] }
    elsif ($a eq '-save')    { push @steps, [save   => (shift // die "-save needs FILE\n")] }
    else                     { die "unknown argument $a (try -help)\n" }
}
$o{port} or do { usage(); exit 1 };

sub usage {
    print <<"EOT";
usage: m2chat.pl -port N [options] [steps]
  -host NAME      host to connect to (default 127.0.0.1)
  -port N         TCP port trs80gp listens on (trs80gp -rA \@N)
  -connect SECS   keep retrying the connection this long (default 60)
  -timeout SECS   longest wait for each -expect (default 60)
  -pace MS        delay between characters sent (default 0)
steps, run in order:
  -expect TEXT    wait until TEXT has been received
  -send TEXT      send TEXT (\\r, \\n, \\e, \\xNN and \\\\ are understood)
  -sendfile FILE  send the bytes of FILE
  -drain SECS     keep receiving for SECS seconds
  -save FILE      write everything received since the last -save to FILE
  -help           this list
Everything received is also printed to standard output.
EOT
}

sub unescape {
    my $s = shift;
    $s =~ s/\\(r|n|e|x([0-9A-Fa-f]{2})|\\)/$1 eq 'r' ? "\r" : $1 eq 'n' ? "\n" : $1 eq 'e' ? "\e" : $1 eq '\\' ? '\\' : chr hex $2/ge;
    $s;
}

my $sock;
my $until = time + $o{connect};
until ($sock = IO::Socket::INET->new(PeerAddr => $o{host}, PeerPort => $o{port}, Proto => 'tcp')) {
    die "m2chat: cannot connect to $o{host}:$o{port}\n" if time > $until;
    sleep 0.2;
}
binmode $sock; $sock->autoflush(1);
binmode STDOUT; $| = 1;
my $sel = IO::Select->new($sock);
my $seen = '';           # received text not yet matched by -expect
my $saved = '';          # received since the last -save

sub receive {            # wait up to $wait seconds for data; false on a closed link
    my $wait = shift;
    return 1 unless $sel->can_read($wait);
    my $n = sysread $sock, my $buf, 4096;
    return 0 unless $n;
    print $buf; $seen .= $buf; $saved .= $buf;
    1;
}

sub transmit {
    my $data = shift;
    if ($o{pace} > 0) {
        for my $c (split //, $data) { syswrite $sock, $c; receive($o{pace} / 1000) }
    } else {
        syswrite $sock, $data;
    }
}

for my $step (@steps) {
    my ($kind, $arg) = @$step;
    if ($kind eq 'expect') {
        my $end = time + $o{timeout};
        while (1) {
            my $at = index $seen, $arg;
            if ($at >= 0) { $seen = substr $seen, $at + length $arg; last }
            my $left = $end - time;
            if ($left <= 0) { print STDERR "\nm2chat: timed out waiting for '$arg'\n"; exit 1 }
            receive($left > 0.5 ? 0.5 : $left) or do { print STDERR "\nm2chat: link closed\n"; exit 1 };
        }
    } elsif ($kind eq 'send') {
        transmit($arg);
    } elsif ($kind eq 'file') {
        open my $h, '<:raw', $arg or die "m2chat: $arg: $!\n";
        local $/; transmit(<$h> // '');
    } elsif ($kind eq 'drain') {
        my $end = time + $arg;
        while ((my $left = $end - time) > 0) { receive($left > 0.5 ? 0.5 : $left) or last }
    } elsif ($kind eq 'save') {
        open my $h, '>:raw', $arg or die "m2chat: $arg: $!\n";
        print $h $saved; close $h; $saved = '';
    }
}
close $sock;
exit 0;
