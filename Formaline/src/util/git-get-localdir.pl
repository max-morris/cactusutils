#! /usr/bin/perl -w

# Determine the local git repository directory

# 2011-11-13 Erik Schnetter <eschnetter@perimeterinstitute.ca>

use strict;



$#ARGV == 0 or die;
my ($git_cmd) = @ARGV;

my $CCTK_HOME = $ENV{'CCTK_HOME'};

# Path where the git-*.pl commands are installed
my $bindir = $ENV{'SCRATCH_BUILD'} . '/Formaline/bin';

my $silent = $ENV{'SILENT'};
$silent = 'yes' if ! defined $silent;
$silent = $silent !~ /^no$/i;
my $silencer = $silent ? '> /dev/null 2>&1' : '';



# Obtain local machine name
my $machine = `cd ${CCTK_HOME} && ${CCTK_HOME}/bin/sim whoami`;
chomp $machine;
$machine =~ s{Current machine:\s*(.*)}{$1};
exit 0 if $machine eq '';

# Obtain sourcebasedir
my @mdb=`cd ${CCTK_HOME} && ${CCTK_HOME}/bin/sim print-mdb "${machine}"`;
@mdb = grep m{^\s*sourcebasedir\s*=}, @mdb;
exit 1 if @mdb != 1;
my $sourcebasedir = $mdb[0];
$sourcebasedir =~ s{^[^=]*=\s*(.*)}{$1};
chomp $sourcebasedir;
exit 0 if $sourcebasedir eq '';

# Replace variables
if ($ENV{'USER'}) {
    my $USER = $ENV{'USER'};
    $sourcebasedir =~ s{\@USER\@}{${USER}}g;
}

# Define local git repo name
my $git_local_repo = "${sourcebasedir}/CactusSourceJar.git";

# Create local git repo if it does not exist
# (Don't print anything to stdout)
system "${bindir}/git-init-local-repo.pl '$git_cmd' '$git_local_repo' >&2";

# Output git repo name
print "CACTUS_LOCAL_GIT_REPO='${git_local_repo}'\n";
