#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"
#include "cctk_Termination.h"
#include "cctk_Timers.h"
#include "util_String.h"

#include "terminationtrigger.h"

enum { BUFLEN = 10000 };

/* Write the termination file's path into the caller's buffer, which
   holds len characters. The caller owns the buffer, so that this is
   reentrant. */
static void get_termination_file(char *filename, size_t len) {
  DECLARE_CCTK_PARAMETERS;

  if (strlen(termination_file) == 0) {
    const char *pbs_jobid = getenv("MANUAL_TERMINATION_JOB_ID");
    if (pbs_jobid == NULL)
      pbs_jobid = getenv("PBS_JOBID");

    if (pbs_jobid == NULL)
      CCTK_VWarn(CCTK_WARN_ABORT, __LINE__, __FILE__, "ManualTermination",
                 "Could not find environment variable "
                 "'MANUAL_TERMINATION_JOB_ID' or 'PBS_JOBID'");
    else
      Util_snprintf(filename, len, "/tmp/cactus_terminate.%s", pbs_jobid);
  } else {
    if (termination_file[0] == '/') {
      /* file name begins with a slash, do not use IO::out_dir */
      Util_snprintf(filename, len, "%s", termination_file);
    } else {
      /* add IO::out_dir to filename */
      Util_snprintf(filename, len, "%s/%s", out_dir, termination_file);
    }
  }
}

/* Note that the termination file is created even if
   termination_from_file is false. This is because
   termination_from_file is steerable and may be changed to true at
   run time. */
void TerminationTrigger_CreateFile(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  FILE *file;
  char filename[BUFLEN];

  /* only one processor needs to create the file */
  if (CCTK_MyProc(cctkGH) != 0) {
    return;
  }

  get_termination_file(filename, sizeof filename);
  file = fopen(filename, "w");
  if (!file) {
    CCTK_VWarn(CCTK_WARN_ABORT, __LINE__, __FILE__, CCTK_THORNSTRING,
               "Could not create termination file \'%s\'", filename);
  }

  fclose(file);
}

void TerminationTrigger_CheckFile(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  FILE *file;
  char filename[BUFLEN];
  int terminate;
  int num_read;

  if (!termination_from_file) {
    return;
  }

  /* only one processor needs to check the file */
  if (CCTK_MyProc(cctkGH) != 0) {
    return;
  }

  if (cctk_iteration % check_file_every != 0) {
    return;
  }

  get_termination_file(filename, sizeof filename);

  if (testsuite && cctk_iteration == 1) {
    file = fopen(filename, "w");
    if (!file) {
      CCTK_VWarn(CCTK_WARN_ABORT, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "Could not create termination file \'%s\'", filename);
    }
    fprintf(file, "1\n");

    fclose(file);
  }

  file = fopen(filename, "r");

  if (file != NULL) {
    num_read = fscanf(file, "%d", &terminate);
    fclose(file);

    if (num_read == 1 && terminate == 1) {
      CCTK_INFO("Found termination signal in termination file.  "
                "Triggering termination...");

      TerminationTrigger_TriggerTermination(CCTK_PASS_CTOC);
    }
  }
}
