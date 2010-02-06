#! /usr/bin/perl -w

# Initialise a repository

# 2010-01-29 Erik Schnetter <schnetter@cct.lsu.edu>

use strict;



$#ARGV == 1 or die;
my ($git_cmd, $git_repo) = @ARGV;

my $silent = $ENV{'SILENT'};
$silent = 'yes' if ! defined $silent;
$silent = $silent !~ /^no$/i;
my $silencer = $silent ? '> /dev/null 2>&1' : '';



# If the repository exists already, do nothing
if (-e "$git_repo/.git") {
    exit;
}



print "Formaline: Creating git repository...\n";

# Create the directory for the repository
mkdir $git_repo;
$ENV{'GIT_DIR'} = "$git_repo/.git";

# Create the repository
print "Executing: $git_cmd init-db\n" unless $silent;
system "$git_cmd init-db $silencer";
if ($?) {
    die "Formaline: WARNING: Error while initialising git repository";
}



# Add a README
open README, "> $git_repo/README" or die;
my $today = `date`;
print README "\
This directory $git_repo

is not empty -- it contains a git repository with the Cactus source
trees of all previous builds, starting on $today.

You can use the command \"git branch\" to list all configurations that
are stored in this repository.  The history of each branch is the
sequence in which the configuration was built.  The most recent
build is stored in the branch head, as usual.  In order to check out a
certain branch into a directory <name>, issue the following commands:
        cd <somewhere_else>
        mkdir <name>
        cd <name>
        git init
        git pull $git_repo <branch>

You can also use the command \"git tag -l\" to list all builds that
are stored in this repository.  This keeps the source tree for each
build directly accessible.  In order to check out a certain tag into a
directory <name>, issue the following commands:
        cd <somewhere_else>
        git clone -o <name> $git_repo
        git checkout <tag>"
    or die;
close README or die;
