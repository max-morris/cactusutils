#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef CCTK_PTHREADS
#include <pthread.h>
#endif

// Choose whether to output tarballs in the background

// Don't use fork, MPI may not like it
// #ifdef HAVE_UNISTD_H
// #  define USE_FORK
// #endif

#ifdef HAVE_CAPABILITY_PTHREADS
#define USE_PTHREADS
#endif

struct datainfo {
  unsigned char const *data;
  size_t length;
  struct datainfo const *next;
};

struct sourceinfo {
  struct datainfo const *first;
  char const *arrangement;
  char const *thorn;
};

extern struct sourceinfo const *const cactus_source[];
extern size_t const cactus_source_length;

static void do_output(cGH const *restrict const cctkGH) {
  DECLARE_CCTK_PARAMETERS;

  size_t myproc, nprocs;
  myproc = CCTK_MyProc(cctkGH);
  nprocs = CCTK_nProcs(cctkGH);
  size_t const nioprocs = nprocs < 10 ? nprocs : 10;

  { CCTK_PRINTSEPARATOR }
  CCTK_VInfo(
      CCTK_THORNSTRING,
      "Writing tarballs with the Cactus sources into the directory \"%s/%s\"",
      out_dir, output_source_subdirectory);

  char dirname[10000];
  snprintf(dirname, sizeof dirname, "%s/%s", out_dir,
           output_source_subdirectory);
  CCTK_CreateDirectory(0755, dirname);

  /* Output all thorns' tarballs */
  for (size_t count = 0; cactus_source[count]; ++count) {
    if (count % nioprocs != myproc)
      continue;

    char filename[10000];
    snprintf(filename, sizeof filename, "%s/%s/Cactus-source-%s.tar.gz",
             out_dir, output_source_subdirectory, cactus_source[count]->thorn);
    FILE *const file = fopen(filename, "w");
    if (!file) {
      CCTK_VWarn(CCTK_WARN_ABORT, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "Failed to open source file \"%s\" for writing", filename);
    }
    for (struct datainfo const *datainfo = cactus_source[count]->first;
         datainfo; datainfo = datainfo->next) {
      fwrite(datainfo->data, sizeof *datainfo->data, datainfo->length, file);
    }
    fclose(file);
  }

  /* Add a README */
  if (myproc == nprocs - 1) {
    char readmename[10000];
    snprintf(readmename, sizeof readmename, "%s/%s/README", out_dir,
             output_source_subdirectory);
    FILE *const readme = fopen(readmename, "w");
    if (!readme) {
      CCTK_VWarn(CCTK_WARN_ABORT, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "Failed to open README file \"%s\" for writing", readmename);
    }
    fprintf(
        readme,
        "README for the Cactus source tree\n"
        "\n"
        "This directory contains a complete Cactus source tree in several "
        "tarballs.\n"
        "(A tarball is a file with a suffix like \".tar.gz\".)\n"
        "The tarballs were created by the thorn AEIThorns/Formaline when the\n"
        "corresponding executable was produced, and were stored in the "
        "executable.\n"
        "Thorn CactusUtils/Formaline contains more information about this "
        "feature.\n"
        "\n"
        "In order to fully recreate the Cactus source tree, unpack all "
        "tarballs\n"
        "in this directory, e.g. with commands like\n"
        "\ttar xzf Cactus-source-Cactus.tar.gz\n"
        "for all tarballs, or\n"
        "\tfor T in *.tar.gz; do tar xzf $T; done\n"
        "in Bash. All tarballs should be unpacked into the same directory.\n"
        "\n"
        "The files \"config-info\" and \"ThornList\" that were used to build "
        "the\n"
        "executable can then be found in the \"configs\" subdirectory.\n");
    fclose(readme);
  }
}

#if defined(USE_PTHREADS)
static void *start_routine(void *const arg) {
  do_output(arg);
  return NULL;
}
#endif

void Formaline_OutputSource(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

#if defined(USE_FORK)

  pid_t const pid = fork();
  /* Return the parent, continue the child. If there was an error, we
     also continue. */
  if (pid > 0)
    return;

  do_output(cctkGH);

  if (pid == 0) {
    /* Exit the child. We exit secretly, so that the parent's files
       and MPI communicators are not affected. */
    _exit(0);
  }

#elif defined(USE_PTHREADS)

  pthread_t thread;
  int const ierr = pthread_create(&thread, NULL, start_routine, cctkGH);
  if (ierr) {
    /* There was an error; output the sources without using pthreads */
    do_output(cctkGH);
  }

#else

  do_output(cctkGH);

#endif
}
