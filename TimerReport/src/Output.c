 /*@@
   @file      Output.c
   @date      July 6 2003
   @author    Gabrielle Allen
   @desc
              Functions to report the timers
   @enddesc
 @@*/

#include <cctk.h>
#include <cctk_Arguments.h>
#include <cctk_Parameters.h>
#include <cctk_Schedule.h>
#include <util_String.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CCTK_MPI
#  include <mpi.h>
#endif

/********************************************************************
 *********************     Local Data Types   ***********************
 ********************************************************************/

struct timer_stats {
  char ** restrict names;          /* timer names */
  CCTK_REAL * restrict secs_avg;   /* global average */
  CCTK_REAL * restrict secs_min;   /* global minimum */
  CCTK_REAL * restrict secs_max;   /* global maximum */
  int ntimers;
};

/********************************************************************
 ********************* Local Routine Prototypes *********************
 ********************************************************************/

static int min(int x, int y);

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

static const char *const sep = "======================================================================";

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

static int min(int x, int y)
{
  return x<y ? x : y;
}



static void Output (CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  if (output_schedule_timers)
  {
    PrintTimes(CCTK_PASS_CTOC);
  }

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
          const char *name = CCTK_TimerName(i);
          
          if (name == NULL)
            name = "(null)";

          CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
                     "Clock \"%s\" not found for timer #%d \"%s\"",
                     all_timers_clock, i, name);
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
          const char * const name = timers.names[i];
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
    free(filename_tsv);
    
    first_time = 0;
    last_ntimers = timers.ntimers;
    
  } /* if root processor */
  
  for (int n=0; n<timers.ntimers; ++n)
  {
    free (timers.names[n]);
  }
  free (timers.names);
  free (timers.secs_avg);
  free (timers.secs_min);
  free (timers.secs_max);
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
        const char * const name = timers.names[i];
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
  
  for (int n=0; n<timers.ntimers; ++n)
  {
    free (timers.names[n]);
  }
  free (timers.names);
  free (timers.secs_avg);
  free (timers.secs_min);
  free (timers.secs_max);
  
  return;
}



static struct timer_stats const * compare_timers = NULL;

static int compare(const void *a, const void *b)
{
  int const idx1 = *(const int *)a;
  int const idx2 = *(const int *)b;
  
  /* compare average times */
  double const d =
    compare_timers->secs_avg[idx2] -
    compare_timers->secs_avg[idx1];
  
  if (d > 0)
    return +1;
  else if (d < 0)
    return -1;
  else
    return 0;
}

static void PrintTopTimers (CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;
  
  /* Collect timing information from all processes */
  struct timer_stats timers;
  if (! CollectTimerInfo (cctkGH, &timers))
  {
    return;
  }
  
  /* Output the times only on the root process (because they are not
     reduced onto the other processes anyway) */
  if (CCTK_MyProc(cctkGH) != 0)
  {
    return;
  }
  
  /* Sort timers (by average) */
  int idx[timers.ntimers];
  for (int i=0; i<timers.ntimers; ++i)
  {
    idx[i] = i;
  }
  compare_timers = &timers;
  qsort (idx, timers.ntimers, sizeof(*idx), compare);
  compare_timers = NULL;
  
  CCTK_VInfo(CCTK_THORNSTRING,
             "Top timers at iteration %d time %g",
             cctk_iteration, (double)cctk_time);
  
  printf("%s\n", sep);
  printf("%5s   %7s %7s %7s   %s (%s)\n",
         "%", "Time/s", "Min/s", "Max/s", "Timer", all_timers_clock);
  printf("%s\n", sep);
  
  /* This should be the "CCTK total time" timer */
  int const total_idx = 0;
  assert (timers.ntimers > total_idx);
  
  CCTK_REAL const max_time = timers.secs_max[idx[total_idx]];
  int const digits = floor(log10(max_time) + 1.0e-4) + 1;
  
  /* Output timing results */
  for (int i=0; i<min(timers.ntimers, n_top_timers); ++i)
  {
    double const percent =
      100.0 * timers.secs_avg[idx[i]] / timers.secs_avg[idx[total_idx]];
    
    /* field widths: 5+3 + 7+1 + 7+1 + 7+3 + n = 80 */
    printf("%5.1f   %7.*f %7.*f %7.*f   %s\n",
           percent,
           6-digits, timers.secs_avg[idx[i]],
           6-digits, timers.secs_min[idx[i]],
           6-digits, timers.secs_max[idx[i]],
           timers.names[idx[i]]);
  }
  printf("%s\n", sep);

  for (int n=0; n<timers.ntimers; ++n)
  {
    free (timers.names[n]);
  }
  free (timers.names);
  free (timers.secs_avg);
  free (timers.secs_min);
  free (timers.secs_max);
  
  return;
}



