#include <cctk.h>
#include <cctk_Arguments.h>
#include <cctk_Parameters.h>

void TerminationTrigger_ParamCheck(CCTK_ARGUMENTS) {
  DECLARE_CCTK_PARAMETERS;

  if (max_walltime != 0.0 && on_remaining_walltime != 0.0) {
    if (on_remaining_walltime >= max_walltime * 60.0) {
      CCTK_VWARN(CCTK_WARN_COMPLAIN,
                 "on_remaining_walltime (%g minutes) is more than max_walltime (%g h) and will be reset to 0.0 to avoid an immediate termination",
                 on_remaining_walltime, max_walltime);
      // setting on_remaining_walltime causes the parameter to be ignored until
      // it is steered by a new parameter file or thorn HTTPD
      int ierr = CCTK_ParameterSet("on_remaining_walltime", CCTK_THORNSTRING, "0.0");
      if (ierr) {
        CCTK_VWARN(CCTK_WARN_ALERT,
                   "Setting parameter "CCTK_THORNSTRING"::on_remaining_walltime failed: %d",
                   ierr);
      }
    }
  }
}
