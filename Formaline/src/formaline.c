/* $Header$ */

#include <assert.h>

#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"



static void
StoreIntKey (char const * key, int value);

static void
StoreStringKey (char const * key, char const * value);



void
Formaline (CCTK_ARGUMENTS)
{
  /* Cactus */
  
  {
    char const * const cactus_version = CCTK_FullVersion();
    StoreStringKey ("Cactus version", cactus_version);
  }
  
  
  
  /* Compiling */
  
  {
    char const * const compile_user = CCTK_CompileUser();
    StoreStringKey ("compile user", compile_user);
  }
  
  {
    char const * const compile_date = CCTK_CompileDate();
    StoreStringKey ("compile date", compile_date);
  }
  
  {
    char const * const compile_time = CCTK_CompileTime();
    StoreStringKey ("compile time", compile_time);
  }
  
  
  
  /* Running */
  
  {
    char const * const run_user = CCTK_RunUser();
    StoreStringKey ("run user", run_user);
  }
  
  {
    char run_date [1000];
    Util_CurrentDate (sizeof run_date, run_date);
    StoreStringKey ("run date", run_date);
  }
  
  {
    char run_time [1000];
    Util_CurrentTime (sizeof run_time, run_time);
    StoreStringKey ("run time", run_time);
  }
  
  {
    char run_host [1000];
    Util_GetHostName (run_host, sizeof run_host);
    StoreStringKey ("run host", run_host);
  }
  
  {
    char const * run_title;
    int type;
    run_title = CCTK_ParameterGet ("run_title", "Cactus", & type);
    assert (type == CCTK_VARIABLE_STRING);
    StoreStringKey ("run title", run_title);
  }
  
  
  
  /* Parameters */
  
  {
    char ** argv;
    int argc;
    int n;
    CCTK_CommandLine (& argv);
    for (argc = 0; argv [argc]; ++ argc);
    StoreIntKey ("argc", argc);
    for (n = 0; n < argc; ++ n)
    {
      char buffer [1000];
      snprintf (sizeof buffer, buffer, "argv[%d]", n);
      StoreStringKey (buffer, argv[n]);
    }
  }
  
  {
    char parameter_file_name [10000];
    CCTK_ParameterFilename (sizeof parameter_file_name, parameter_file_name);
    StoreStringKey ("parameter file name", parameter_file_name);
  }
}



/* Dummy implementations */

static void
StoreIntKey (char const * const key, int const value)
{
  assert (key);
  
  /* Do nothing */
}

static void
StoreStringKey (char const * const key, char const * const value)
{
  assert (key);
  assert (value);
  
  /* Do nothing */
}
