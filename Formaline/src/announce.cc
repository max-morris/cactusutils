// $Header$

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

#include <sys/types.h>
#include <unistd.h>

#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"
#include "cctk_Version.h"
#include "util_Network.h"

#include "http_Content.h"

#include "file.hh"
#include "multistorage.hh"
#include "portal.hh"

using namespace std;



static char * jobid = 0;



  // Create a unique job id
static void
create_jobid (CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;
  
  ostringstream jobidbuf;
  
  char run_host [1000];
  Util_GetHostName (run_host, sizeof run_host);
  jobidbuf << run_host;
  
  jobidbuf << "-";
  
#if 0
  char const * const run_user = CCTK_RunUser();
#else
  char const * const run_user = getenv ("USER");
#endif
  jobidbuf << run_user;
  
  jobidbuf << "-";
  
  time_t const tim = time (0);
  struct tm * const ptm = gmtime (& tim);
  jobidbuf << setfill ('0')
           << setw(4) << ptm->tm_year
           << setw(2) << ptm->tm_mon
           << setw(2) << ptm->tm_mday
           << "-"
           << setw(2) << ptm->tm_hour
           << setw(2) << ptm->tm_min
           << setw(2) << ptm->tm_sec;
  
  jobidbuf << "-";
  
  pid_t const pid = getpid();
  jobidbuf << pid;
  
  string const jobidstr = jobidbuf.str();
  jobid = strdup (jobidstr.c_str());
}



extern "C"
void
Formaline_AnnounceInitial (CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;
  
  // Only store from the root processor
  if (CCTK_MyProc (cctkGH) != 0) return;
  
  
  
  create_jobid (cctkGH);
  
  
  
  multistorage stores;
  
  if (announce_to_portal)
  {
    stores.add_storage (new portal (jobid, storage::initial));
  }
  
  if (store_into_file)
  {
    stores.add_storage (new file (jobid, storage::initial));
  }
  
  if (stores.num_storages() == 0) return;
  
  
  
  // Information in the Portal/Announce format
  {
    // Don't know what this is for
    stores.store ("jobtype", "default");
  }
  {
    int type;
    void const * const ptr
      = CCTK_ParameterGet ("cctk_run_title", "Cactus", & type);
    assert (type == PARAMETER_STRING);
    char const * const run_title = * static_cast<char const * const *> (ptr);
    stores.store ("app_title", run_title);
  }
  {
    char run_date [1000];
    Util_CurrentDate (sizeof run_date, run_date);
    char run_time [1000];
    Util_CurrentTime (sizeof run_time, run_time);
    ostringstream timebuf;
    timebuf << run_date << " " << run_time;
    string const timestr = timebuf.str();
    stores.store ("start_time", timestr.c_str());
  }
  {
    // Don't know what this is for
    stores.store ("project_name", "");
  }
  {
    stores.store ("output_files", out_dir);
  }
  {
    char run_host [1000];
    Util_GetHostName (run_host, sizeof run_host);
    stores.store ("host", run_host);
  }
  {
    unsigned long http_port;
#ifdef __HTTP_CONTENT_H__
    if (CCTK_IsThornActive ("HTTPD"))
    {
      // Thorn is compiled in and active, ask it
      http_port = HTTP_Port();
    }
    else
    {
      // Thorn is compiled in but not active, ignore it
      http_port = 0;
    }
#else
    {
      // Thorn is not compiled in, ignore it
      http_port = 0;
    }
#endif
    stores.store ("port", (int) http_port);
  }
  {
    stores.store ("portal_username", portal_username);
  }
  {
#if 0
    char const * const run_user = CCTK_RunUser();
#else
    char const * const run_user = getenv ("USER");
#endif
    stores.store ("local_username", run_user);
  }
  {
    char parameter_filename [10000];
    CCTK_ParameterFilename (sizeof parameter_filename, parameter_filename);
    stores.store ("parameter_file", parameter_filename);
  }
  {
    char ** argv;
    int argc;
    int n;
    CCTK_CommandLine (& argv);
    for (argc = 0; argv [argc]; ++ argc);
    stores.store ("executable", argc == 0 ? "" : argv[0]);
  }
  {
    // Don't know what this is for
    stores.store ("data_directory", "");
  }
  {
    // Could also be "private"
    stores.store ("app_visibility", "public");
  }
  {
    // Could apparently be none, register, update, deregister
    stores.store ("notification_reports", "");
  }
  {
    // Could apparently be none, email, im, sms
    stores.store ("notification_methods", "");
  }
  
  
  
  // Cactus
  
  {
    char const * const cactus_version = CCTK_FullVersion();
    stores.store ("Cactus version", cactus_version);
  }
  
  
  
  // Compiling
  
#if 0
  {
    char const * const compile_user = CCTK_CompileUser();
    stores.store ("compile user", compile_user);
  }
#endif
  
  {
    char const * const compile_date = CCTK_CompileDate();
    stores.store ("compile date", compile_date);
  }
  
  {
    char const * const compile_time = CCTK_CompileTime();
    stores.store ("compile time", compile_time);
  }
  
  
  
  // Running
  
#if 0
  {
    char const * const run_user = CCTK_RunUser();
    stores.store ("run user", run_user);
  }
#else
  {
    char const * const run_user = getenv ("USER");
    stores.store ("run user", run_user);
  }
#endif
  
  {
    char run_date [1000];
    Util_CurrentDate (sizeof run_date, run_date);
    stores.store ("run date", run_date);
  }
  
  {
    char run_time [1000];
    Util_CurrentTime (sizeof run_time, run_time);
    stores.store ("run time", run_time);
  }
  
  {
    char run_host [1000];
    Util_GetHostName (run_host, sizeof run_host);
    stores.store ("run host", run_host);
  }
  
  {
    int type;
    void const * const ptr
      = CCTK_ParameterGet ("cctk_run_title", "Cactus", & type);
    assert (type == PARAMETER_STRING);
    char const * const run_title = * static_cast<char const * const *> (ptr);
    stores.store ("run title", run_title);
  }
  
  
  
  // Command line arguments
  
  {
    char ** argv;
    int argc;
    int n;
    CCTK_CommandLine (& argv);
    for (argc = 0; argv [argc]; ++ argc);
    stores.store ("argc", argc);
    for (n = 0; n < argc; ++ n)
    {
      char buffer [1000];
      snprintf (buffer, sizeof buffer, "argv[%d]", n);
      stores.store (buffer, argv[n]);
    }
  }
  
  {
    char parameter_filename [10000];
    CCTK_ParameterFilename (sizeof parameter_filename, parameter_filename);
    stores.store ("parameter filename", parameter_filename);
  }
  
#if 0
  // This is superfluous, and it does not look nice
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
    stores.store ("parameter file", parameter_file);
  }
