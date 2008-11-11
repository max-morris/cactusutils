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
#include <assert.h>

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
static void OutputAllTimers (CCTK_ARGUMENTS);
static void PrintTopTimers (CCTK_ARGUMENTS);

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

    if (output_all_timers)
      OutputAllTimers(CCTK_PASS_CTOC);

    if (n_top_timers > 0)
      PrintTopTimers(CCTK_PASS_CTOC);

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

static void OutputAllTimers (CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  FILE *file;
  char *filename;
  const char *flags;
  static int first_time = 1;
  int ntimers = CCTK_NumTimers();
  cTimerData  *td = CCTK_TimerCreateData();
  double timer_secs = -1;
  static int last_ntimers = -1;

  assert(ntimers >= 0);

  Util_asprintf(&filename, "%s/%s.%04d.txt", 
                out_dir, "AllTimers", CCTK_MyProc(cctkGH));

  /* truncate or append */
  flags = first_time && IO_TruncateOutputFiles(cctkGH) ? "w" : "a";

  file = fopen(filename, flags);
  if (file)
  {
    if (first_time)
    {
      fprintf(file, "# Clock %s\n", all_timers_clock);
      fprintf(file, "# Unit seconds\n");
    }
    /* If the number of timers has changed, output the header
     * containing the timer names again.  It would be better to track
     * creation and deletion of timers in the flesh, but this always
     * works as long as timers are never deleted. */
    if (first_time || last_ntimers != ntimers)
    {
      fprintf(file, "# Column 1 iteration\n");
      fprintf(file, "# Column 2 time\n");
      for (int i = 0; i < ntimers; i++)
      {
        const char *name = CCTK_TimerName(i);
          
        if (name == NULL)
          name = "";

        fprintf(file, "# Column %d %s\n", i+3, name);
      }
    }

    fprintf(file, "%d\t%f\t", cctk_iteration, cctk_time);

    for (int i = 0; i < ntimers; i++)
    {
      CCTK_TimerI(i, td);
        
      {
        const cTimerVal  *tv = CCTK_GetClockValue(all_timers_clock, td);

        if (tv != NULL)
        {
          timer_secs = CCTK_TimerClockSeconds(tv);
        }
        else
        {
          CCTK_VWarn(0, __LINE__, __FILE__, CCTK_THORNSTRING,
                     "Clock \"%s\" not found", all_timers_clock);
        }
      }
        
      fprintf(file, "%.15g", timer_secs);
      if (i != ntimers - 1)
        fprintf(file, "\t");
    }
    fprintf(file, "\n");
    fclose(file);
  }
  else
  {
    CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
               "Could not open timer report output file \"%s\"", filename);
  }
  free(filename);
  CCTK_TimerDestroyData(td);
  first_time = 0;
  last_ntimers = ntimers;
  return;
}

typedef struct
{
  int timer;
  double t;
} timer_pair;

static int compare(const void * a, const void * b)
{
  timer_pair *t1 = (timer_pair *) a;
  timer_pair *t2 = (timer_pair *) b;

  double d = t2->t - t1->t;

  if (d > 0)
    return 1;
  else if (d == 0)
    return 0;
  else return -1;
}


static void PrintTopTimers (CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  int ntimers = CCTK_NumTimers();
  cTimerData  *td = CCTK_TimerCreateData();
  double timer_secs = -1;
  timer_pair   timers[ntimers];
  int topn = n_top_timers;
  double total = 0;

  char *sep = "======================================================================";

  assert(ntimers >= 0);
  for (int i = 0; i < ntimers; i++)
  {
    CCTK_TimerI(i, td);
        
    {
      const cTimerVal  *tv = CCTK_GetClockValue(all_timers_clock, td);

      if (tv != NULL)
      {
        timer_secs = CCTK_TimerClockSeconds(tv);
      }
      else
      {
        CCTK_VWarn(0, __LINE__, __FILE__, CCTK_THORNSTRING,
                   "Clock \"%s\" not found", all_timers_clock);
      }
    }

    timers[i].timer = i;
    timers[i].t = timer_secs;
  }

  qsort(timers, ntimers, sizeof(timer_pair), compare);

  CCTK_TimerDestroyData(td);

  CCTK_VInfo(CCTK_THORNSTRING,
             "Top timers at iteration %d time %g",
             cctk_iteration, (double)cctk_time);

  printf("%s\n", sep);
  printf("%%      Time/s   Timer (%s)\n", all_timers_clock);
  printf("%s\n", sep);

  /* This should be the "CCTK total time" timer */
  total = timers[0].t;

  for (int i = 0; i < (ntimers >= topn ? topn : ntimers) ; i++)
  {
    double percent = 100.0 * timers[i].t / total;

    char percent_string[10];
    char time_string[10];

    snprintf(percent_string, 6, "%.5f", percent);
    snprintf(time_string, 8, "%.7f", timers[i].t);

    printf("%s  %s  %s\n", percent_string, time_string, 
           CCTK_TimerName(timers[i].timer));
  }
  printf("%s\n", sep);

  return;
}
