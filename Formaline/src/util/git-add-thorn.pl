#! /usr/bin/perl -w

# Add a thorn to the repository

# 2010-01-29 Erik Schnetter <schnetter@cct.lsu.edu>

use strict;
use File::Path;



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

# system "cd $git_root && $git_cmd add @files" or die;

my $srcdir = $git_root;
my $dstdir = "$scratch/tmp-$thorn";

rmtree $dstdir;                 # ignore errors

for my $file (@files) {
    my $dir = $file;
    if ($dir =~ m+/+) {
        $dir =~ s+/[^/]*$++;
    } else {
        $dir = '.';
    }
    mkpath "$dstdir/$dir";      # ignore errors
    link "$srcdir/$file", "$dstdir/$file" or
        die "ERROR: Cannot create hard link from \"$srcdir/$file\" to \"$dstdir/$file\"";
}

print "Executing: cd $dstdir && $git_cmd add @files\n" unless $silent;
system "cd $dstdir && $git_cmd add @files $silencer";
if ($?) {
    die "Could not add thorn $thorn to git repository";
}

rmtree $dstdir or die;