/* Note: Timer names are truncated to 100 characters for simplicity */
#define TIMERNAME_LENGTH 101    /* this includes the NUL character */

typedef char timername_t[TIMERNAME_LENGTH];
static timername_t *compare_string_array = NULL;
static int compare_string(void const *const a, void const *const b)
{
  int const ia = *(int const*)a;
  int const ib = *(int const*)b;
  return strcmp(compare_string_array[ia], compare_string_array[ib]);
}



/* Collect timer information onto the root processor */
static int CollectTimerInfo(cGH const *restrict const cctkGH,
                            struct timer_stats *restrict const timers)
{
  DECLARE_CCTK_PARAMETERS;
  
  assert(timers);
  
  /* Gather number of timers from each process */
  int const myproc = CCTK_MyProc(cctkGH);
  int const nprocs = CCTK_nProcs(cctkGH);
  
  int my_ntimers = CCTK_NumTimers();
  int all_ntimers[nprocs];
#ifdef CCTK_MPI
  MPI_Gather(&my_ntimers, 1, MPI_INT, all_ntimers, 1, MPI_INT, 0,
             MPI_COMM_WORLD);
#else
  memcpy(all_ntimers, &my_ntimers, sizeof *all_ntimers);
#endif
  int total_ntimers = 0;
  if (myproc == 0) {
    for (int p=0; p<nprocs; ++p) {
      total_ntimers += all_ntimers[p];
    }
  }
  
  /* Determine local timer names and their values */
  /* (these arrays can be too large for the stack_ */
  timername_t *my_timernames = malloc(my_ntimers * sizeof *my_timernames);
  assert(my_ntimers==0 || my_timernames);
  for (int n=0; n<my_ntimers; ++n) {
    const char *name = CCTK_TimerName(n);
    
    if (name == NULL)
      snprintf(my_timernames[n], TIMERNAME_LENGTH-1, "DESTROYED TIMER %5d", n);
    else
      strncpy(my_timernames[n], name, TIMERNAME_LENGTH-1);
    my_timernames[n][TIMERNAME_LENGTH-1] = '\0';
  }
  double my_timervalues[my_ntimers];
  {
    cTimerData *const td = CCTK_TimerCreateData();
    for (int n=0; n<my_ntimers; ++n) {
      CCTK_TimerI(n, td);
      cTimerVal const *const tv = CCTK_GetClockValue(all_timers_clock, td);
      if (!tv) {
        const char *name = CCTK_TimerName(n);
    
        if (name == NULL)
          name = "(null)";

        CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
                   "Clock \"%s\" not found for timer #%d \"%s\"",
                   all_timers_clock, n, name);
        my_timervalues[n] = -1;
      } else {
        my_timervalues[n] = CCTK_TimerClockSeconds(tv);
      }
    }
    CCTK_TimerDestroyData(td);
  }
  
  /* Gather timer names and values from each process */
  /* (these arrays can likely be too large for the stack) */
  timername_t *all_timernames = malloc(total_ntimers * sizeof *all_timernames);
  assert(total_ntimers==0 || all_timernames);
  double all_timervalues[total_ntimers];
  int name_displacements[nprocs], value_displacements[nprocs];
  int name_counts[nprocs];
  name_displacements[0] = 0;
  value_displacements[0] = 0;
  name_counts[0] = all_ntimers[0] * TIMERNAME_LENGTH;
  for (int p=1; p<nprocs; ++p) {
    name_displacements[p] =
      name_displacements[p-1] + all_ntimers[p-1] * TIMERNAME_LENGTH;
    value_displacements[p] = value_displacements[p-1] + all_ntimers[p-1];
    name_counts[p] = all_ntimers[p] * TIMERNAME_LENGTH;
  }
