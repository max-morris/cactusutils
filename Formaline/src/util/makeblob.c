/* $Header$ */

#include <assert.h>
#include <stdio.h>

int
main (int argc, char * * argv)
{
  char const * arrangement;
  char const * thorn;
  unsigned long count;
  
  assert (argc == 3);
  arrangement = argv[1];
  thorn = argv[2];
  assert (arrangement);
  assert (thorn);
  
  printf ("/* This is an auto-generated file -- do not edit */\n");
  printf ("\n");
  printf ("#include <stddef.h>\n");
  printf ("\n");
  printf ("struct sourceinfo\n");
  printf ("{\n");
  printf ("  char const * data;\n");
  printf ("  size_t length;\n");
  printf ("  char const * arrangement;\n");
  printf ("  char const * thorn;\n");
  printf ("};\n");
  printf ("\n");
  printf ("static char const data [] = {");
  for (count = 0; ; ++ count)
  {
    int const ch = getc (stdin);
    if (feof (stdin)) break;
    if (ferror (stdin)) return 1;
    if (count != 0) {
      printf (",");
    }
    if (count % 16 == 0)
    {
      printf ("\n");
    }
    printf ("%3d", ch);
  }
  printf ("\n");
  printf ("};\n");
  printf ("\n");
  printf ("struct sourceinfo const cactus_source_%s =\n", thorn);
  printf ("{\n");
  printf ("  data,\n");
  printf ("  %lu,\n", count);
  printf ("  \"%s\",\n", arrangement);
  printf ("  \"%s\"\n", thorn);
  printf ("};\n");
  
  return 0;
}
