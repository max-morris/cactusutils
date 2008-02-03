#! /usr/bin/perl -w

use strict;

my $items_per_line = 16;
my $items_per_file = 1024 * 1024;

$#ARGV == 1 or die;

my $arrangement = $ARGV[0];
my $thorn = $ARGV[1];
$thorn ne '' or die;

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

struct datainfo const cactus_data_0000_$thorn;
struct sourceinfo const cactus_source_$thorn =
{
  & cactus_data_0000_$thorn,
  \"$arrangement\",
  \"$thorn\"
};
EOF

my $done = 0;
for (my $fcount = 0; ! $done; ++ $fcount)
{
    printf "\n";
    printf "static unsigned char const data_%04d [] = {", $fcount;
    my $count;
    for ($count = 0; $count < $items_per_file; ++ $count)
    {
        my $ch = getc;
        if (! defined $ch)
        {
            $done = 1;
            last;
        }
        if ($count != 0)
        {
            printf ",";
        }
        if ($count % $items_per_line == 0)
        {
            printf "\n";
            printf "  ";
        }
        printf "%3d", ord $ch;
    }
    printf "\n";
    printf "};\n";
    printf "\n";
    if (! $done)
    {
        printf "struct datainfo const cactus_data_%04d_%s;\n", $fcount + 1, $thorn;
    }
    printf "struct datainfo const cactus_data_%04d_%s =\n", $fcount, $thorn;
    printf "{\n";
    printf "  data_%04d,\n", $fcount;
    printf "  %dUL,\n", $count;
    if (! $done)
    {
        printf "  & cactus_data_%04d_%s\n", $fcount + 1, $thorn;
    }
    else
    {
        printf "  NULL\n";
    }
    printf "};\n";
}
