 /*@@
   @file      Output.c
   @date      July 6 2003
   @author    Gabrielle Allen
   @desc
              Functions to report the timers
   @enddesc
   @version   $Header$
 @@*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cctk.h"
#include "cctk_Parameters.h"
#include "cctk_Arguments.h"

#include "util_String.h"

#include "cctk_Schedule.h"

static const char * const rcsid = "$Header$";

CCTK_FILEVERSION(CactusUtils_TimerReport_Output_c);

/********************************************************************
 *********************     Local Data Types   ***********************
 ********************************************************************/

struct timer_stats {
  CCTK_REAL * restrict secs_local; /* process-local values */
  CCTK_REAL * restrict secs_avg;   /* global average */
  CCTK_REAL * restrict secs_min;   /* global minimum */
  CCTK_REAL * restrict secs_max;   /* global maximum */
  int ntimers;
};

/********************************************************************
 ********************* Local Routine Prototypes *********************
 ********************************************************************/

static void Output (CCTK_ARGUMENTS);
static void PrintTimes (CCTK_ARGUMENTS);
static void OutputAllTimers (CCTK_ARGUMENTS);
static void OutputAllTimersTogether (CCTK_ARGUMENTS);
static void OutputAllTimersReadable (CCTK_ARGUMENTS);
static void PrintTopTimers (CCTK_ARGUMENTS);

static int CollectTimerInfo (cGH const * restrict const cctkGH,
                             struct timer_stats * restrict const timers);

static char *QuoteForCSV (const char*);
static char *QuoteForTSV (const char*);

/********************************************************************
 *********************  Scheduled Routine Prototypes  ***************
 ********************************************************************/

void TimerReport_OutputEvery(CCTK_ARGUMENTS);
void TimerReport_OutputTerminate(CCTK_ARGUMENTS);
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
   @routine    TimerReport_OutputEvery
   @date       2008-11-12
   @author     Erik Schnetter
   @desc
   Output the timer table periodically
   @enddesc
@@*/
void TimerReport_OutputEvery(CCTK_ARGUMENTS)
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
    Output(CCTK_PASS_CTOC);

    if (next)
    {
      CCTK_ParameterSet("next", CCTK_THORNSTRING, "no");
    }
  }
}



 /*@@
   @routine    TimerReport_OutputTerminate
   @date       2008-11-12
   @author     Erik Schnetter
   @desc
   Output the timer table periodically
   @enddesc
@@*/
void TimerReport_OutputTerminate(CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  CCTK_VInfo(CCTK_THORNSTRING,
             "Timer Report before terminating at iteration %d time %g",
             cctk_iteration, (double)cctk_time);
  Output(CCTK_PASS_CTOC);
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
    Output(CCTK_PASS_CTOC);
  }
}



/********************************************************************
 ********************    Internal Routines   ************************
 ********************************************************************/



static void Output (CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  PrintTimes(CCTK_PASS_CTOC);

  if (output_all_timers)
  {
    OutputAllTimers(CCTK_PASS_CTOC);
  }

  if (output_all_timers_together)
  {
    OutputAllTimersTogether(CCTK_PASS_CTOC);
  }

  if (output_all_timers_readable)
  {
    OutputAllTimersReadable(CCTK_PASS_CTOC);
  }

  if (n_top_timers > 0)
  {
    PrintTopTimers(CCTK_PASS_CTOC);
  }
}



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
                  "%s/%s.%06d.txt", out_dir, out_filename, myproc);

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
  int i;

  assert(ntimers >= 0);

  Util_asprintf(&filename, "%s/%s.%06d.txt", 
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
      fprintf(file, "# Column 2 simulation time\n");
      for (i = 0; i < ntimers; i++)
      {
        const char *name = CCTK_TimerName(i);
          
        if (name == NULL)
          name = "";

        fprintf(file, "# Column %d %s\n", i+3, name);
      }
    }

    fprintf(file, "%d\t%.15g", cctk_iteration, (double)cctk_time);

    for (i = 0; i < ntimers; i++)
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
          CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
                     "Clock \"%s\" not found for timer #%d \"%s\"",
                     all_timers_clock, i, CCTK_TimerName(i));
          timer_secs = -1;
        }
      }
        
      fprintf(file, "\t%.15g", timer_secs);
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



