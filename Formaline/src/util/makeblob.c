/* $Header$ */

#include <stdio.h>

int
main (int argc, char * * argv)
{
  FILE * metafile;
  unsigned long metacount;
  char filename [100];
  FILE * file;
  unsigned long count;
  
  for (metacount = 0; ; ++ metacount)
  {
    sprintf (filename, "cactus-source-%08lu.c", metacount);
    file = fopen (filename, "w");
    fprintf (file, "/* This is an auto-generated file -- do not edit */\n");
    fprintf (file, "#include <stddef.h>\n");
    fprintf (file, "\n");
    fprintf (file, "char const cactus_source_%08lu [] = {", metacount);
    for (count = 0; count < 1000000; ++ count)
    {
      int const ch = getc (stdin);
      if (feof (stdin)) break;
      if (ferror (stdin)) return 1;
      if (count != 0) {
        fprintf (file, ",");
      }
      if (count % 16 == 0)
      {
        fprintf (file, "\n");
      }
      fprintf (file, "%3d", ch);
    }
    fprintf (file, "\n");
    fprintf (file, "};\n");
    fprintf (file, "size_t const cactus_source_length_%08lu = %lu;\n", metacount, count);
    fclose (file);
    
    if (feof (stdin)) break;
  }
  ++ metacount;
  
  
  
  metafile = fopen ("cactus-source.c", "w");
  fprintf (metafile, "/* This is an auto-generated file -- do not edit */\n");
  fprintf (metafile, "#include <stddef.h>\n");
  fprintf (metafile, "\n");
  fprintf (metafile, "struct chunkinfo\n");
  fprintf (metafile, "{\n");
  fprintf (metafile, "  char const * data;\n");
  fprintf (metafile, "  size_t * length;\n");
  fprintf (metafile, "};\n");
  fprintf (metafile, "\n");
  for (count = 0; count < metacount; ++ count)
  {
    fprintf (metafile, "extern char const cactus_source_%08lu [];\n", count);
    fprintf (metafile, "extern size_t cactus_source_length_%08lu;\n", count);
  }
  fprintf (metafile, "\n");
  fprintf (metafile, "struct chunkinfo cactus_source_chunks [] = {");
  for (count = 0; count < metacount; ++ count)
  {
    if (count != 0) {
      fprintf (metafile, ",");
    }
    fprintf (metafile, "\n");
    fprintf (metafile, "{ cactus_source_%08lu, & cactus_source_length_%08lu }", count, count);
  }
  fprintf (metafile, "\n");
  fprintf (metafile, "};\n");
  fprintf (metafile, "size_t cactus_source_chunks_length = %lu;\n", metacount);
  fclose (metafile);
  
  return 0;
}
