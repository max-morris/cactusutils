#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

#include "cctk.h"
#include "util_Network.h"

// IRIX wants this before <time.h>
#if HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif

#if TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else
#  if HAVE_SYS_TIME_H
#    include <sys/time.h>
#  elif HAVE_TIME_H
#    include <time.h>
#  endif
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "id.hh"



namespace Formaline
{
  
  using namespace std;
  
  
  
  // Unique build ID
  extern "C" char const build_id[];
  
  
  
  // Get a unique build id
  char const *
  get_build_id (cGH const * const cctkGH)
  {
    return build_id;
  }
  
  
  
  // Get a unique job id
  char const *
  get_job_id (cGH const * const cctkGH)
  {
    // Unique job ID
    static char * job_id = 0;
    
    if (job_id != 0) return job_id;
    
    ostringstream job_idbuf;
    job_idbuf << "job-";
    
    char cparfilename [10000];
    CCTK_ParameterFilename (sizeof cparfilename, cparfilename);
    string parfilename (cparfilename);
    size_t const last_slash = parfilename.rfind ('/');
    if (last_slash < string::npos) {
      parfilename.erase (0, last_slash + 1);
    }
    size_t const first_dot = parfilename.find ('.');
    parfilename.erase (first_dot);
    {
      string::iterator it = parfilename.begin();
      while (it != parfilename.end()) {
        char const c = *it;
        if (isalnum (c) or c == '+' or c == '-' or c == '.' or c == '_') {
          // Allow character
          ++ it;
        } else {
          // Remove character
          it = parfilename.erase (it);
        }
      }
    }
    job_idbuf << parfilename;
    
    job_idbuf << "-";
    
    char run_host [1000];
    Util_GetHostName (run_host, sizeof run_host);
    job_idbuf << run_host;
    
    job_idbuf << "-";
    
#if 0
    char const * const run_user = CCTK_RunUser();
#else
    char const * const run_user = getenv ("USER");
#endif
    job_idbuf << run_user;
    
    job_idbuf << "-";
    
    time_t const tim = time (0);
    struct tm * const ptm = gmtime (& tim);
    job_idbuf << setfill ('0')
              << setw(4) << 1900 + ptm->tm_year
              << "."
              << setw(2) << ptm->tm_mon + 1
              << "."
              << setw(2) << ptm->tm_mday
              << "-"
              << setw(2) << ptm->tm_hour
              << "."
              << setw(2) << ptm->tm_min
              << "."
              << setw(2) << ptm->tm_sec;
    
    job_idbuf << "-";
    
    pid_t const pid = getpid();
    job_idbuf << pid;
    
    string const job_idstr = job_idbuf.str();
    job_id = strdup (job_idstr.c_str());
    
    return job_id;
  }
  
  
  
  extern "C" CCTK_POINTER_TO_CONST
  Formaline_UniqueBuildID (CCTK_POINTER_TO_CONST const cctkGH_)
  {
    cGH const * const cctkGH = static_cast<cGH const *> (cctkGH_);
    return static_cast<CCTK_POINTER_TO_CONST> (get_build_id (cctkGH));
  }
  
  extern "C" CCTK_POINTER_TO_CONST
  Formaline_UniqueSimulationID (CCTK_POINTER_TO_CONST const cctkGH_)
  {
    cGH const * const cctkGH = static_cast<cGH const *> (cctkGH_);
    return static_cast<CCTK_POINTER_TO_CONST> (get_job_id (cctkGH));
  }
  
  
  
  extern "C" int
  Formaline_PrintIDs ()
  {
    CCTK_VInfo (CCTK_THORNSTRING, "Build id: %s", get_build_id (0));
    CCTK_VInfo (CCTK_THORNSTRING, "Simulation id: %s", get_job_id (0));
    return 0;
  }
  
} // namespace Formaline
