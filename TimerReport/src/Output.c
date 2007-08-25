 /*@@
   @file      Output.c
   @date      July 6 2003
   @author    Gabrielle Allen
   @desc
              Functions to report the timers
   @enddesc
   @version   $Header$
 @@*/

#include <stdio.h>
#include <stdlib.h>

#include "cctk.h"
#include "cctk_Parameters.h"
#include "cctk_Arguments.h"

#include "util_String.h"

#include "cctk_Schedule.h"

static const char *rcsid = "$Header$";

CCTK_FILEVERSION(CactusUtils_TimerReport_Output_c);

/********************************************************************
 *********************     Local Data Types   ***********************
 ********************************************************************/

/********************************************************************
 ********************* Local Routine Prototypes *********************
 ********************************************************************/

static void PrintTimes (CCTK_ARGUMENTS);

/********************************************************************
 *********************  Scheduled Routine Prototypes  ***************
 ********************************************************************/

void TimerReport_Output(CCTK_ARGUMENTS);
void TimerReport_Checkpoint(CCTK_ARGUMENTS);

/********************************************************************
 ********************* Other Routine Prototypes *********************
 ********************************************************************/

/********************************************************************
 *********************     Local Data   *****************************
 ********************************************************************/

/********************************************************************
 ********************    External Routines   ************************
 ********************************************************************/

 /*@@
   @routine    TimerReport_Output
   @date       July 6 2003
   @author     Gabrielle Allen
   @desc
   Output the timer table
   @enddesc
@@*/
void TimerReport_Output(CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  if (next ||
      out_at == cctk_iteration ||
      (out_every && cctk_iteration%out_every == 0))
  {

    CCTK_VInfo(CCTK_THORNSTRING,
               "Timer Report at iteration %d time %g",
               cctk_iteration, (double)cctk_time);
    PrintTimes(CCTK_PASS_CTOC);

    if (next)
    {
      CCTK_ParameterSet("next", CCTK_THORNSTRING, "no");
    }

  }
}

 /*@@
   @routine    TimerReport_Checkpoint
   @date       April 10 2004
   @author     Erik Schnetter
   @desc
   Output the timer table if before_checkpoint is set
   @enddesc
@@*/
void TimerReport_Checkpoint(CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  if (before_checkpoint &&
      (checkpoint_every && cctk_iteration%checkpoint_every == 0))
  {

    CCTK_VInfo(CCTK_THORNSTRING,
               "Timer Report before checkpointing at iteration %d, time %g",
               cctk_iteration, (double)cctk_time);
    PrintTimes(CCTK_PASS_CTOC);

  }
}

/********************************************************************
 ********************    Internal Routines   ************************
 ********************************************************************/

static void PrintTimes (CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  int myproc;
  FILE *file;
  char *filename;
  const int filename_length = 10000;
  const char *flags;
  static int first_time = 1;

  if (CCTK_EQUALS (out_filename, ""))
  {
    /* Print to stdout.  */
    CCTK_SchedulePrintTimes(NULL);
  }
  else
  {
    /* Print to a file.  */
    myproc = CCTK_MyProc(cctkGH);
    filename = malloc(filename_length);
    Util_snprintf(filename, filename_length,
                  "%s/%s.%04d.txt", out_dir, out_filename, myproc);

    /* truncate or append */
    flags = first_time && IO_TruncateOutputFiles(cctkGH) ? "w" : "a";
    first_time = 0;

    file = fopen(filename, flags);
    if (file)
    {
      /* Print the schedule to the file */
      fprintf(file, "Timer Report at iteration %d time %g:\n\n",
              cctk_iteration, (double) cctk_time);
      CCTK_SchedulePrintTimesToFile(NULL, file);
      fprintf(file, "\n********************************************************************************\n");
      fclose(file);
    }
    else
    {
      CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "Could not open timer report output file \"%s\"", filename);
    }
    free(filename);
  }
}
