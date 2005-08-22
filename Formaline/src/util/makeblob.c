/* $Header$ */

#include <assert.h>
#include <stdio.h>

int
main (int argc, char * * argv)
{
  char const * arrangement;
  char const * thorn;
  char const * output;
  char filename [10000];
  FILE * file;
  int fcount;
  int done;
  unsigned long count;
  unsigned long const items_per_line = 16;
  unsigned long const items_per_file = 1024 * 1024;
  
  assert (argc == 4);
  arrangement = argv[1];
  assert (arrangement);
  thorn = argv[2];
  assert (thorn);
  output = argv[3];
  assert (output);
  
#if 0
  snprintf (filename, sizeof filename, "%s.c", output);
  file = fopen (filename, "w");
  assert (file);
#endif
  file = stdout;
  
  fprintf (file, "/* This is an auto-generated file -- do not edit */\n");
  fprintf (file, "\n");
  fprintf (file, "#include <stddef.h>\n");
  fprintf (file, "\n");
  fprintf (file, "struct datainfo\n");
  fprintf (file, "{\n");
  fprintf (file, "  unsigned char const * data;\n");
  fprintf (file, "  size_t length;\n");
  fprintf (file, "  struct datainfo const * next;\n");
  fprintf (file, "};\n");
  fprintf (file, "\n");
  fprintf (file, "struct sourceinfo\n");
  fprintf (file, "{\n");
  fprintf (file, "  struct datainfo const * first;\n");
  fprintf (file, "  char const * arrangement;\n");
  fprintf (file, "  char const * thorn;\n");
  fprintf (file, "};\n");
  
  fprintf (file, "\n");
  fprintf (file, "struct datainfo const cactus_data_%04d_%s;\n", 0, thorn);
  fprintf (file, "struct sourceinfo const cactus_source_%s =\n", thorn);
  fprintf (file, "{\n");
  fprintf (file, "  & cactus_data_%04d_%s,\n", 0, thorn);
  fprintf (file, "  \"%s\",\n", arrangement);
  fprintf (file, "  \"%s\"\n", thorn);
  fprintf (file, "};\n");
  
  for (fcount = 0, done = 0; ! done; ++ fcount)
  {
#if 0
    snprintf (filename, sizeof filename, "%s-%04d.c", output, fcount);
    file = fopen (filename, "w");
    assert (file);
    
    fprintf (file, "/* This is an auto-generated file -- do not edit */\n");
    fprintf (file, "\n");
    fprintf (file, "#include <stddef.h>\n");
    fprintf (file, "\n");
    fprintf (file, "struct datainfo\n");
    fprintf (file, "{\n");
    fprintf (file, "  unsigned char const * data;\n");
    fprintf (file, "  size_t length;\n");
    fprintf (file, "  struct datainfo const * next;\n");
    fprintf (file, "};\n");
    fprintf (file, "\n");
    fprintf (file, "struct sourceinfo\n");
    fprintf (file, "{\n");
    fprintf (file, "  struct datainfo const * first;\n");
    fprintf (file, "  char const * arrangement;\n");
    fprintf (file, "  char const * thorn;\n");
    fprintf (file, "};\n");
#endif
    
    fprintf (file, "\n");
    fprintf (file, "static unsigned char const data_%04d [] = {", fcount);
    for (count = 0; count < items_per_file; ++ count)
    {
      int const ch = getc (stdin);
      if (feof (stdin))
      {
        done = 1;
        break;
      }
      if (ferror (stdin)) return 1;
      if (count != 0)
      {
        fprintf (file, ",");
      }
      if (count % items_per_line == 0)
      {
        fprintf (file, "\n");
      }
      fprintf (file, "%3d", ch);
    }
    fprintf (file, "\n");
    fprintf (file, "};\n");
    fprintf (file, "\n");
    if (! done)
    {
      fprintf (file, "struct datainfo const cactus_data_%04d_%s;\n",
               fcount + 1, thorn);
    }
    fprintf (file, "struct datainfo const cactus_data_%04d_%s =\n",
             fcount, thorn);
    fprintf (file, "{\n");
    fprintf (file, "  data_%04d,\n", fcount);
    fprintf (file, "  %luUL,\n", count);
    if (! done)
    {
      fprintf (file, "  & cactus_data_%04d_%s\n", fcount + 1, thorn);
    }
    else
    {
      fprintf (file, "  NULL\n");
    }
    fprintf (file, "};\n");
    
#if 0
    fclose (file);
#endif
  }
  
#if 0
  fclose (file);
#endif
  
  return 0;
}
