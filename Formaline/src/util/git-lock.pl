#! /usr/bin/perl -w

# Obtain a lock for a git repository

# 2008-03-06 Erik Schnetter <schnetter@cct.lsu.edu>

use strict;
use Cwd;
#use Fcntl ':flock';
use sigtrap qw(die normal-signals);



# Obtain exclusive access to the repository

# We cannot use flock since the file system may be remote
#open LOCKFILE, "$cctk_home/Makefile";
#flock LOCKFILE, LOCK_EX;

my $git_dir = $ENV{'GIT_DIR'};
$git_dir = getcwd unless defined $git_dir;
$git_dir =~ s+/.git/*$+/+;

# We are very conservative as to what characters we allow.  Other
# characters can be added, but be careful.
$git_dir =~ m|^[[:alnum:]+-._/]+$| or die;

my $lockdir = "$git_dir/GITLOCK";



my $waittime = 0.01;
my $maxwaittime = 10;
while (! (mkdir $lockdir)) {
    # Wait some time
    my $unit = $waittime==1 ? "second" : "seconds";
    print "Git repository is busy; waiting $waittime $unit...\n";
    system "sleep '$waittime'";
    # Back off exponentially
    $waittime *= 2;
    $waittime = 1 if $waittime>1 && $waittime<2;
    $waittime = $maxwaittime if $waittime > $maxwaittime;
}



# Execute the command
system @ARGV;
$? != -1 or die;
exit $? >> 8;



# Release the lock
END {
    my $retval = $?;
    rmdir $lockdir;
    $? = $retval;
}
