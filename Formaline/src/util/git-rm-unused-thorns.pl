#! /usr/bin/perl -w

# Remove all Cactus thorns from the repository index unless they are
# in the thorn list

# 2010-04-08 Erik Schnetter <schnetter@cct.lsu.edu>

use strict;



$#ARGV >= 3 or die;
my ($git_cmd, $git_repo, $git_root, @thorns) = @ARGV;

my $silent = $ENV{'SILENT'};
$silent = 'yes' if ! defined $silent;
$silent = $silent !~ /^no$/i;
my $silencer = $silent ? '> /dev/null 2>&1' : '';



# Ensure that the repository exists
die unless -e "$git_repo/.git";
$ENV{'GIT_DIR'} = "$git_repo/.git";



my @files = split m{\n}, `$git_cmd ls-files arrangements 2> /dev/null`;

# Remove the files one by one because we want to ignore errors, but
# git aborts after the first error
file: for my $file (@files) {
    for my $thorn (@thorns) {
        next file if $file =~ m{^arrangements/$thorn/};
    }
    
    print "Executing: $git_cmd rm --cached -r $file\n"
        unless $silent;
    system "$git_cmd rm --cached -r $file > /dev/null 2>&1";
    # Ignore errors
    #if ($?) {
    #    die "Could not remove file $file from git repository";
    #}
}
