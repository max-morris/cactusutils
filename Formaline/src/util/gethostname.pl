#! /usr/bin/perl -w

# Find the host name of this machine

# 2006-05-02 Erik Schnetter <schnetter@cct.lsu.edu>

# Search through the host name and all aliases.
# Use the first name that contains dots, indicating that this name is
# a long host name that includes a domain name.
# If there is no name that includes a domain name, use whatever the
# system calls "host name".

use strict;

# Get the system's idea of its host name
my $hostname = `hostname`;
chomp $hostname;

# Find its host name and all aliases
my ($name, $aliases) = gethostbyname ($hostname);

# Use the host name as fallback
my $goodname = $name ? $name : $hostname;

# Split the aliases
my @names = ();
push (@names, $name) if ($name);
push (@names, split (' ', $aliases)) if ($aliases);

# Search for a name that contains a dot
foreach my $maybename (@names)
{
    if ($maybename =~ /[.]/)
    {
        $goodname = $maybename;
        last;
    }
}

print "$goodname\n";
