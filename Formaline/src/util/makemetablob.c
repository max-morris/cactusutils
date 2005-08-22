/* $Header$ */

#include <stdio.h>

int
main (int argc, char * * argv)
{
  int count;
  
  printf ("/* This is an auto-generated file -- do not edit */\n");
  printf ("\n");
  printf ("#include <stddef.h>\n");
  printf ("\n");
  printf ("struct datainfo\n");
  printf ("{\n");
  printf ("  unsigned char const * data;\n");
  printf ("  size_t length;\n");
  printf ("  struct datainfo const * next;\n");
  printf ("};\n");
  printf ("\n");
  printf ("struct sourceinfo\n");
  printf ("{\n");
  printf ("  struct datainfo const * first;\n");
  printf ("  char const * arrangement;\n");
  printf ("  char const * thorn;\n");
  printf ("};\n");
  printf ("\n");
  for (count = 1; count < argc; ++ count)
  {
    printf ("extern struct sourceinfo cactus_source_%s;\n", argv[count]);
  }
  printf ("\n");
  printf ("struct sourceinfo const * const cactus_source [] = {");
  for (count = 1; count < argc; ++ count)
  {
    if (count != 1)
    {
      printf (",");
    }
    printf ("\n");
    printf ("  & cactus_source_%s", argv[count]);
  }
  printf ("\n");
  printf ("};\n");
  printf ("size_t const cactus_source_length = %d;\n", argc - 1);
  
  return 0;
}
