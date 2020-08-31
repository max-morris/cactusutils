#! /usr/bin/perl -w

use diagnostics;
use warnings;
use strict;

my $items_per_line = 16;
my $items_per_file = 128 * 1024;

$#ARGV == 2 or die "ARGV=@ARGV";

my $basename = $ARGV[0];
my $arrangement = $ARGV[1];
my $thorn = $ARGV[2];
$thorn ne '' or die;



# Write several files, each with a maximum length

my $done = 0;
for (my $count = 0; ! $done; ++ $count) {
    my $fcount      = sprintf "%04d", $count;
    my $fcount_next = sprintf "%04d", $count + 1;
    
    open FILE, "> $basename-$fcount.c" or die "Could not open $basename-$fcount.c: $!";
    print FILE <<EOF;
/* This is an auto-generated file -- do not edit */

\#include <stddef.h>
\#include <stdlib.h>

struct datainfo
{
  unsigned char const * data;
  size_t length;
  struct datainfo const * next;
};

EOF
    
    print FILE "static unsigned char const data_${fcount} [] = {";
    my $bytes;
    for ($bytes = 0; $bytes < $items_per_file; ++ $bytes) {
        my $ch = getc;
        if (! defined $ch) {
            $done = 1;
            last;
        }
        if ($bytes != 0) {
            printf FILE ",";
        }
        if ($bytes % $items_per_line == 0) {
            printf FILE "\n";
            printf FILE "  ";
        }
        printf FILE "%3d", ord $ch;
    }
    printf FILE "\n";
    printf FILE "};\n";
    
    print FILE "\n";
    if (! $done) {
        print FILE "extern struct datainfo const cactus_data_${fcount_next}_${thorn};\n";
    }
    print FILE "struct datainfo const cactus_data_${fcount}_${thorn} =\n";
    print FILE "{\n";
    print FILE "  data_${fcount},\n";
    print FILE "  ${bytes}UL,\n";
    if (! $done) {
        print FILE "  & cactus_data_${fcount_next}_${thorn}\n";
    }
    else {
        print FILE "  NULL\n";
    }
    print FILE "};\n";
    
    close FILE;
}



# Write meta-file

{
    open FILE, "> $basename.c" or die "Could not open $basename.c: $!";
    printf FILE <<EOF;
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

extern struct datainfo const cactus_data_0000_$thorn;
struct sourceinfo const cactus_source_$thorn =
{
  & cactus_data_0000_$thorn,
  \"$arrangement\",
  \"$thorn\"
};
EOF
    close FILE;
}
