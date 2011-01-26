#! /usr/bin/perl -w

# Commit everything to the repository

# 2010-01-29 Erik Schnetter <schnetter@cct.lsu.edu>

use strict;



$#ARGV == 4 or die;
my ($git_cmd, $git_repo, $git_root, $build_id, $config_id) = @ARGV;

# Path where the git-*.pl commands are installed
my $bindir = $ENV{'SCRATCH_BUILD'} . '/Formaline/bin';

my $silent = $ENV{'SILENT'};
$silent = 'yes' if ! defined $silent;
$silent = $silent !~ /^no$/i;
my $silencer = $silent ? '> /dev/null 2>&1' : '';



# Ensure that the repository exists
die unless -e "$git_repo/.git";
$ENV{'GIT_DIR'} = "$git_repo/.git";



print "Formaline: Committing source tree to git repository...\n";

# Try to use the previous commit as parent, if possible
print "Executing: $git_cmd commit -m $build_id\n" unless $silent;
system "$git_cmd commit -m $build_id $silencer";
# Ignore errors
#if ($?) {
#    die "Could not commit";
#}

print "Executing: $git_cmd tag '$build_id'\n" unless $silent;
system "$git_cmd tag '$build_id' $silencer";
if ($?) {
    die "Could not tag\nCommand was\n   $git_cmd tag '$build_id'";
}
print "Formaline: Created git tag $build_id\n";

print "Executing: $git_cmd branch -f '$config_id'\n" unless $silent;
system "$git_cmd branch -f '$config_id' $silencer";
if ($?) {
    die "Could not update branch\nCommand was\n   $git_cmd branch -f '$config_id'";
}
print "Formaline: Updated git branch $config_id\n";

print "Executing: '$bindir/git-gc-repo.pl' '$git_cmd' '$git_repo'\n" unless $silent;
system "'$bindir/git-gc-repo.pl' '$git_cmd' '$git_repo' $silencer";
if ($?) {
    die "Could not collect garbage\nCommand was\n   '$bindir/git-gc-repo.pl' '$git_cmd' '$git_repo'";
}
