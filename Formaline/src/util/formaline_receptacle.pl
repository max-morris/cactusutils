#!/usr/bin/perl -Tw

# Taken from http://www.perl.com/doc/FMTEYEWTK/IPC/inet.html
# Adapted 2005-10-07 by Erik Schnetter <schnetter@cct.lsu.edu>

require 5.002;

use strict;
BEGIN { $ENV{PATH} = '/usr/ucb:/bin' }
use Socket;
use Carp;

use File::Temp qw/ :mktemp /;

sub spawn;  # forward declaration
sub logmsg { print "$0 $$: @_ at ", scalar localtime, "\n" } 

my $port = shift || 9296;
my $proto = getprotobyname('tcp');

socket(SERVER, PF_INET, SOCK_STREAM, $proto)        || die "socket: $!";
setsockopt(SERVER, SOL_SOCKET, SO_REUSEADDR, 1)     || die "setsockopt: $!";
bind(SERVER, sockaddr_in($port, INADDR_ANY))        || die "bind: $!";
listen(SERVER,5)                                    || die "listen: $!";
logmsg "server started on port $port";

my $waitedpid = 0;
my $paddr;

sub REAPER
{ 
    $SIG{CHLD} = \&REAPER;  # loathe sysV
    $waitedpid = wait;
    logmsg "reaped $waitedpid" . ($? ? " with exit $?" : '');
}

$SIG{CHLD} = \&REAPER;
for ($waitedpid = 0; 
     ($paddr = accept(CLIENT,SERVER)) || $waitedpid; 
     $waitedpid = 0, close CLIENT) 
{
    next if $waitedpid;
    my($port,$iaddr) = sockaddr_in($paddr);
    my $name = gethostbyaddr($iaddr,AF_INET);
    logmsg "connection from $name [", inet_ntoa($iaddr), "] at port $port";
    spawn sub {
	my ($fh, $file) = mkstemps ('output-XXXXXXXX', '.txt');
	my $oldline = '';
	while (my $line = <STDIN>) {
	    print $fh $line;
	    last if ($oldline eq "\r\n" && $line eq "\r\n");
	    $oldline = $line;
	}
	close $fh;
    };
}

sub spawn
{
    my $coderef = shift;
    unless (@_ == 0 && $coderef && ref($coderef) eq 'CODE') { 
        confess "usage: spawn CODEREF";
    }
    my $pid;
    if (!defined($pid = fork)) {
        logmsg "cannot fork: $!";
        return;
    } elsif ($pid) {
        logmsg "forked $pid";
        return; # I'm the parent
    }
    # else I'm the child -- go spawn
    open(STDIN,  "<&CLIENT")   || die "can't dup client to stdin";
    open(STDOUT, ">&CLIENT")   || die "can't dup client to stdout";
    ## open(STDERR, ">&STDOUT") || die "can't dup stdout to stderr";
    exit &$coderef();
}
