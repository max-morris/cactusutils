#! /usr/bin/perl -w

# Add a thorn to the repository

# 2010-01-29 Erik Schnetter <schnetter@cct.lsu.edu>

use strict;
use File::Path;
use File::stat;



$#ARGV >= 3 or die;
my ($git_cmd, $git_repo, $git_root, $thorn, @files) = @ARGV;

my $scratch = $ENV{'SCRATCH_BUILD'};
defined $scratch or die;

my $silent = $ENV{'SILENT'};
$silent = 'yes' if ! defined $silent;
$silent = $silent !~ /^no$/i;
my $silencer = $silent ? '> /dev/null 2>&1' : '';



# Ensure that the repository exists
die unless -e "$git_repo/.git";
$ENV{'GIT_DIR'} = "$git_repo/.git";



# This does not work, because arrangements or thorns may be symbolic
# links, and git refuses to follow them.  Instad, we copy the whole
# thorn (using hard links), and then add the copy.

# system "cd '$git_root' && $git_cmd add @files" or die;

my $srcdir = $git_root;

for my $file (@files) {
    if (! -f "$file") {                 # only accept normal files.
        die "ERROR: Refusing to add \"$srcdir/$file\" as it is not a regular file";
    }
    my $st = stat ($file) or die "ERROR: could not stat \"$srcdir/$file\"";
    my $mode = sprintf ("%o", $st->mode);
    my $hash = `cd $git_root ; git hash-object -w --stdin <$file $silencer`;
    chomp $hash;
    # use 3-arguments version of cacheinfo due to old git versions on some clusters
    system "$git_cmd update-index --add --cacheinfo $mode $hash $file $silencer";
}
