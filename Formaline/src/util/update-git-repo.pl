#! /usr/bin/perl -w

# 2015-10-17 Erik Schnetter <schnetter@gmail.com>

# Take a snapshot of the Cactus source tree rooted at the current directory,
# creating a commit in the git repository located at $git_repo. The repository
# is expected to be for a single Cactus configuration, and is created if it does
# not exist. The commit is daisy-chained onto the previous commit (if there is
# one). A branch is created with a name unique to this machine, source tree, and
# configuration; and a tag is created that additionally contains a unique time
# stamp. These use the unique configuration and build ids from Formaline.

# The content of the git repository is then pushed into a "master" repository
# for the source tree (which thus collects information about all
# configurations). This repo is in turn pushed into a "local" repository,
# collecting information about all source trees on this machine.

# Furthermore, if a remote "central" git repository is configured (url, user
# name, ssh key), then the master and/or local repo are pushed there. This
# "central" git repository might e.g. be located on Bitbucket.

use strict;
use File::Path;
use File::stat;

# Read arguments
$#ARGV == 6 or die;
my ($git_cmd, $git_repo, $git_master_repo, $git_local_repo, $git_central_repo,
    $build_id, $config_id) = @ARGV;

my $silent = $ENV{'SILENT'};
$silent = 'yes' if ! defined $silent;
$silent = $silent !~ /^no$/i;

if (!$silent) {
  print "Formaline: git_cmd [$git_cmd]\n";
  print "Formaline: git_repo [$git_repo]\n";
  print "Formaline: git_master_repo [$git_master_repo]\n";
  print "Formaline: git_local_repo [$git_local_repo]\n";
  print "Formaline: git_central_repo [$git_central_repo]\n";
  print "Formaline: build_id [$build_id]\n";
  print "Formaline: config_id [$config_id]\n";
}

sub runcmd($$);
sub callcmd($$);
sub init_repo($$);
sub gc_repo($);
sub main();

sub runcmd($$)
{
  $#_ == 1 or die;
  my ($descr, $cmd) = @_;
  print "Formaline: executing: $cmd\n" unless $silent;
  my $out = `{ $cmd; } 2>&1`;
  print $out unless $silent;
  if ($?) {
    die "Formaline: ERROR during: $descr\nCommand was: $cmd\nOutput: $out";
  }
}

sub callcmd($$)
{
  $#_ == 1 or die;
  my ($descr, $cmd) = @_;
  print "Formaline: executing: $cmd\n" unless $silent;
  my $output = `$cmd`;
  if ($?) {
    die "Formaline: ERROR during: $descr\nCommand was: $cmd";
  }
  return $output;
}

sub init_repo($$)
{
  $#_ == 1 or die;
  my ($name, $git_repo) = @_;
  if (! -e "$git_repo/.git") {
    print "Formaline: Creating $name git repository...\n";

    # Create the directory for the repository
    mkdir $git_repo or die "mkdir $git_repo failed: $!";

    # Create the repository
    runcmd "Initializing git repository", "$git_cmd --git-dir='$git_repo/.git' init-db";
    runcmd "Configuring git repository", "$git_cmd --git-dir='$git_repo/.git' config receive.denyCurrentBranch false && $git_cmd --git-dir='$git_repo/.git' config gc.autoDetach false";

    # Add a README
    open README, ">$git_repo/README" or die "open $git_repo/README failed: $!";
    my $today = `date`;
    chomp $today;
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
            git checkout <tag>
    "
        or die "Could not write to README file: $!";
    close README or die "Could not write to README file: $!";
  }

  # Ensure the repository exists
  die unless -e "$git_repo/.git";
}