#endif
  
  {
    int type;
    void const * const ptr
      = CCTK_ParameterGet ("out_dir", "IOUtil", & type);
    assert (type == PARAMETER_STRING);
    char const * const out_dir = * static_cast<char const * const *> (ptr);
    stores.store ("out dir", out_dir);
  }
  
  {
    int nprocs;
    nprocs = CCTK_nProcs (cctkGH);
    stores.store ("nprocs", nprocs);
  }
  
  
  
#if 0
  // All Cactus thorns
  
  {
    int const numthorns = CCTK_NumCompiledThorns ();
    for (int thorn = 0; thorn < numthorns; ++ thorn)
    {
      char const * const thornname = CCTK_CompiledThorn (thorn);
      
      ostringstream keybuf;
      keybuf << "thorns/" << thornname;
      string const keystr = keybuf.str();
      char const * const key = keystr.c_str();
      
      if (CCTK_IsThornActive (thornname))
      {
        stores.store (key, "active");
      }
      else
      {
        stores.store (key, "inactive");
      }
    }
  }
  
  
  
  // All Cactus parameters
  
  {
    int first = 1;
    for (;;)
    {
      cParamData const * parameter_data;
      char * parameter_fullname;
      
      int const ierr
        = CCTK_ParameterWalk (first, 0,
                              & parameter_fullname, & parameter_data);
      if (ierr > 0) break;
      assert (ierr >= 0);
      
      ostringstream keybuf;
      keybuf << "parameters/" << parameter_fullname;
      string const keystr = keybuf.str();
      char const * const key = keystr.c_str();
      
      int type;
      void const * const parameter_value
        = CCTK_ParameterGet (parameter_data->name, parameter_data->thorn,
                             & type);
      assert (type == parameter_data->type);
      assert (parameter_value != 0);
      
      switch (type)
      {
      case PARAMETER_BOOLEAN:
      case PARAMETER_INT:
        {
          CCTK_INT const value
            = * static_cast<CCTK_INT const *> (parameter_value);
          stores.store (key, value);
        }
        break;
      case PARAMETER_REAL:
        {
          CCTK_REAL const value
            = * static_cast<CCTK_REAL const *> (parameter_value);
          stores.store (key, value);
        }
        break;
      case PARAMETER_KEYWORD:
      case PARAMETER_STRING:
        {
          char const * const value
            = * static_cast<char const * const *> (parameter_value);
          stores.store (key, value);
        }
        break;
      default:
        assert (0);
      }
      
      free (parameter_fullname);
      
      first = 0;
    }
  }
#endif
  
  
  
  // Simulation state
  
  {
    stores.store ("cctk_iteration", cctk_iteration);
  }
}



extern "C"
void
Formaline_AnnounceUpdate (CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;
  
  // Only store from the root processor
  if (CCTK_MyProc (cctkGH) != 0) return;
  
  
  
  multistorage stores;
  
  if (announce_to_portal)
  {
    stores.add_storage (new portal (jobid, storage::update));
  }
  
  if (store_into_file)
  {
    stores.add_storage (new file (jobid, storage::update));
  }
  
  if (stores.num_storages() == 0) return;
  
  
  
  // Simulation state
  
  {
    stores.store ("cctk_iteration", cctk_iteration);
  }
}



extern "C"
void
Formaline_AnnounceFinal (CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;
  
  // Only store from the root processor
  if (CCTK_MyProc (cctkGH) != 0) return;
  
  
  
  multistorage stores;
  
  if (announce_to_portal)
  {
    stores.add_storage (new portal (jobid, storage::final));
  }
  
  if (store_into_file)
  {
    stores.add_storage (new file (jobid, storage::final));
  }
  
  if (stores.num_storages() == 0) return;
  
  
  
  // Simulation state
  
  {
    stores.store ("cctk_iteration", cctk_iteration);
  }
}
