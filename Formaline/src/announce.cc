#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"
#include "cctk_Version.h"
#include "util_Network.h"
#include "util_String.h"

// IRIX wants this before <time.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#elif HAVE_TIME_H
#include <time.h>
#endif
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef CCTK_MPI
#include <mpi.h>
#endif

#include "http_Content.h"

#include "file.hh"
#include "id.hh"
#include "json_file.hh"
#include "multistorage.hh"
#include "portal.hh"
#include "rdf.hh"
#include "thornlist.hh"

namespace Formaline {

using namespace std;

// Unique message IDs
static int warningID = 0;
static int infoID = 0;

// Time of late update
static CCTK_REAL last_update_time = 0;

// Get current time
static CCTK_REAL get_current_time() {
#ifdef HAVE_TIME_GETTIMEOFDAY
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec + tv.tv_usec / (CCTK_REAL)1.0e+6;
#else
  return 0;
#endif
}

extern "C" void Formaline_AnnounceInitial(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  // Only store from the root processor
  if (CCTK_MyProc(cctkGH) != 0)
    return;

  if (verbose)
    CCTK_INFO("Announcing initial meta information");

  if (create_id_files) {
    // Create files from the configuration, source, build, run, and
    // simulation ids
    // (This shows what jobs were run in the output directory)
    {
      ostringstream filenamebuf;
      filenamebuf << out_dir << "/formaline-" << get_config_id(cctkGH);
      string const filenamestring = filenamebuf.str();
      ofstream fil;
      fil.open(filenamestring.c_str(), ios::trunc);
      fil << get_config_id(cctkGH) << "\n";
      fil.close();
    }
    {
      ostringstream filenamebuf;
      filenamebuf << out_dir << "/formaline-" << get_build_id(cctkGH);
      string const filenamestring = filenamebuf.str();
      ofstream fil;
      fil.open(filenamestring.c_str(), ios::trunc);
      fil << get_build_id(cctkGH) << "\n";
      fil.close();
    }
    {
      ostringstream filenamebuf;
      filenamebuf << out_dir << "/formaline-" << get_simulation_id(cctkGH);
      string const filenamestring = filenamebuf.str();
      ofstream fil;
      fil.open(filenamestring.c_str(), ios::trunc);
      fil << get_simulation_id(cctkGH) << "\n";
      fil.close();
    }
    {
      ostringstream filenamebuf;
      filenamebuf << out_dir << "/formaline-" << get_run_id(cctkGH);
      string const filenamestring = filenamebuf.str();
      ofstream fil;
      fil.open(filenamestring.c_str(), ios::trunc);
      fil << get_run_id(cctkGH) << "\n";
      fil.close();
    }
  }

  // Announce
  {

    multistorage stores;

    if (announce_to_portal) {
      stores.add_storage(new portal(get_run_id(cctkGH), storage::initial));
    }

    if (send_as_rdf) {
      stores.add_storage(new rdf(get_run_id(cctkGH), storage::initial, cctkGH));
    }

    if (store_into_file) {
      stores.add_storage(new file(get_run_id(cctkGH), storage::initial));
    }

    if (store_into_json_file) {
      stores.add_storage(new json_file(get_run_id(cctkGH), storage::initial));
    }

    if (stores.num_storages() == 0)
      return;

    // Information in the Portal/Announce format

    {
      // Don't know what this is for
      stores.store("jobtype", "default");
    }

    {
      int type;
      void const *const ptr =
          CCTK_ParameterGet("cctk_run_title", "Cactus", &type);
      assert(type == PARAMETER_STRING);
      char const *const run_title = *static_cast<char const *const *>(ptr);
      stores.store("app_title", run_title);
    }

    {
      char run_date[1000];
      Util_CurrentDate(sizeof run_date, run_date);
      char run_time[1000];
      Util_CurrentTime(sizeof run_time, run_time);
      ostringstream timebuf;
      timebuf << run_date << " " << run_time;
      string const timestr = timebuf.str();
      stores.store("start_time", timestr.c_str());
    }

    {
      // Don't know what this is for
      stores.store("project_name", "");
    }

    { stores.store("output_files", out_dir); }

    {
      char run_host[1000];
      Util_GetHostName(run_host, sizeof run_host);
      stores.store("host", run_host);
    }

    {
      int const nprocs = CCTK_nProcs(cctkGH);
      stores.store("nprocs", nprocs);
    }

#if 0
      {
        char run_host [1000];
        char (* run_hosts) [1000] = 0;
        int const nprocs = CCTK_nProcs (cctkGH);
        int n;
        
        Util_GetHostName (run_host, sizeof run_host);
        stores.store ("host", run_host);
        
        run_hosts = malloc (nprocs * sizeof * run_hosts);
#ifdef CCTK_MPI
        // Note: Only the root processor actually comes here
        MPI_Gather (run_host, sizeof run_host, MPI_CHAR,
                    run_hosts, sizeof * run_hosts, MPI_CHAR,
                    0, MPI_COMM_WORLD);
#else
        assert (nprocs == 1);
        strcpy (run_hosts[0], run_host);
#endif
        for (n = 0; n < nprocs; ++ n) {
          ostringstream namebuf;
          namebuf << "hosts[" << n << "]";
          string const namestr = namebuf.str();
          strcpy (run_host, run_hosts[n]);
          stores.store (namestr.c_str(), run_host);
        }
        free (run_hosts);
      }
#endif

    {
      unsigned long http_port;
#ifdef __HTTP_CONTENT_H__
      if (CCTK_IsThornActive("HTTPD")) {
        // Thorn is compiled in and active, ask it
        http_port = HTTP_Port();
      } else {
        // Thorn is compiled in but not active, ignore it
        http_port = 0;
      }
#else
      {
        // Thorn is not compiled in, ignore it
        http_port = 0;
      }
#endif
      if (http_port != 0) {
        stores.store("port", (int)http_port);
      }
    }

    { stores.store("portal_username", portal_username); }

    {
#if 0
        char const * const run_user = CCTK_RunUser();
#else
      char const *const run_user = getenv("USER");
#endif
      stores.store("local_username", run_user);
    }

    {
      char parameter_filename[10000];
      CCTK_ParameterFilename(sizeof parameter_filename, parameter_filename);
      stores.store("parameter_filename", parameter_filename);
    }

    {
      char **argv;
      int const argc = CCTK_CommandLine(&argv);
      stores.store("executable", argc == 0 ? "" : argv[0]);
    }

    {
      // Don't know exactly what this is for -- send the IO output
      // directory
      stores.store("data_directory", out_dir);
    }

    {
      // Could also be "private"
      stores.store("app_visibility", "public");
    }

    {
      // Could apparently be none, register, update, deregister
      stores.store("notification_reports", "");
    }

    {
      // Could apparently be none, email, im, sms
      stores.store("notification_methods", "");
    }

    // PBS

    {
      // PBS logname
      char const *const pbs_logname = getenv("PBS_LOGNAME");
      stores.store("PBS_LOGNAME", pbs_logname);
    }

    {
      // PBS host
      char const *const pbs_host = getenv("PBS_HOST");
      stores.store("PBS_HOST", pbs_host);
    }

    {
      // PBS workdir
      char const *const pbs_workdir = getenv("PBS_WORKDIR");
      stores.store("PBS_WORKDIR", pbs_workdir);
    }

    // Cactus

    {
      char const *const cactus_version = CCTK_FullVersion();
      stores.store("Cactus_version", cactus_version);
    }

    // Compiling

    { stores.store("config_id", get_config_id(cctkGH)); }

    { stores.store("build_id", get_build_id(cctkGH)); }

#if 0
      {
        char const * const compile_user = CCTK_CompileUser();
        stores.store ("compile_user", compile_user);
      }
#endif

    {
      char const *const compile_date = CCTK_CompileDate();
      stores.store("compile_date", compile_date);
    }

    {
      char const *const compile_time = CCTK_CompileTime();
      stores.store("compile_time", compile_time);
    }

    // Running

    { stores.store("simulation_id", get_simulation_id(cctkGH)); }

    { stores.store("run_id", get_run_id(cctkGH)); }

#if 0
      {
        char const * const run_user = CCTK_RunUser();
        stores.store ("run_user", run_user);
      }
#else
    {
      char const *const run_user = getenv("USER");
      stores.store("run_user", run_user);
    }
#endif

    {
      char run_date[1000];
      Util_CurrentDate(sizeof run_date, run_date);
      stores.store("run_date", run_date);
    }

    {
      char run_time[1000];
      Util_CurrentTime(sizeof run_time, run_time);
      stores.store("run_time", run_time);
    }

    {
      char run_host[1000];
      Util_GetHostName(run_host, sizeof run_host);
      stores.store("run_host", run_host);
    }

    {
      int type;
      void const *const ptr =
          CCTK_ParameterGet("cctk_run_title", "Cactus", &type);
      assert(type == PARAMETER_STRING);
      char const *const run_title = *static_cast<char const *const *>(ptr);
      stores.store("run_title", run_title);
    }

    // Command line arguments

    {
      char **argv;
      int const argc = CCTK_CommandLine(&argv);
      stores.store("argc", argc);
      for (int n = 0; n < argc; ++n) {
        char buffer[1000];
        snprintf(buffer, sizeof buffer, "argv[%d]", n);
        stores.store(buffer, argv[n]);
      }
    }

    {
      char parameter_filename[10000];
      CCTK_ParameterFilename(sizeof parameter_filename, parameter_filename);
      stores.store("parameter_filename", parameter_filename);
    }

    {
      char parameter_filename[10000];
      char parameter_file[1000000];
      size_t count;
      FILE *file;
      CCTK_ParameterFilename(sizeof parameter_filename, parameter_filename);
      file = fopen(parameter_filename, "r");
      count = fread(parameter_file, 1, sizeof parameter_file - 1, file);
      fclose(file);
      assert(count < sizeof parameter_file - 1);
      parameter_file[count] = '\0';
      stores.store("parameter_file", parameter_file);
    }

    {
      char cwd[10000];
      getcwd(cwd, sizeof cwd);
      cwd[sizeof cwd - 1] = '\0'; // just to be sure
      stores.store("current_dir", cwd);
    }

    { stores.store("out_dir", out_dir); }

    // All Cactus thorns

    {
      multistorage thorn_stores;
      stores.open_group(thorn_stores, "thorns");
      int const numthorns = CCTK_NumCompiledThorns();
      for (int thorn = 0; thorn < numthorns; ++thorn) {
        char const *const thornname = CCTK_CompiledThorn(thorn);
        if (CCTK_IsThornActive(thornname)) {
          thorn_stores.store(thornname, "active");
        } else {
          thorn_stores.store(thornname, "inactive");
        }
      }
      stores.close_group(thorn_stores);
    }

    {
      multistorage arrangement_stores;
      stores.open_group(arrangement_stores, "thorn_arrangements");
      int const numthorns = ThornList::NumThorns();
      char const *const *const thornnames = ThornList::ThornNames();
      for (int thorn = 0; thorn < numthorns; ++thorn) {
        string const combination = thornnames[thorn];
        size_t const sep = combination.find('/');
        assert(sep != string::npos);
        string const arrangement = combination.substr(0, sep);
        string const thornname = combination.substr(sep + 1);
        arrangement_stores.store(thornname.c_str(), arrangement.c_str());
      }
      stores.close_group(arrangement_stores);
    }

    // All Cactus parameters

    {
      multistorage parameter_stores;
      stores.open_group(parameter_stores, "parameters");
      typedef pair<string, cParamData const *> param;

      // Collect all parameters into a list
      // (A list allows efficient inserting)
      list<param> paramlist;
      for (int first = 1;; first = 0) {
        cParamData const *parameter_data;
        char *parameter_fullname;

        int const ierr =
            CCTK_ParameterWalk(first, 0, &parameter_fullname, &parameter_data);
        if (ierr > 0)
          break;
        assert(ierr >= 0);

        // Skip parameters that belong to inactive thorns
        if (CCTK_IsThornActive(parameter_data->thorn)) {
          paramlist.push_back(
              param(string(parameter_fullname), parameter_data));
        }

        free(parameter_fullname);
      }

      // Copy the list into a vector
      // (A vector allows efficient sorting)
      vector<param> paramvector;
      paramvector.insert(paramvector.begin(), paramlist.begin(),
                         paramlist.end());

      // Sort the parameters
      sort(paramvector.begin(), paramvector.end());

      // Store the parameters
      for (vector<param>::const_iterator parameter = paramvector.begin();
           parameter != paramvector.end(); ++parameter) {
        char const *const parameter_fullname = parameter->first.c_str();
        cParamData const *const parameter_data = parameter->second;
        char const *const key = parameter_fullname;

        int type;
        void const *const parameter_value = CCTK_ParameterGet(
            parameter_data->name, parameter_data->thorn, &type);
        assert(parameter_value != 0);
        assert(type == parameter_data->type);

        int const times_set = CCTK_ParameterQueryTimesSet(
            parameter_data->name, parameter_data->thorn);

        switch (type) {
        case PARAMETER_BOOLEAN: {
          CCTK_INT default_value;
          int const ierr =
              CCTK_SetBoolean(&default_value, parameter_data->defval);
          assert(!ierr);
          CCTK_INT const value =
              *static_cast<CCTK_INT const *>(parameter_value);
          if (times_set > 0 or value != default_value) {
            parameter_stores.store(key, (bool)value);
          }
        } break;
        case PARAMETER_INT: {
          CCTK_INT const default_value = strtol(parameter_data->defval, 0, 0);
          CCTK_INT const value =
              *static_cast<CCTK_INT const *>(parameter_value);
          if (times_set > 0 or value != default_value) {
            parameter_stores.store(key, value);
          }
        } break;
        case PARAMETER_REAL: {
          char *const default_string = strdup(parameter_data->defval);
          assert(default_string);
          // Convert "d" and "D" to "e" and "E", because this is what
          // strtod expects
          for (char *p = default_string; *p; ++p) {
            switch (*p) {
            case 'd':
              *p = 'e';
              break;
            case 'D':
              *p = 'E';
              break;
            }
          }
          CCTK_REAL const default_value = strtod(default_string, 0);
          free(default_string);
          CCTK_REAL const value =
              *static_cast<CCTK_REAL const *>(parameter_value);
          if (times_set > 0 or value != default_value) {
            parameter_stores.store(key, value);
          }
        } break;
        case PARAMETER_KEYWORD: {
          char const *const value =
              *static_cast<char const *const *>(parameter_value);
          if (times_set > 0 or
              Util_StrCmpi(parameter_data->defval, value) != 0) {
            parameter_stores.store(key, value);
          }
        } break;
        case PARAMETER_STRING: {
          char const *const value =
              *static_cast<char const *const *>(parameter_value);
          if (times_set > 0 or strcmp(parameter_data->defval, value) != 0) {
            parameter_stores.store(key, value);
          }
        } break;
        default:
          assert(0);
        }
      } // for all parameters
      stores.close_group(parameter_stores);
    }

#if 0
      // Simulation state
      
      {
        // This information is wrong when recovering
        stores.store ("cctk_iteration", cctk_iteration);
        stores.store ("cctk_time", cctk_time);
      }
#endif

  } // announce

#if 0
    last_update_time = get_current_time();
#endif
}

class args {
  cGH const *cctkGH;
  multistorage &stores;
  char const *reductions;

public:
  args(cGH const *cctkGH_, multistorage &stores_, char const *reductions_);