sub gc_repo($)
{
  $#_ == 0 or die;
  my ($git_repo) = @_;

  my $sizefile = "$git_repo/.oldreposize";

  # Determine current repository size
  my $reposize = `du -s $git_repo/.git` or die "du -s $git_repo/.git failed: $!";
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

  # Collect garbage once the repository has grown by more than a factor of two
  my $maxreposize = 2 * $oldreposize;
  if ($reposize > $maxreposize) {
      print "Formaline: Optimising git repository (slow only the first time)...\n";

      # Collect garbage
      runcmd "Garbage collecting git repo", "$git_cmd --git-dir='$git_repo/.git' gc";

      # Determine new repository size
      my $newreposize = `du -s '$git_repo/.git'` or die "du -s '$git_repo/.git' failed: $!";
      $newreposize = (split ' ', $newreposize)[0];

      # Write new repository size
      open (FILE, "> $sizefile") or die "open $sizefile failed: $!";
      print FILE "$newreposize\n" or die "Could not write to $sizefile: $!";
      close FILE or die "Could not write to $sizefile: $!";
  }
}

sub main()
{
  init_repo "configuration", $git_repo;

  # Determine the files that are currently in the repo
  my $have_files = callcmd "Listing files in git repository",
                           "$git_cmd --git-dir='$git_repo/.git' ls-tree --full-tree --name-only -r HEAD -- >/dev/null 2>&1 || true";
  my @have_files = split /\n/, $have_files;

  # Determine the files that should be in the repo
  my @want_files = <STDIN>;
  chomp @want_files;
  # remove tarballs (assumed to be from ExternalLibraries) from git repo
  @want_files = grep {!/\.tar\.gz$/} @want_files;
  map {s{//}{/}g;} @want_files;

  my $scratch = $ENV{'SCRATCH_BUILD'};
  defined $scratch or die "SCRATCH_BUILD environment variable not found";
  my $dstdir = "$scratch/Formaline-tmp-tree";
  rmtree $dstdir;

  print "Formaline: Updating files in git repository...\n";
  runcmd "Resetting git index", "$git_cmd --git-dir='$git_repo/.git' reset --mixed >/dev/null 2>&1 || true";

  # Remove superfluous files
  my %want_files = map {$_ => 1} @want_files;
  my @to_remove = grep {not $want_files{$_}} @have_files;

  # Add all current files to the index (since we don't know which files have
  # changed), add any missing files to list of removed files since Formaline's
  # file tracking is not perfect for files that do not affect the executable.
  for my $file (@want_files) {
    # only accept normal files, ignore missing files since our file tracking is not perfect
    if (! -e $file) {
      push @to_remove, $file;
      next;
    } elsif (! -f $file) {
      warn "WARNING: Refusing to add \"$file\" as it is not a regular file";
      next;
    }
    # first try building a shadow hierarchy of files that we can add in one git
    # command taking advantage of git's lstat caching to speed things up
    my $dir = $file;
    if ($dir =~ m+/+) {
        $dir =~ s+/[^/]*$++;
    } else {
        $dir = '.';
    }
    mkpath "$dstdir/$dir";      # ignore errors
    if(not link $file, "$dstdir/$file") {
      delete $want_files{$file}; # remove from list of files to later add

      my $st = stat $file or die "ERROR: could not stat \"$file\"";
      my $mode = sprintf "%o", $st->mode;
      my $hash = callcmd "Calculating hash for file", "$git_cmd --git-dir='$git_repo/.git' hash-object -w --stdin <'$file'";
      chomp $hash;
      # Use 3-arguments version of cacheinfo due to old git versions on some clusters

      my $configs_dir = $ENV{'CONFIGS_DIR'};
      my $relative_file = $file;
      $relative_file =~ s|^$configs_dir/|configs/|g;

      runcmd "Adding file to git repo", "$git_cmd --git-dir='$git_repo/.git' update-index --add --cacheinfo $mode $hash '$relative_file'";
    }
  }
  @want_files = grep {$want_files{$_}} keys %want_files;
  if (@want_files) {
    runcmd "Adding files @want_files to git repo", "cd $dstdir; $git_cmd --git-dir='$git_repo/.git' add --no-all .";
  }
  rmtree $dstdir or die "Failed to remove $dstdir: $!";

  # Remove the files one by one because we want to ignore errors, but git aborts
  # after the first error
  for my $file (@to_remove) {
    #runcmd "Removing file from git repo", "$git_cmd --git-dir='$git_repo/.git' rm --cached -f '$file'";
    runcmd "Removing file from git repo", "$git_cmd --git-dir='$git_repo/.git' update-index --force-remove '$file'";
  }


  # Commit
  my $has_changes = callcmd "Checking whether there are source tree changes",
                            "$git_cmd --git-dir='$git_repo/.git' diff-index --cached --no-renames --quiet HEAD -- ; echo ?\$";
  if ($has_changes) {
    print "Formaline: Committing source tree to git repository...\n";
    # Invent a user id if there is none, since newer versions of git insist on it
    runcmd "Setting user id for git repo", "$git_cmd --git-dir='$git_repo/.git' config user.name >/dev/null 2>&1 || $git_cmd --git-dir='$git_repo/.git' config user.name \"\${USER}\"";
    runcmd "Setting email address for git repo", "$git_cmd --git-dir='$git_repo/.git' config user.email >/dev/null 2>&1 || $git_cmd --git-dir='$git_repo/.git' config user.email \"\${USER}\@localhost\"";
    # make sure no background processes are started
    runcmd "Disabling background garbage collection", "$git_cmd --git-dir='$git_repo/.git' config gc.detachauto false";
    # Try to use the previous commit as parent, if possible
    runcmd "Commiting to git repo", "$git_cmd --git-dir='$git_repo/.git' commit -m '$build_id'";
    runcmd "Tagging git repo", "$git_cmd --git-dir='$git_repo/.git' tag '$build_id'";
    print "Formaline: Created git tag $build_id\n";
    runcmd "Updating branch in git repo", "$git_cmd --git-dir='$git_repo/.git' branch -f '$config_id'";
    print "Formaline: Updated git branch $config_id\n";
    gc_repo $git_repo;

    # Push
    init_repo "master", $git_master_repo;
    print "Formaline: Pushing to master git repository...\n";
    runcmd "Pushing to master git repository", "$git_cmd --git-dir='$git_repo/.git' push -v -f --all '$git_master_repo'";
    runcmd "Pushing tags to master git repository", "$git_cmd --git-dir='$git_repo/.git' push -v -f --tags '$git_master_repo'";
    gc_repo $git_master_repo;

    if ($git_local_repo ne '') {
      init_repo("local", $git_local_repo);
      print "Formaline: Pushing to local git repository $git_local_repo...\n";
      runcmd "Pushing to local git repository", "$git_cmd --git-dir='$git_master_repo/.git' push -v -f --all '$git_local_repo'";
      runcmd "Pushing tags to local git repository", "$git_cmd --git-dir='$git_master_repo/.git' push -v -f --tags '$git_local_repo'";
      gc_repo $git_local_repo;
      if ($git_central_repo ne '') {
        print "Formaline: Pushing to central repository $git_central_repo...\n";
        runcmd "Pushing to central git repository", "$git_cmd --git-dir='$git_local_repo/.git' push -v -f --all '$git_central_repo'";
        runcmd "Pushing tags to central git repository", "$git_cmd --git-dir='$git_local_repo/.git' push -v -f --tags '$git_central_repo'";
      }
    } else {
      if ($git_central_repo ne '') {
        print "Formaline: Pushing to central repository $git_central_repo...\n";
        runcmd "Pushing to central git repository", "$git_cmd --git-dir='$git_master_repo/.git' push -v -f --all '$git_central_repo'";
        runcmd "Pushing tags to central git repository", "$git_cmd --git-dir='$git_master_repo/.git' push -v -f --tags '$git_central_repo'";
      }
    }
  } else {
    print "Formaline: There are no source tree changes; we are done.\n";
  }
}

main()
