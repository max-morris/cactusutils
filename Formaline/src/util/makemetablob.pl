#! /usr/bin/perl -w

use strict;
  
print <<EOF;
/* This is an auto-generated file -- do not edit */

\#include <stddef.h>

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

for (my $count = 0; $count <= $#ARGV; ++ $count)
{
    printf "extern struct sourceinfo cactus_source_%s;\n", $ARGV[$count];
}
printf "\n";
printf "struct sourceinfo const * const cactus_source [] = {";
for (my $count = 0; $count <= $#ARGV; ++ $count)
{
    if ($count != 0)
    {
        printf ",";
    }
    printf "\n";
    printf "  & cactus_source_%s", $ARGV[$count];
}
printf "\n";
printf "};\n";
printf "size_t const cactus_source_length = %d;\n", $#ARGV + 1;