  void output_variable(int varindex, char const *options) const;
};

args::args(cGH const *const cctkGH_, multistorage &stores_,
           char const *const reductions_)
    : cctkGH(cctkGH_), stores(stores_), reductions(reductions_) {}

void args::output_variable(int const varindex,
                           char const *const options) const {
  int const groupindex = CCTK_GroupIndexFromVarI(varindex);
  assert(groupindex >= 0);
  cGroup group;
  int const ierr = CCTK_GroupData(groupindex, &group);
  assert(!ierr);

  ostringstream keybuf, valbuf;
  char *const fullname = CCTK_FullName(varindex);
  assert(fullname);
  keybuf << "variables/" << fullname;
  free(fullname);
  string const keystr = keybuf.str();
  char const *const key = keystr.c_str();

  void const *const varptr = CCTK_VarDataPtrI(cctkGH, 0, varindex);
  if (!varptr) {
    // No storage -- do nothing
    // TODO: output warning
    return;
  }

  switch (group.grouptype) {
  case CCTK_SCALAR:
    switch (group.vartype) {
    case CCTK_VARIABLE_INT: {
      CCTK_INT const val = *(CCTK_INT const *)varptr;
      stores.store(key, val);
    } break;
    case CCTK_VARIABLE_REAL: {
      CCTK_REAL const val = *(CCTK_REAL const *)varptr;
      stores.store(key, val);
    } break;
    case CCTK_VARIABLE_COMPLEX: {
      CCTK_COMPLEX const val = *(CCTK_COMPLEX const *)varptr;
      {
        ostringstream keyrebuf;
        keyrebuf << key << ".Re";
        string const keyrestr = keyrebuf.str();
        char const *const keyre = keyrestr.c_str();
        stores.store(keyre, CCTK_CmplxReal(val));
      }
      {
        ostringstream keyimbuf;
        keyimbuf << key << ".Im";
        string const keyimstr = keyimbuf.str();
        char const *const keyim = keyimstr.c_str();
        stores.store(keyim, CCTK_CmplxImag(val));
      }
    } break;
    default:
      ;
      // not supported yet
      // TODO: output warning
    }
    break;
  case CCTK_ARRAY:
  case CCTK_GF:
    // not supported yet
    // TODO: output warning
    break;
  default:
    CCTK_WARN(0, "internal error");
  }
}

// C-style wrapper
extern "C" {
static void args_output_variable(int const varindex, char const *const options,
                                 void *const theargs0) {
  args *const theargs = static_cast<args *>(theargs0);
  theargs->output_variable(varindex, options);
}
}

extern "C" void Formaline_AnnounceUpdate(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  // Only store from the root processor
  if (CCTK_MyProc(cctkGH) != 0)
    return;

  if (get_current_time() < last_update_time + update_interval)
    return;

  if (verbose)
    CCTK_INFO("Announcing meta information update");

  // Announce
  {

    multistorage stores;

    if (announce_to_portal) {
      stores.add_storage(new portal(get_run_id(cctkGH), storage::update));
    }

    if (send_as_rdf) {
      stores.add_storage(new rdf(get_run_id(cctkGH), storage::update, cctkGH));
    }

    if (store_into_file) {
      stores.add_storage(new file(get_run_id(cctkGH), storage::update));
    }

    if (stores.num_storages() == 0)
      return;

    // Running

    {
      char run_date[1000];
      Util_CurrentDate(sizeof run_date, run_date);
      stores.store("run_date", run_date);
    }

    {
      char run_time[1000];
      Util_CurrentTime(sizeof run_time, run_time);
      stores.store("run_time", run_time);
    }

    // Simulation state

    {
      stores.store("cctk_iteration", cctk_iteration);
      stores.store("cctk_time", cctk_time);
    }

    // Groups and variables

    {
      args theargs(cctkGH, stores, out_reductions);

      int const icnt = CCTK_TraverseString(out_vars, args_output_variable,
                                           &theargs, CCTK_GROUP_OR_VAR);
      if (icnt < 0) {
        switch (icnt) {
        case -1:
          CCTK_WARN(0, "no callback routine was given");
          break;
        case -2:
          CCTK_WARN(2,
                    "option string is not associated with a group or variable");
          break;
        case -3:
          CCTK_WARN(2, "unterminated option string");
          break;
        case -4:
          CCTK_WARN(2, "garbage found at end of option string");
          break;
        case -5:
          CCTK_WARN(2, "invalid token in traversed string found");
          break;
        default:
          CCTK_WARN(1, "error while traversing output variables");
        }
      }
    }

  } // announce

  last_update_time = get_current_time();
}

extern "C" void Formaline_AnnounceFinal(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  // Only store from the root processor
  if (CCTK_MyProc(cctkGH) != 0)
    return;

  if (verbose)
    CCTK_INFO("Announcing final meta information");

  // Announce
  {

    multistorage stores;

    if (announce_to_portal) {
      stores.add_storage(new portal(get_run_id(cctkGH), storage::final));
    }

    if (send_as_rdf) {
      stores.add_storage(new rdf(get_run_id(cctkGH), storage::final, cctkGH));
    }

    if (store_into_file) {
      stores.add_storage(new file(get_run_id(cctkGH), storage::final));
    }

    if (stores.num_storages() == 0)
      return;

    // Running

    {
      char run_date[1000];
      Util_CurrentDate(sizeof run_date, run_date);
      stores.store("run_date", run_date);
    }

    {
      char run_time[1000];
      Util_CurrentTime(sizeof run_time, run_time);
      stores.store("run_time", run_time);
    }

    // Simulation state

    {
      stores.store("cctk_iteration", cctk_iteration);
      stores.store("cctk_time", cctk_time);
    }

  } // announce
}

void CatchWarning(int level, int line, char const *filename, char const *thorn,
                  char const *message, void *data);

void CatchInfo(char const *thorn, char const *message, void *data);

#if 1

extern "C" void Formaline_RegisterWarnings(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  if (max_warn_level >= 0) {
    int const ierr1 =
        CCTK_WarnCallbackRegister(0, max_warn_level, cctkGH, CatchWarning);
    assert(!ierr1);
  }

  if (output_info) {
    int const ierr2 = CCTK_InfoCallbackRegister(cctkGH, CatchInfo);
    assert(!ierr2);
  }
}

#else

extern "C" int Formaline_RegisterWarnings() {
  DECLARE_CCTK_PARAMETERS;

  if (max_warn_level >= 0) {
    int const ierr1 =
        CCTK_WarnCallbackRegister(0, max_warn_level, 0, CatchWarning);
    assert(!ierr1);
  }

  if (output_info) {
    int const ierr2 = CCTK_InfoCallbackRegister(0, CatchInfo);
    assert(!ierr2);
  }

  return 0;
}

#endif

void CatchWarning(int const level, int const line, char const *const filename,
                  char const *const thorn, char const *const message,
                  void *const data) {
  cGH *const cctkGH = static_cast<cGH *>(data);
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  // Only store from the root processor
  if (CCTK_MyProc(cctkGH) != 0)
    return;

  // Announce
  multistorage stores;

  if (announce_to_portal) {
    stores.add_storage(new portal(get_run_id(cctkGH), storage::update));
  }

  if (send_as_rdf) {
    stores.add_storage(new rdf(get_run_id(cctkGH), storage::update, cctkGH));
  }

  if (store_into_file) {
    stores.add_storage(new file(get_run_id(cctkGH), storage::update));
  }

  if (stores.num_storages() == 0)
    return;

  // Message
  {
    ostringstream keybuf;
    keybuf << "warning #" << warningID << "/level";
    string const keystr = keybuf.str();
    char const *const key = keystr.c_str();
    stores.store(key, level);
  }
  {
    ostringstream keybuf;
    keybuf << "warning #" << warningID << "/line";
    string const keystr = keybuf.str();
    char const *const key = keystr.c_str();
    stores.store(key, line);
  }
  {
    ostringstream keybuf;
    keybuf << "warning #" << warningID << "/file";
    string const keystr = keybuf.str();
    char const *const key = keystr.c_str();
    stores.store(key, filename);
  }
  {
    ostringstream keybuf;
    keybuf << "warning #" << warningID << "/thorn";
    string const keystr = keybuf.str();
    char const *const key = keystr.c_str();
    stores.store(key, thorn);
  }
  {
    ostringstream keybuf;
    keybuf << "warning #" << warningID << "/message";
    string const keystr = keybuf.str();
    char const *const key = keystr.c_str();
    stores.store(key, message);
  }

  ++warningID;
}

void CatchInfo(char const *const thorn, char const *const message,
               void *const data) {
  cGH *const cctkGH = static_cast<cGH *>(data);
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  // Only store from the root processor
  if (CCTK_MyProc(cctkGH) != 0)
    return;

  // Announce
  multistorage stores;

  if (announce_to_portal) {
    stores.add_storage(new portal(get_run_id(cctkGH), storage::update));
  }

  if (send_as_rdf) {
    stores.add_storage(new rdf(get_run_id(cctkGH), storage::update, cctkGH));
  }

  if (store_into_file) {
    stores.add_storage(new file(get_run_id(cctkGH), storage::update));
  }

  if (stores.num_storages() == 0)
    return;

  // Message
  {
    ostringstream keybuf;
    keybuf << "info #" << infoID << "/thorn";
    string const keystr = keybuf.str();
    char const *const key = keystr.c_str();
    stores.store(key, thorn);
  }
  {
    ostringstream keybuf;
    keybuf << "info #" << infoID << "/message";
    string const keystr = keybuf.str();
    char const *const key = keystr.c_str();
    stores.store(key, message);
  }

  ++infoID;
}

} // namespace Formaline
