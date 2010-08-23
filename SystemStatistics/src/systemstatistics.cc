
#include <stdio.h> 
#include <string.h> 
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>
#include "assert.h"

#include "cctk.h" 
#include "cctk_Arguments.h" 
#include "cctk_Parameters.h" 

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifndef HAVE_MALLINFO

// Provide a dummy mallinfo function if none is available
struct mallinfo {
  int arena;
  int ordblks;
  int smblks;
  int hblks;
  int hblkhd;
  int usmblks;
  int fsmblks;
  int uordblks;
  int fordblks;
  int keepcost;
};

struct mallinfo mallinfo()
{
  struct mallinfo m;
  m.arena = 0;
  m.ordblks = 0;
  m.smblks = 0;
  m.hblks = 0;
  m.hblkhd = 0;
  m.usmblks = 0;
  m.fsmblks = 0;
  m.uordblks = 0;
  m.fordblks = 0;
  m.keepcost = 0;
  return m;
}

#endif

static unsigned long int get_rss()
{
  unsigned int size=0; //       total program size
  unsigned int resident=0;//   resident set size
  unsigned int share=0;//      shared pages
  unsigned int text=0;//       text (code)
  unsigned int lib=0;//        library
  unsigned int data=0;//       data/stack
  unsigned int dt=0;//         dirty pages (unused in Linux 2.6)

  int page_size = sysconf(_SC_PAGESIZE);

  char buf[30];
  snprintf(buf, 30, "/proc/%u/statm", (unsigned)getpid());
  FILE* pf = fopen(buf, "r");
  // If the /proc filesystem does not exist, this file will not be
  // found and the function will return 0
  if (pf) 
  {
    fscanf(pf, "%u %u %u %u %u %u", &size, &resident, &share, &text, &lib, &data);
    fclose(pf);
  }
  return (unsigned long int ) resident * (unsigned long int) page_size;
}

static unsigned int get_majflt()
{
  int pid;
  char exe[256];
  char  state;
  int dummyi;
  unsigned long int dummyu;
  unsigned long int majflt;

  unsigned int page_size = sysconf(_SC_PAGESIZE);

  char buf[30];
  snprintf(buf, 30, "/proc/%u/stat", (unsigned)getpid());
  FILE* pf = fopen(buf, "r");
  // If the /proc filesystem does not exist, this file will not be
  // found and the function will return 0
  if (pf) 
  {
    fscanf(pf, "%d %s %c %d %d %d %d %d %lu %lu %lu %lu", 
           &pid, exe, &state, &dummyi,  &dummyi, &dummyi, &dummyi, 
           &dummyi, &dummyu, &dummyu, &dummyu, &majflt);
    fclose(pf);
  }
  return majflt * page_size;
}

extern "C" void SystemStatistics_Collect(CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS
  DECLARE_CCTK_PARAMETERS

  const int mb = 1024*1024;
  const int kb = 1024;

  *maxrss = get_rss();
  *majflt = get_majflt();
  *arena = mallinfo().arena;
  *ordblks = mallinfo().ordblks;
  *hblks = mallinfo().hblks;
  *hblkhd = mallinfo().hblkhd;
  *uordblks = mallinfo().uordblks;
  *fordblks = mallinfo().fordblks;
  *keepcost = mallinfo().keepcost;

  *maxrss_mb = get_rss() / mb;
  *majflt_mb = *majflt / mb;
  *arena_mb = *arena / mb;
  *ordblks_mb = *ordblks / mb;
  *hblks_mb = *hblks / mb;
  *hblkhd_mb = *hblkhd / mb;
  *uordblks_mb = *uordblks / mb;
  *fordblks_mb = *fordblks / mb;
  *keepcost_mb = *keepcost / mb;

  *maxrss_kb = get_rss() / kb;
  *majflt_kb = *majflt / kb;
  *arena_kb = *arena / kb;
  *ordblks_kb = *ordblks / kb;
  *hblks_kb = *hblks / kb;
  *hblkhd_kb = *hblkhd / kb;
  *uordblks_kb = *uordblks / kb;
  *fordblks_kb = *fordblks / kb;
  *keepcost_kb = *keepcost / kb;
}

// This function is only necessary because CarpetIOBasic doesn't
// reduce grid scalars
extern "C" void SystemStatistics_Reduce(CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS
  DECLARE_CCTK_PARAMETERS

  CCTK_INT   stats_local[] = 
    {*maxrss_mb, *majflt_mb, *arena_mb, *ordblks, *hblks, *hblkhd, 
     *uordblks, *fordblks, *keepcost}; // Some of these might be in the wrong units

  CCTK_INT   stats_minimum[sizeof(*stats_local)];
  CCTK_INT   stats_maximum[sizeof(*stats_local)];
  CCTK_INT   stats_average[sizeof(*stats_local)];

  int ierr = -1;

  ierr = CCTK_ReduceLocArrayToArray1D
    (cctkGH, -1, CCTK_ReductionArrayHandle ("minimum"),
     stats_local, stats_minimum, sizeof(*stats_local), CCTK_VARIABLE_INT);
  assert(ierr == 0);

  ierr = CCTK_ReduceLocArrayToArray1D
    (cctkGH, -1, CCTK_ReductionArrayHandle ("maximum"),
     stats_local, stats_maximum, sizeof(*stats_local), CCTK_VARIABLE_INT);
  assert(ierr == 0);

  ierr = CCTK_ReduceLocArrayToArray1D
    (cctkGH, -1, CCTK_ReductionArrayHandle ("average"),
     stats_local, stats_average, sizeof(*stats_local), CCTK_VARIABLE_INT);
  assert(ierr == 0);

//   if (CCTK_MyProc(cctkGH) == 0)
//   {
//     CCTK_VInfo(CCTK_THORNSTRING,
//                "Process memory statistics at iteration %d time %g:",
//                cctk_iteration, (double) cctk_time);
//     CCTK_VInfo(CCTK_THORNSTRING, "maxrss/MB min=%f max=%f avg=%f",
//                stats_minimum[0], stats_maximum[0], stats_average[0]);
//   }

  *maxrss_min_mb = stats_minimum[0];
  *maxrss_max_mb = stats_maximum[0];
  *maxrss_avg_mb = stats_average[0];
}
