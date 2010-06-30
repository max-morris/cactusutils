#! /usr/bin/perl -w

use strict;

print <<EOF;
/* This is an auto-generated file -- do not edit */

\#include <stddef.h>
\#include <stdlib.h>

struct datainfo
{
  unsigned char const * data;
  size_t length;
  struct datainfo const * next;
};

struct sourceinfo
{
  struct datainfo const * first;
  char const * arrangement;
  char const * thorn;
};

EOF

foreach my $argv (@ARGV) {
    print "extern struct sourceinfo const cactus_source_${argv};\n";
}

print <<EOF;

struct sourceinfo const * const cactus_source [] = {
EOF

foreach my $argv (@ARGV) {
    print "  & cactus_source_${argv},\n";
}

print <<EOF;
  NULL
};
EOF
