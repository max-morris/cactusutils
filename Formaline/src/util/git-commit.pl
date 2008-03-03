#! /usr/bin/perl -w

# Commit the current source tree to a git repository

# 2008-03-02 Erik Schnetter <schnetter@cct.lsu.edu>

use strict;
#use Fcntl ':flock';
use Getopt::Long;

# $(GIT-REPO)
# $(CCTK-HOME)
# $(CONFIG-DIR)
# $(SCRATCH-DIR)

my %options;
GetOptions
    (\%options,
     'git=s',                   # git command
     'git-repo=s',              # git repository
     'CCTK_HOME=s',             # main Cactus directory
     'TOP=s',                   # main directory of the configuration
     'TARBALL_DIR=s',           # Formaline's directory
    ) or die "Could not parse command line options";

my $git         = $options{'git'};
my $git_repo    = $options{'git-repo'};
my $cctk_home   = $options{'CCTK_HOME'};
my $config_dir  = $options{'TOP'};
my $tarball_dir = $options{'TARBALL_DIR'};

defined $git         or die;
defined $git_repo    or die;
defined $cctk_home   or die;
defined $config_dir  or die;
defined $tarball_dir or die;

chdir $git_repo or die;

# Use relative path names
$config_dir =~ m+/configs/+;
$config_dir = "configs/$'";

my $config_id_file = "$config_dir/CONFIG-ID";
my $build_id_file  = "$config_dir/BUILD-ID";
my $commit_id_file = "$config_dir/GIT-COMMIT-ID";

my @thorns = @ARGV;



# Obtain exclusive access to the repository

# We cannot use flock since the file system may be remote
#open LOCKFILE, "$cctk_home/Makefile";
#flock LOCKFILE, LOCK_EX;

my $gitlock = "$cctk_home/GITLOCK";
my $waittime = 1;
my $maxwaittime = 30;
while (! (mkdir $gitlock)) {
    # Check whether the locking process still exists
    my $pid = `cat $gitlock/PID`;
    chomp $pid;
    if ($pid ne '') {
        system "ps $pid > /dev/null 2>&1";
        if ($?) {
            # Break the lock
            print "Git repository seems free -- breaking the lock\n";
            unlink "$gitlock/PID" or die;
            rmdir "$gitlock" or die;
            $waittime = 1;
            next;
        }
    }
    # Wait some time
    my $unit = $waittime==1 ? "second" : "seconds";
    print "Git repository is busy; waiting $waittime $unit...\n";
    system "sleep $waittime";
    # Back off exponentially
    $waittime *= 2;
    $waittime = $maxwaittime if $waittime > $maxwaittime;
}
# Ensure that the lock exists
die unless -d $gitlock;
# Add our PID
open PID, '>', "$gitlock/PID.tmp" or die;
print PID "$$\n";
close PID;
rename "$gitlock/PID.tmp", "$gitlock/PID" or die;
# Check whether it is really our PID
my $pid = `cat $gitlock/PID`;
chomp $pid;
die unless $pid == $$;



# Ensure that the git repository exists
if (! -e "$git_repo/.git/HEAD") {
    print "Formaline: Creating new git repository...\n";
    system "$git init"; $? and die;
    # Add a root commit to please some tools
    system ": >> README"; $? and die;
    system "$git add README"; $? and die;
    system "$git commit -m 'README'"; $? and die;
}

# Remove all staged files
print "Formaline: Preparing git repository...\n";
system "$git rm --cached -r . > /dev/null 2>&1"; # may fail

# Add flesh files to repository
print "Formaline: Adding flesh to git repository...\n";
system "xargs ls < $tarball_dir/cactus-flesh-source.files > $tarball_dir/cactus-flesh-source.files.tmp 2> /dev/null"; $? and die;
system "xargs $git add < $tarball_dir/cactus-flesh-source.files.tmp"; $? and die;
unlink "$tarball_dir/cactus-flesh-source.files.tmp" or die;

# Add thorn files to repository
foreach my $thorn (@thorns) {
    print "Formaline: Adding thorn $thorn to git repository...\n";
    system "xargs ls < $tarball_dir/cactus-thorn-source-$thorn.files > $tarball_dir/cactus-thorn-source-$thorn.files.tmp 2> /dev/null"; $? and die;
    system "xargs $git add < $tarball_dir/cactus-thorn-source-$thorn.files.tmp"; $? and die;
    unlink "$tarball_dir/cactus-thorn-source-$thorn.files.tmp" or die;
}

# Write source tree and create a commit
print "Formaline: Committing source tree to git repository...\n";
system "$git add $config_id_file $build_id_file"; $? and die;
my $tree_id_file = "$tarball_dir/GIT-TREE-ID";
system "$git write-tree > $tree_id_file"; $? and die;
my $tree_id = `cat $tree_id_file`;
chomp $tree_id;

# Try to use the previous commit as parent, if possible
my $old_commit_id;
if (-e $commit_id_file) {
    $old_commit_id = `cat $commit_id_file`;
    chomp $old_commit_id;
}

# (Use the build id as commit message)
my $did_commit = 0;
if (defined $old_commit_id && $old_commit_id ne '') {
    system "$git commit-tree $tree_id -p $old_commit_id < $build_id_file > $commit_id_file.new";
    $did_commit = $? == 0;
}
if (! $did_commit) {
    system "$git commit-tree $tree_id < $build_id_file > $commit_id_file.new"; $? and die;
}
rename "$commit_id_file.new", "$commit_id_file" or die;
my $commit_id = `cat $commit_id_file`;
chomp $commit_id;
print "Formaline: Git commit id is $commit_id\n";

# Remember the commit id
mkdir "$cctk_home/GIT-COMMIT-IDS";
system "cp $commit_id_file $cctk_home/GIT-COMMIT-IDS/$commit_id"; $? and die;

# Update branch
my $config_id = `cat $config_id_file`;
chomp $config_id;
die unless defined $config_id && $config_id ne '';
system "$git branch -f $config_id $commit_id"; $? and die;

print "Formaline: Cleaning up git repository...\n";
system "$git rm --cached -r . > /dev/null 2>&1"; # may fail

# Call git-gc for good measure
print "Formaline: Optimising git repository (slow only the first time)...\n";
system "$git gc > /dev/null 2>&1"; $? and die;



# Release the lock
system "rm -rf $gitlock";

print "Formaline: Done.\n";
