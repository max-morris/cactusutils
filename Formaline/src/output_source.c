/* $Header$ */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"



struct chunkinfo
{
  char const * data;
  size_t const * length;
};

extern struct chunkinfo cactus_source_chunks [];
extern size_t cactus_source_chunks_length;



void
Formaline_OutputSource (CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;
  
  char const sourcename [] = "cactus-source.tar.gz";
  size_t filenamelength;
  char * filename;
  FILE * file;
  size_t chunk;
  
  if (CCTK_MyProc (cctkGH) != 0) return;
  
  filenamelength = strlen (out_dir) + strlen (sourcename) + 2;
  filename = malloc (filenamelength);
  assert (filename);
  sprintf (filename, "%s/%s", out_dir, sourcename);
  
  CCTK_VInfo (CCTK_THORNSTRING,
              "Writing tarball with Cactus sources to file \"%s\"", filename);
  
  file = fopen (filename, "w");
  assert (file);
  for (chunk = 0; chunk < cactus_source_chunks_length; ++ chunk)
  {
    fwrite (cactus_source_chunks[chunk].data,
            sizeof * cactus_source_chunks[chunk].data,
            * cactus_source_chunks[chunk].length,
            file);
  }
  fclose (file);
  
  free (filename);
}
