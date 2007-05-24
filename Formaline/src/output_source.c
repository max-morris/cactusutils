/* $Header$ */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"



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

extern struct sourceinfo const * const cactus_source [];
extern size_t const cactus_source_length;



void
Formaline_OutputSource (CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  char filename [10000];
  FILE * file;
  size_t count;
  struct datainfo const * datainfo;

  if (CCTK_MyProc (cctkGH) != 0) return;

  { CCTK_PRINTSEPARATOR }
  CCTK_VInfo (CCTK_THORNSTRING,
              "Writing tarballs with the Cactus sources into the directory \"%s/%s\"",
              out_dir, output_source_subdirectory);

  snprintf (filename, sizeof filename,
            "%s/%s", out_dir, output_source_subdirectory);
  CCTK_CreateDirectory (0755, filename);

  /* Output all thorns' tarballs */
  for (count = 0; count < cactus_source_length; ++ count)
  {
    snprintf (filename, sizeof filename,
              "%s/%s/Cactus-source-%s.tar.gz",
              out_dir, output_source_subdirectory,
              cactus_source[count]->thorn);
    file = fopen (filename, "w");
    if (file == NULL)
    {
      CCTK_VWarn (0, __LINE__, __FILE__, CCTK_THORNSTRING,
                  "Failed to open source file \"%s\" for writing", filename);
    }
    for (datainfo = cactus_source[count]->first;
         datainfo;
         datainfo = datainfo->next)
    {
      fwrite (datainfo->data, sizeof * datainfo->data, datainfo->length, file);
    }
    fclose (file);
  }

  /* Add a README */
  snprintf (filename, sizeof filename,
            "%s/%s/README", out_dir, output_source_subdirectory);
  file = fopen (filename, "w");
  if (file == NULL)
  {
    CCTK_VWarn (0, __LINE__, __FILE__, CCTK_THORNSTRING,
                "Failed to open README file \"%s\" for writing", filename);
  }
  fprintf (file,
"README for the Cactus source tree\n"
"\n"
"This directory contains a complete Cactus source tree in several tarballs.\n"
"(A tarball is a file with a suffix like \".tar.gz\".)\n"
"The tarballs were created by the thorn AEIThorns/Formaline when the\n"
"corresponding executable was produced, and were stored in the executable.\n"
"Thorn AEIThorns/Formaline contains more information about this feature.\n"
"\n"
"In order to fully recreate the Cactus source tree, unpack all tarballs\n"
"in this directory, e.g. with commands like\n"
"\ttar xzf cactus-source-Cactus.tar.gz\n"
"for all tarballs.  All tarballs should be unpacked into the same directory.\n"
"\n"
"The files \"config-info\" and \"ThornList\" that were used to build the\n"
"executable can then be found in the \"configs\" subdirectory.\n"
           );
  fclose (file);
}
