#! /usr/bin/perl -w

# Push everything to the master repository

# 2010-01-29 Erik Schnetter <schnetter@cct.lsu.edu>

use strict;



$#ARGV == 2 or die;
my ($git_cmd, $git_repo, $git_master_repo) = @ARGV;

# Local and central Cactus repositories
my $git_local_repo = $ENV{'CACTUS_LOCAL_GIT_REPO'};
my $git_central_repo = $ENV{'CACTUS_CENTRAL_GIT_REPO'};

# Path where the git-*.pl commands are installed
my $bindir = $ENV{'SCRATCH_BUILD'} . '/Formaline/bin';

my $silent = $ENV{'SILENT'};
$silent = 'yes' if ! defined $silent;
$silent = $silent !~ /^no$/i;
my $silencer = $silent ? '> /dev/null 2>&1' : '';



# Ensure that the repositories exist
die unless -e "$git_repo/.git";
$ENV{'GIT_DIR'} = "$git_repo/.git";
die unless -e "$git_master_repo/.git";



print "Formaline: Pushing source tree to master git repository...\n";

print "Executing: $git_cmd config receive.denyCurrentBranch false\n" unless $silent;
system "$git_cmd --git-dir='$git_master_repo/.git' config receive.denyCurrentBranch false $silencer";
if ($?) {
    die "Formaline: WARNING: Error while configuring master git repository\nCommand was\n   $git_cmd --git-dir='$git_master_repo/.git' config receive.denyCurrentBranch false";
}

print "Executing: $git_cmd push -v -f --all '$git_master_repo'\n" unless $silent;
system "$git_cmd push -v -f --all '$git_master_repo' $silencer";
if ($?) {
    die "Could not push branches\nCommand was\n   $git_cmd push -v -f --all '$git_master_repo'";
}

print "Executing: $git_cmd push -v -f --tags '$git_master_repo'\n" unless $silent;
system "$git_cmd push -v -f --tags '$git_master_repo' $silencer";
if ($?) {
    die "Could not push tags\nCommand was\n   $git_cmd push -v -f --tags '$git_master_repo'";
}

print "Executing: '$bindir/git-gc-repo.pl' '$git_cmd' '$git_master_repo'\n"
    unless $silent;
system "'$bindir/git-gc-repo.pl' '$git_cmd' '$git_master_repo' $silencer";
if ($?) {
    die "Could not collect garbage\nCommand was\n   '$bindir/git-gc-repo.pl' '$git_cmd' '$git_master_repo'";
}



if (defined $git_local_repo) {
    $ENV{'GIT_DIR'} = "$git_master_repo/.git";
    
    print "Formaline: Pushing to local repository $git_local_repo...\n";
    
    print "Executing: $git_cmd push -v -f --all '$git_local_repo'\n"
        unless $silent;
    system "$git_cmd push -v -f --all '$git_local_repo' $silencer";
    if ($?) {
        die "Could not push branches\nCommand was\n   $git_cmd push -v -f --all '$git_local_repo'";
    }
    
    print "$git_cmd push -v -f --tags '$git_local_repo'\n" unless $silent;
    system "$git_cmd push -v -f --tags '$git_local_repo' $silencer";
    if ($?) {
        die "Could not push tags\nCommand was\n   $git_cmd push -v -f --tags '$git_local_repo'";
    }
    
    system "${bindir}/git-gc-repo.pl '$git_cmd' '$git_local_repo'";
}



if (defined $git_central_repo) {
    $ENV{'GIT_DIR'} = "$git_master_repo/.git";
    
    print "Formaline: Pushing to central repository $git_central_repo...\n";
    
    print "Executing: $git_cmd push -v -f --all '$git_central_repo'\n"
        unless $silent;
    system "$git_cmd push -v -f --all '$git_central_repo' $silencer";
    if ($?) {
        die "Could not push branches\nCommand was\n   $git_cmd push -v -f --all '$git_central_repo'";
    }
    
    print "$git_cmd push -v -f --tags '$git_central_repo'\n" unless $silent;
    system "$git_cmd push -v -f --tags '$git_central_repo' $silencer";
    if ($?) {
        die "Could not push tags\nCommand was\n   $git_cmd push -v -f --tags '$git_central_repo'";
    }
}
