#! /usr/bin/perl -w

# Remove a Cactus thorn from the repository index, if and wherever it
# exists

# 2010-01-29 Erik Schnetter <schnetter@cct.lsu.edu>

use strict;



$#ARGV >= 4 or die;
my ($git_cmd, $git_repo, $git_root, $thorn, @files) = @ARGV;

my $silent = $ENV{'SILENT'};
$silent = 'yes' if ! defined $silent;
$silent = $silent !~ /^no$/i;
my $silencer = $silent ? '> /dev/null 2>&1' : '';



# Ensure that the repository exists
die unless -e "$git_repo/.git";
$ENV{'GIT_DIR'} = "$git_repo/.git";



# Remove the files one by one because we want to ignore errors, but
# git aborts after the first error
for my $file (@files) {
    
    print "Executing: $git_cmd rm --cached -r '$file' 2> /dev/null\n"
        unless $silent;
    system "$git_cmd rm --cached -r '$file' > /dev/null 2>&1";
    # Ignore errors
    #if ($?) {
    #    die "Could not remove thorn $thorn from git repository";
    #}
    
}