static void OutputAllTimersTogether (CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;
  
  struct timer_stats timers;
  if (!CollectTimerInfo (cctkGH, &timers))
    return;
  
  if (CCTK_MyProc(cctkGH) == 0)
  {
    static int first_time = 1;
    static int last_ntimers = -1;
    
    char *filename, *filename_csv, *filename_tsv;
    Util_asprintf(&filename, "%s/%s.txt", out_dir, "AllTimers");
    Util_asprintf(&filename_csv, "%s/%s.csv", out_dir, "AllTimers");
    Util_asprintf(&filename_tsv, "%s/%s.tsv", out_dir, "AllTimers");
    
    /* truncate or append */
    const char * const flags =
      first_time && IO_TruncateOutputFiles(cctkGH) ? "w" : "a";
    
    FILE * const file = fopen(filename, flags);
    FILE * const file_csv = fopen(filename_csv, flags);
    FILE * const file_tsv = fopen(filename_tsv, flags);
    if (file)
    {
      if (first_time)
      {
        fprintf(file, "# Clock %s\n", all_timers_clock);
        fprintf(file, "# Unit seconds\n");
        char *const all_timers_clock_csv = QuoteForCSV(all_timers_clock);
        fprintf(file_csv, "\"Clock %s\",", all_timers_clock_csv);
        fprintf(file_csv, "\"Unit seconds\"\n");
        free (all_timers_clock_csv);
        char *const all_timers_clock_tsv = QuoteForTSV(all_timers_clock);
        fprintf(file_tsv, "Clock %s\t", all_timers_clock_tsv);
        fprintf(file_tsv, "Unit seconds\n");
        free (all_timers_clock_tsv);
      }
      /* If the number of timers has changed, output the header
       * containing the timer names again.  It would be better to
       * track creation and deletion of timers in the flesh, but this
       * method here works as long as timers are never deleted.  */
      if (last_ntimers != timers.ntimers)
      {
        fprintf(file, "# Column 1 iteration\n");
        fprintf(file, "# Column 2 simulation time\n");
        fprintf(file, "# For all following columns:\n");
        fprintf(file, "#    Column 3n   average of all processes\n");
        fprintf(file, "#    Column 3n+1 minimum of all processes\n");
        fprintf(file, "#    Column 3n+2 maximum of all processes\n");
        fprintf(file_csv, "\"iteration\",");
        fprintf(file_csv, "\"simulation time\"");
        fprintf(file_tsv, "iteration\t");
        fprintf(file_tsv, "simulation time");
        for (int i = 0; i < timers.ntimers; i++)
        {
          const char * const name = CCTK_TimerName(i);
          fprintf(file, "# Column %d %s\n", 3*i+3, name ? name : "");
          char *const name_csv = QuoteForCSV(name ? name : "");
          fprintf(file_csv,
                  ",\"%s (average)\",\"%s (minimum)\",\"%s (maximum)\"",
                  name_csv, name_csv, name_csv);
          free (name_csv);
          char *const name_tsv = QuoteForTSV(name ? name : "");
          fprintf(file_tsv,
                  "\t%s (average)\t%s (minimum)\t%s (maximum)",
                  name_tsv, name_tsv, name_tsv);
          free (name_tsv);
        }
        fprintf(file_csv, "\n");
        fprintf(file_tsv, "\n");
      }
      
      fprintf(file, "%d\t%.15g", cctk_iteration, (double)cctk_time);
      fprintf(file_csv, "%d,%.15g", cctk_iteration, (double)cctk_time);
      fprintf(file_tsv, "%d\t%.15g", cctk_iteration, (double)cctk_time);
      
      for (int i = 0; i < timers.ntimers; i++)
      {
        fprintf(file, "\t%.15g %.15g %.15g",
                (double)timers.secs_avg[i],
                (double)timers.secs_min[i], (double)timers.secs_max[i]);
        fprintf(file_csv, ",%.15g,%.15g,%.15g",
                (double)timers.secs_avg[i],
                (double)timers.secs_min[i], (double)timers.secs_max[i]);
        fprintf(file_tsv, "\t%.15g\t%.15g\t%.15g",
                (double)timers.secs_avg[i],
                (double)timers.secs_min[i], (double)timers.secs_max[i]);
      }
      fprintf(file, "\n");
      fprintf(file_csv, "\n");
      fprintf(file_tsv, "\n");
      fclose(file);
      fclose(file_csv);
      fclose(file_tsv);
    }
    else
    {
      CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "Could not open timer report output files \"%s\" and  \"%s\"",
                 filename, filename_csv);
    }
    free(filename);
    free(filename_csv);
    
    first_time = 0;
    last_ntimers = timers.ntimers;
    
  } /* if root processor */
  
  free (timers.secs_local);
  free (timers.secs_avg  );
  free (timers.secs_min  );
  free (timers.secs_max  );
}



