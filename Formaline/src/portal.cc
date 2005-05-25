// $Header$

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"
#include "cctk_Version.h"
#include "util_Network.h"

#include "connection.hh"



extern "C"
void
Formaline_Portal (CCTK_ARGUMENTS)
{
  if (CCTK_MyProc (cctkGH) != 0) return;
  
  /* Create a unique job id */
  char const * restrict const jobid = "not unique";
  
  connection conn (jobid);
  
  
  
  /* Cactus */
  
  {
    char const * const cactus_version = CCTK_FullVersion();
    conn.store ("Cactus version", cactus_version);
  }
  
  
  
  /* Compiling */
  
#if 0
  {
    char const * const compile_user = CCTK_CompileUser();
    conn.store ("compile user", compile_user);
  }
#endif
  
  {
    char const * const compile_date = CCTK_CompileDate();
    conn.store ("compile date", compile_date);
  }
  
  {
    char const * const compile_time = CCTK_CompileTime();
    conn.store ("compile time", compile_time);
  }
  
  
  
  /* Running */
  
#if 0
  {
    char const * const run_user = CCTK_RunUser();
    conn.store ("run user", run_user);
  }
#else
  {
    char const * const run_user = getenv ("USER");
    conn.store ("run user", run_user);
  }
#endif
  
  {
    char run_date [1000];
    Util_CurrentDate (sizeof run_date, run_date);
    conn.store ("run date", run_date);
  }
  
  {
    char run_time [1000];
    Util_CurrentTime (sizeof run_time, run_time);
    conn.store ("run time", run_time);
  }
  
  {
    char run_host [1000];
    Util_GetHostName (run_host, sizeof run_host);
    conn.store ("run host", run_host);
  }
  
  {
    int type;
    void const * const ptr
      = CCTK_ParameterGet ("run_title", "Cactus", & type);
    assert (type == CCTK_VARIABLE_STRING);
    char const * const run_title = static_cast<char const *> (ptr);
    conn.store ("run title", run_title);
  }
  
  
  
  /* Parameters */
  
  {
    char ** argv;
    int argc;
    int n;
    CCTK_CommandLine (& argv);
    for (argc = 0; argv [argc]; ++ argc);
    conn.store ("argc", argc);
    for (n = 0; n < argc; ++ n)
    {
      char buffer [1000];
      snprintf (buffer, sizeof buffer, "argv[%d]", n);
      conn.store (buffer, argv[n]);
    }
  }
  
  {
    char parameter_filename [10000];
    CCTK_ParameterFilename (sizeof parameter_filename, parameter_filename);
    conn.store ("parameter filename", parameter_filename);
  }
  
  {
    char parameter_filename [10000];
    char parameter_file [1000000];
    size_t count;
    FILE * file;
    CCTK_ParameterFilename (sizeof parameter_filename, parameter_filename);
    file = fopen (parameter_filename, "r");
    count = fread (parameter_file, 1, sizeof parameter_file - 1, file);
    fclose (file);
    assert (count < sizeof parameter_file - 1);
    parameter_file [count] = '\0';
    conn.store ("parameter file", parameter_file);
  }
  
  {
    int type;
    void const * const ptr
      = CCTK_ParameterGet ("out_dir", "IO", & type);
    assert (type == CCTK_VARIABLE_STRING);
    char const * const out_dir = static_cast<char const *> (ptr);
    conn.store ("out dir", out_dir);
  }
  
  {
    int nprocs;
    nprocs = CCTK_nProcs (cctkGH);
    conn.store ("nprocs", nprocs);
  }
  
}
