#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "cctk.h"
#include "cctk_Parameters.h"
#include "util_Network.h"

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

#include "id.hh"

namespace Formaline {

using namespace std;

// Configuration ID
extern "C" char const *const config_id;

// Unique build ID
extern "C" char const *const build_id;

// Get the configuration id
char const *get_config_id(cGH const *const cctkGH) { return config_id; }

// Get the unique build id
char const *get_build_id(cGH const *const cctkGH) { return build_id; }

// Get a unique simulation id
char const *get_simulation_id(cGH const *const cctkGH) {
  // Unique simulation ID
  static char *simulation_id = 0;

  if (simulation_id != 0)
    return simulation_id;

  string simulation_id_str;
  // Expect the simulation ID id a file in the parent directory of
  // the output directory
  DECLARE_CCTK_PARAMETERS;
  string const out_dir_str = string(out_dir);
  size_t const last_nonslash = out_dir_str.find_last_not_of("/");
  if (last_nonslash != string::npos) {
    size_t const last_slash = out_dir_str.rfind('/', last_nonslash);
    size_t const len = last_slash == string::npos ? 0 : last_slash + 1;
    string const simulation_id_filename =
        out_dir_str.substr(0, len) + string("SIMULATION_ID");

    // Open file and read content
    ifstream simulation_id_file(simulation_id_filename.c_str());
    if (simulation_id_file) {
      getline(simulation_id_file, simulation_id_str);
    }
  }

  if (simulation_id_str.empty()) {
    // Use the run ID as fallback if no simulation ID can be found
    simulation_id_str = string(get_run_id(cctkGH));
  }

  simulation_id = strdup(simulation_id_str.c_str());
  return simulation_id;
}

// Get a unique run id
char const *get_run_id(cGH const *const cctkGH) {
  // Unique run ID
  static char *run_id = 0;

  if (run_id != 0)
    return run_id;

  ostringstream run_idbuf;
  run_idbuf << "run-";

  char cparfilename[1000];
  CCTK_ParameterFilename(sizeof cparfilename, cparfilename);
  string parfilename(cparfilename);
  size_t const last_slash = parfilename.rfind('/');
  if (last_slash < string::npos) {
    parfilename.erase(0, last_slash + 1);
  }
  size_t const first_dot = parfilename.find('.');
  if (first_dot < string::npos) {
    parfilename.erase(first_dot);
  }
  {
    string::iterator it = parfilename.begin();
    while (it != parfilename.end()) {
      char const c = *it;
      if (isalnum(c) or c == '+' or c == '-' or c == '.' or c == '_') {
        // Allow character
        ++it;
      } else {
        // Remove character
        it = parfilename.erase(it);
      }
    }
  }
  run_idbuf << parfilename;

  run_idbuf << "-";

  char run_host[1000];
  Util_GetHostName(run_host, sizeof run_host);
  run_idbuf << run_host;

  run_idbuf << "-";

#if 0
    char const * const run_user = CCTK_RunUser();
#else
  char const *run_user = getenv("USER");
  if (not run_user) {
    run_user = "";
  }
#endif
  run_idbuf << run_user;

  run_idbuf << "-";

  time_t const tim = time(0);
  struct tm *const ptm = gmtime(&tim);
  run_idbuf << setfill('0') << setw(4) << 1900 + ptm->tm_year << "." << setw(2)
            << ptm->tm_mon + 1 << "." << setw(2) << ptm->tm_mday << "-"
            << setw(2) << ptm->tm_hour << "." << setw(2) << ptm->tm_min << "."
            << setw(2) << ptm->tm_sec;

  run_idbuf << "-";

  pid_t const pid = getpid();
  run_idbuf << pid;

  string const run_idstr = run_idbuf.str();
  run_id = strdup(run_idstr.c_str());

  return run_id;
}

extern "C" CCTK_POINTER_TO_CONST
Formaline_UniqueConfigID(CCTK_POINTER_TO_CONST const cctkGH_) {
  cGH const *const cctkGH = static_cast<cGH const *>(cctkGH_);
  return static_cast<CCTK_POINTER_TO_CONST>(get_config_id(cctkGH));
}

extern "C" CCTK_POINTER_TO_CONST
Formaline_UniqueBuildID(CCTK_POINTER_TO_CONST const cctkGH_) {
  cGH const *const cctkGH = static_cast<cGH const *>(cctkGH_);
  return static_cast<CCTK_POINTER_TO_CONST>(get_build_id(cctkGH));
}

extern "C" CCTK_POINTER_TO_CONST
Formaline_UniqueRunID(CCTK_POINTER_TO_CONST const cctkGH_) {
  cGH const *const cctkGH = static_cast<cGH const *>(cctkGH_);
  return static_cast<CCTK_POINTER_TO_CONST>(get_run_id(cctkGH));
}

extern "C" CCTK_POINTER_TO_CONST
Formaline_UniqueSimulationID(CCTK_POINTER_TO_CONST const cctkGH_) {
  cGH const *const cctkGH = static_cast<cGH const *>(cctkGH_);
  return static_cast<CCTK_POINTER_TO_CONST>(get_simulation_id(cctkGH));
}

extern "C" int Formaline_PrintIDs() {
  CCTK_VInfo(CCTK_THORNSTRING, "Configuration id: %s", get_config_id(0));
  CCTK_VInfo(CCTK_THORNSTRING, "Build id: %s", get_build_id(0));
  CCTK_VInfo(CCTK_THORNSTRING, "Simulation id: %s", get_simulation_id(0));
  CCTK_VInfo(CCTK_THORNSTRING, "Run id: %s", get_run_id(0));
  return 0;
}

} // namespace Formaline
