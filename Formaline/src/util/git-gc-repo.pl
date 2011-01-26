#! /usr/bin/perl -w

# Collect garbage in the repository if it grown in size by more than a
# factor of 10.

# 2010-01-29 Erik Schnetter <schnetter@cct.lsu.edu>

use strict;



$#ARGV == 1 or die;
my ($git_cmd, $git_repo) = @ARGV;

my $silent = $ENV{'SILENT'};
$silent = 'yes' if ! defined $silent;
$silent = $silent !~ /^no$/i;
my $silencer = $silent ? '> /dev/null 2>&1' : '';



# Ensure that the repository exists
die unless -e "$git_repo/.git";
$ENV{'GIT_DIR'} = "$git_repo/.git";



my $git_dir = "$git_repo/.git";
my $sizefile = "$git_repo/.oldreposize";

# Determine current repository size
my $reposize = `du -s $git_dir` or die;
$reposize = (split ' ', $reposize)[0];

# Read old repository size
my $oldreposize;
if (open FILE, '<', $sizefile) {
    $oldreposize = <FILE>;
    close FILE;
}
if (! defined $oldreposize) {
    $oldreposize = 0;
}

# Collect garbage once the repository has grown by more than a factor
# of two
my $maxreposize = 2 * $oldreposize;
if ($reposize > $maxreposize) {
    
    print "Formaline: Optimising git repository (slow only the first time)...\n";
    
    # Collect garbage
    print "Executing: $git_cmd gc\n" unless $silent;
    system "$git_cmd gc $silencer";
    if ($?) {
        die "Could not compact repository\nCommand was\n   $git_cmd gc";
    }
    
    # Determine new repository size
    my $newreposize = `du -s '$git_dir'` or die;
    $newreposize = (split ' ', $newreposize)[0];
    
    # Write new repository size
    open (FILE, "> $sizefile") or die;
    print FILE "$newreposize\n" or die;
    close FILE or die;
}