static void OutputAllTimersReadable (CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;
  
  struct timer_stats timers;
  if (!CollectTimerInfo (cctkGH, &timers))
    return;
  
  if (CCTK_MyProc(cctkGH) == 0)
  {
    static int first_time = 1;
    static int last_ntimers = -1;
    
    char *filename;
    Util_asprintf(&filename, "%s/%s.txt", out_dir, "AllTimersReadable");
    
    /* truncate or append */
    const char * const flags =
      first_time && IO_TruncateOutputFiles(cctkGH) ? "w" : "a";
    
    FILE * const file = fopen(filename, flags);
    if (file)
    {
      if (first_time)
      {
        fprintf(file, "# Clock %s\n", all_timers_clock);
        fprintf(file, "# Unit seconds\n");
      }
      /* If the number of timers has changed, output the header
       * containing the timer names again.  It would be better to
       * track creation and deletion of timers in the flesh, but this
       * method here works as long as timers are never deleted.  */
      if (last_ntimers != timers.ntimers)
      {
        fprintf(file, "# Column 1 iteration\n");
        fprintf(file, "# Column 2 simulation time\n");
        fprintf(file, "# Column 3 timer number\n");
        fprintf(file, "# Column 4,5,6 average, minimum, maximum over all processors\n");
        fprintf(file, "# Column 7+ timer name\n");
      }
      for (int i = 0; i < timers.ntimers; i++)
      {
        const char * const name = CCTK_TimerName(i);
        fprintf(file, "%d %.15g\t%d\t%.15g %.15g %.15g\t%s\n",
                cctk_iteration, (double)cctk_time,
                i,
                (double)timers.secs_avg[i],
                (double)timers.secs_min[i], (double)timers.secs_max[i],
                name ? name : "");
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
    
    first_time = 0;
    last_ntimers = timers.ntimers;
    
  } /* if root processor */
  
  free (timers.secs_local);
  free (timers.secs_avg  );
  free (timers.secs_min  );
  free (timers.secs_max  );
  
  return;
}



typedef struct
{
  int timer;
  double t;
} timer_pair;

static int compare(const void * a, const void * b)
{
  const timer_pair *t1 = (const timer_pair *) a;
  const timer_pair *t2 = (const timer_pair *) b;

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
  int i;

  const char *sep = "======================================================================";

  assert(ntimers >= 0);
  for (i = 0; i < ntimers; i++)
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

  for (i = 0; i < (ntimers >= topn ? topn : ntimers) ; i++)
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

static int integer_same_on_all_procs(cGH const * restrict const cctkGH, const CCTK_INT i)
{
  /* There is no "equals" reduction operator, so we check that
   * minimum and maximum are the same */
  const int reduce_min = CCTK_ReductionArrayHandle ("minimum");
  const int reduce_max = CCTK_ReductionArrayHandle ("maximum");
  CCTK_INT min_i = 0;
  CCTK_INT max_i = 0;

  if (CCTK_ReduceLocScalar(cctkGH, -1 /* All processors */, reduce_min,
                           &i, &min_i, CCTK_VARIABLE_INT)) 
    CCTK_WARN (CCTK_WARN_ABORT, "Error in calling min reduction operator");

  if (CCTK_ReduceLocScalar(cctkGH, -1 /* All processors */, reduce_max,
                           &i, &max_i, CCTK_VARIABLE_INT))
    CCTK_WARN (CCTK_WARN_ABORT, "Error in calling max reduction operator");
  return min_i == max_i;
}


/* Collect timer information onto the root processor */
static int CollectTimerInfo (cGH const * restrict const cctkGH,
                             struct timer_stats * restrict const timers)
{
  DECLARE_CCTK_PARAMETERS;
  
  assert (timers);
  
  timers->ntimers = CCTK_NumTimers();
  assert (timers->ntimers >= 0);

  /* Check that the number of timers is consistent across processors */
  if (!integer_same_on_all_procs(cctkGH, timers->ntimers))
  {
    CCTK_VWarn (1, __LINE__, __FILE__, CCTK_THORNSTRING,
                "Number of timers is inconsistent across processes; cannot collect timer information.  Number of timers on processor %d: %d",  
                CCTK_MyProc(cctkGH), timers->ntimers);
    return 0;
  }

  timers->secs_local = malloc(timers->ntimers * sizeof *timers->secs_local);
  assert (timers->ntimers==0 || timers->secs_local);
  if (CCTK_MyProc(cctkGH) == 0)
  {
    timers->secs_avg   = malloc(timers->ntimers * sizeof *timers->secs_avg  );
    timers->secs_min   = malloc(timers->ntimers * sizeof *timers->secs_min  );
    timers->secs_max   = malloc(timers->ntimers * sizeof *timers->secs_max  );
    assert (timers->ntimers==0 || timers->secs_avg  );
    assert (timers->ntimers==0 || timers->secs_min  );
    assert (timers->ntimers==0 || timers->secs_max  );
  }
  else
  {
    timers->secs_avg = NULL;
    timers->secs_min = NULL;
    timers->secs_max = NULL;
  }
  
  cTimerData * const td = CCTK_TimerCreateData();
  for (int i = 0; i < timers->ntimers; i++)
  {
    CCTK_TimerI(i, td);
    const cTimerVal * const tv = CCTK_GetClockValue(all_timers_clock, td);
    if (tv)
    {
      timers->secs_local[i] = CCTK_TimerClockSeconds(tv);
    }
    else
    {
      CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "Clock \"%s\" not found for timer #%d \"%s\"",
                 all_timers_clock, i, CCTK_TimerName(i));
      timers->secs_local[i] = -1;
    }
  }
  CCTK_TimerDestroyData(td);
  
  const int reduce_avg = CCTK_ReductionArrayHandle ("average");
  const int reduce_min = CCTK_ReductionArrayHandle ("minimum");
  const int reduce_max = CCTK_ReductionArrayHandle ("maximum");
  if (reduce_avg < 0 || reduce_min < 0 || reduce_max < 0)
  {
    CCTK_WARN (CCTK_WARN_ABORT,
               "Reduction operators for average, minimum, or maximum not defined");
  }
  const int ierr1 = CCTK_ReduceLocArrayToArray1D
    (cctkGH, 0, reduce_avg,
     timers->secs_local, timers->secs_avg, timers->ntimers, CCTK_VARIABLE_REAL);
  const int ierr2 = CCTK_ReduceLocArrayToArray1D
    (cctkGH, 0, reduce_min,
     timers->secs_local, timers->secs_min, timers->ntimers, CCTK_VARIABLE_REAL);
  const int ierr3 = CCTK_ReduceLocArrayToArray1D
    (cctkGH, 0, reduce_max,
     timers->secs_local, timers->secs_max, timers->ntimers, CCTK_VARIABLE_REAL);
  if (ierr1 || ierr2 || ierr3)
  {
    CCTK_WARN (CCTK_WARN_ABORT,
               "Error in calling reduction operators");
  }
  return 1;
}



/* Quote a string so that it can be output as CSV entry.  Return a
   newly allocated string that must be freed by the caller.  */
static char *QuoteForCSV (const char *const string)
{
  const int len = strlen(string);
  /* Allocate sufficient space */
  char *const res = malloc(2*len+3);
  char *q = res;
  /* Begin with a quote */
  *q++ = '"';
  /* Copy string into result, quoting as necessary */
  for (const char *p = string; *p; ++p) {
    /* Quote all quote characters by doubling them (so <"hi" there>
       becomes <""hi"" there> */
    if (*p == '"') *q++ = '"';
    *q++ = *p;
  }
  /* End with a quote */
  *q++ = '"';
  *q = '\0';
  return res;
}



/* Quote a string so that it can be output as TSV entry.  Return a
   newly allocated string that must be freed by the caller.  */
static char *QuoteForTSV (const char *const string)
{
  const int len = strlen(string);
  /* Allocate sufficient space */
  char *const res = malloc(len+1);
  char *q = res;
  /* Copy string into result, replacing tabs as necessary */
  for (const char *p = string; *p; ++p) {
    if (*p == '\t') {
      *q++ = ' ';
    } else {
      *q++ = *p;
    }
  }
  *q = '\0';
  return res;
}