#ifdef CCTK_MPI
  MPI_Gatherv(my_timernames, my_ntimers*TIMERNAME_LENGTH, MPI_CHAR,
              all_timernames, name_counts, name_displacements, MPI_CHAR,
              0, MPI_COMM_WORLD);
  MPI_Gatherv(my_timervalues, my_ntimers, MPI_DOUBLE,
              all_timervalues, all_ntimers, value_displacements, MPI_DOUBLE,
              0, MPI_COMM_WORLD);
#else
  memcpy(all_timernames, my_timernames, my_ntimers*TIMERNAME_LENGTH);
  memcpy(all_timervalues, my_timervalues, my_ntimers*sizeof *all_timervalues);
#endif
  
  /* Continue only on the root process */
  if (myproc != 0) {
    timers->ntimers  = 0;
    timers->names    = NULL;
    timers->secs_avg = NULL;
    timers->secs_min = NULL;
    timers->secs_max = NULL;
    free(all_timernames);
    free(my_timernames);
    
    return 1;
  }
  
  /* Construct global list of timers: sort, then unique */
  /* TODO: sort the processes' timers separately (and in parallel),
     then merge them */
  int sort_index[total_ntimers];
  for (int i=0; i<total_ntimers; ++i) {
    sort_index[i] = i;
  }
  assert(!compare_string_array);
  compare_string_array = all_timernames;
  qsort(sort_index, total_ntimers, sizeof *sort_index, compare_string);
  compare_string_array = NULL;
  int unique_timers = 0;
  if (total_ntimers > 0) {
    unique_timers = 1;        /* first timer is always unique */
    for (int i=1; i<total_ntimers; ++i) {
      if (strcmp(all_timernames[sort_index[i]],
                 all_timernames[sort_index[unique_timers-1]]) != 0)
      {
        sort_index[unique_timers++] = sort_index[i];
      }
    }
  }
  
  /* Allocate timer data structure */
  timers->ntimers = unique_timers;
  assert(timers->ntimers >= 0);
  
  timers->names    = malloc(timers->ntimers * sizeof *timers->names   );
  timers->secs_avg = malloc(timers->ntimers * sizeof *timers->secs_avg);
  timers->secs_min = malloc(timers->ntimers * sizeof *timers->secs_min);
  timers->secs_max = malloc(timers->ntimers * sizeof *timers->secs_max);
  assert(timers->ntimers==0 || timers->names   );
  assert(timers->ntimers==0 || timers->secs_avg);
  assert(timers->ntimers==0 || timers->secs_min);
  assert(timers->ntimers==0 || timers->secs_max);
  
  for (int n=0; n<unique_timers; ++n) {
    timers->names[n] = strdup(all_timernames[sort_index[n]]);
  }
  
  /* Reduce timer values */
  for (int n=0; n<timers->ntimers; ++n) {
    double count  = 0.0;
    double sum    = 0.0;
    double minval = HUGE_VAL;
    double maxval = 0.0;
    /* Reduce over all processes */
    for (int p=0; p<nprocs; ++p) {
      int const name_offset = name_displacements[p] / TIMERNAME_LENGTH;
      /* Look for this timer */
      /* TODO: sort, then use bsearch */
      int i;
      for (i=0; i<all_ntimers[p]; ++i) {
        if (strcmp(timers->names[n], all_timernames[name_offset+i]) == 0) break;
      }
      if (i < all_ntimers[p]) {
        /* Found the timer */
        double const value = all_timervalues[value_displacements[p]+i];
        count += 1;
        sum   += value;
        minval = fmin(minval, value);
        maxval = fmax(maxval, value);
      } else {
        /* Timer does not exist on this process -- ignore */
      }
    }
    if (count == 0) {
      /* Special case to make result look nicer */
      timers->secs_avg[n] = -1; /* instead of nan */
      timers->secs_min[n] = -1; /* instead of infinity */
      timers->secs_max[n] = maxval; /* zero */
    } else {
      timers->secs_avg[n] = sum / count;
      timers->secs_min[n] = minval;
      timers->secs_max[n] = maxval;
    }
  }
  
  free(all_timernames);
  free(my_timernames);
  
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
