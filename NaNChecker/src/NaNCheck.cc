/*@@
  @file       NaNCheck.c
   @date      Sat 21 Apr 2001
   @author    Thomas Radke
   @desc
              Routines to check CCTK real and complex variables
              against Not-a-Number values.
   @enddesc
   @version   $Id$
 @@*/

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <sstream>

#include "cctk.h"
#include "cctk_WarnLevel.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"
#include "cctk_Termination.h"
#include "cctk_FortranString.h"

#include "util_String.h"

#include "NaNCheck.h"

using namespace std;

namespace NaNChecker {

/********************************************************************
 ********************    External Routines   ************************
 ********************************************************************/
extern "C" {
void NaNChecker_ResetCounter(CCTK_ARGUMENTS);
void NaNChecker_NaNCheck(CCTK_ARGUMENTS);
void NaNChecker_TakeAction(CCTK_ARGUMENTS);
}

/********************************************************************
 ********************    Fortran Wrappers    ************************
 ********************************************************************/
extern "C" {
void CCTK_FCALL CCTK_FNAME(NaNChecker_CheckVarsForNaN)(int *ierror,
                                                       const cGH **GH,
                                                       const int *report_max,
                                                       THREE_FORTSTRING_ARG);
void CCTK_FCALL CCTK_FNAME(NaNChecker_SetVarsToNaN)(int *ierror, const cGH **GH,
                                                    ONE_FORTSTRING_ARG);
}

/********************************************************************
 ********************    Internal Routines   ************************
 ********************************************************************/
void CheckForNaN(int vindex, const char *optstring, void *arg);
void SetToNaN(int vindex, const char *optstring, void *arg);
void PrintWarning(const char *error_type, int verbose, int linear_index,
                  int reflevel, int map, int cctk_iteration, int fp_type,
                  const CCTK_REAL *const coords[], const char *fullname,
                  const cGroupDynamicData *gdata);

/********************************************************************
 ********************    Internal Typedefs   ************************
 ********************************************************************/
typedef struct {
  const cGH *GH;
  int verbose;
  bool report_missing_storage;
  int report_max;
  const char *action_if_found;
  CCTK_INT count;
  CCTK_INT *NaNmask;
  int bitmask;
  bool check_for_nan;
  bool check_for_inf;
  int cctk_iteration;
  int ignore_restricted_points;
  const char *restriction_mask;
} t_nanchecker_info;

/* provide support for older Cactus flesh */
#ifndef HAVE_CCTK_DECLARED_TIMELEVELS
#define CCTK_MaxActiveTimeLevelsVI(cctkGH, vi) CCTK_MaxTimeLevelsVI(vi)
#endif

/********************************************************************
 ********************    Static Variables    ************************
 ********************************************************************/
int last_iteration_output = -1;

/*@@
  @routine    NaNChecker_ResetCounter
  @author     Thomas Radke
  @date       Tue 22 June 2004
  @desc
              Set the NaNChecker::NaNsFound counter to 0.
  @enddesc

  @var        GH
  @vdesc      Pointer to CCTK GH
  @vtype      const cGH *
  @vio        in
  @endvar
@@*/
void NaNChecker_ResetCounter(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS_NaNChecker_ResetCounter;

  *NaNsFound = 0;
}

/*@@
  @routine    NaNChecker_NaNCheck
  @author     Thomas Radke
  @date       Sat 21 Apr 2001
  @desc
              If it is time to check, loop through all variables to check
              for NaN values
  @enddesc
  @calls      CCTK_TraverseString
              CheckForNaN

  @var        GH
  @vdesc      Pointer to CCTK GH
  @vtype      const cGH *
  @vio        in
  @endvar
@@*/
t_nanchecker_info info;
extern "C" void NaNChecker_NaNCheck_Prepare(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS_NaNChecker_NaNCheck_Prepare;
  DECLARE_CCTK_PARAMETERS;
  int i, nelems;

  if (cctk_iteration < check_after ||
      check_every == 0 || cctk_iteration % check_every) {
    return;
  }

  info.GH = cctkGH;
  info.count = 0;
  info.bitmask = 0;
  info.report_max = report_max;
  info.action_if_found = action_if_found;
  info.verbose = CCTK_Equals(verbose, "all");
  /* don't report missing storage variables if we check everything since there
   * will be very many variables without storage */
  info.report_missing_storage = not CCTK_EQUALS(check_vars, "all");
  info.cctk_iteration = cctk_iteration;
  info.ignore_restricted_points = ignore_restricted_points;
  info.restriction_mask = restriction_mask;

  info.NaNmask = out_NaNmask ? NaNmask : NULL;
  if (info.NaNmask) {
    /* NaNmask can only be output once per iteration */
    if (cctk_iteration == last_iteration_output) {
      CCTK_WARN(2, "Already output NaNmask at this iteration");
      return;
    }

    /* zero out the NaN mask */
    for (i = 0, nelems = 1; i < cctk_dim; i++) {
      nelems *= cctk_ash[i];
    }
    memset(info.NaNmask, 0, nelems * sizeof(CCTK_INT));
  }

  if (CCTK_Equals(check_for, "NaN")) {
    info.check_for_nan = true;
    info.check_for_inf = false;
  } else if (CCTK_Equals(check_for, "Inf")) {
    info.check_for_nan = false;
    info.check_for_inf = true;
  } else {
    info.check_for_nan = true;
    info.check_for_inf = true;
  }
}

extern "C" void NaNChecker_NaNCheck_Check(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS_NaNChecker_NaNCheck_Check;
  DECLARE_CCTK_PARAMETERS;

  if (cctk_iteration < check_after ||
      check_every == 0 || cctk_iteration % check_every ||
      (info.NaNmask && cctk_iteration == last_iteration_output)) {
    return;
  }

  CCTK_TraverseString(check_vars, CheckForNaN, &info, CCTK_GROUP_OR_VAR);
}

extern "C" void NaNChecker_NaNCheck_Finish(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS_NaNChecker_NaNCheck_Finish;
  DECLARE_CCTK_PARAMETERS;

  if (cctk_iteration < check_after ||
      check_every == 0 || cctk_iteration % check_every ||
      (info.NaNmask && cctk_iteration == last_iteration_output)) {
    return;
  }

  /* reduce the NaN counter globally and accumulate in NaNChecker::NaNsFound */
  int sum_handle = CCTK_ReductionArrayHandle("sum");
  CCTK_INT count = 0;
  CCTK_ReduceLocalScalar(cctkGH, -1, sum_handle, &info.count, &count,
                         CCTK_VARIABLE_INT);
  *NaNsFound += count;
}

/*@@
  @routine    NaNChecker_TakeAction
  @author     Thomas Radke
  @date       Tue 22 June 2004
  @desc
              If any NaNs were found, output the NaNmask (if requested)
              and take some action (according to NaNChecker::action_if_found).
  @enddesc

  @var        GH
  @vdesc      Pointer to CCTK GH
  @vtype      const cGH *
  @vio        in
  @endvar
@@*/
extern "C" void NaNChecker_TakeAction(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS_NaNChecker_TakeAction;
  DECLARE_CCTK_PARAMETERS;

  /* if no NaNs were found then this routine doesn't do anything */
  if (!*NaNsFound) {
    return;
  }

  /* output NaN mask with the 'IOHDF5' I/O method */
  if (out_NaNmask) {
    if (CCTK_Equals(verbose, "all")) {
      CCTK_INFO("Write out NaN mask using the 'IOHDF5' I/O method");
    }
    CCTK_OutputVarAsByMethod(cctkGH, "NaNChecker::NaNmask{downsample={1 1 1}}",
                             "IOHDF5", "NaNmask");

    /* save the iteration of the last NaNmask output */
    last_iteration_output = cctk_iteration;
  }

  if (CCTK_Equals(action_if_found, "terminate")) {
    CCTK_WARN(1, "'action_if_found' parameter is set to 'terminate' - "
                 "scheduling graceful termination of Cactus");
    CCTK_TerminateNext(cctkGH);
  } else if (CCTK_Equals(action_if_found, "abort")) {
    CCTK_WARN(1, "'action_if_found' parameter is set to 'abort' - "
                 "aborting Cactus now");
    CCTK_Abort(cctkGH, 0);
  }
}

/*@@
  @routine    NaNChecker_CheckVarsForNaN
  @author     Thomas Radke
  @date       Wed 5 Dec 2001
  @desc
              User-callable routine to check for NaN values
              in CCTK grid variables.
  @enddesc
  @calls      CCTK_TraverseString
              CheckForNaN

  @var        GH
  @vdesc      Pointer to CCTK GH
  @vtype      const cGH *
  @vio        in
  @endvar
  @var        report_max
  @vdesc      How many NaN's to report
  @vtype      int
  @vio        in
  @endvar
  @var        vars
  @vdesc      Groups and/or variables to check for NaN's
  @vtype      const char *
  @vio        in
  @endvar
  @var        check_for
  @vdesc      Whether to check for NaN's and/or infinite numbers
              This is only evaluated if isinf(3) is available.
  @vtype      const char *
  @vio        in
  @endvar
  @var        action_if_found
  @vdesc      What do do if a NaN was found
              This is treated the same as the KEYWORD parameter
              'NaNChecker::action_if_found' in that it can only take certain
              values, or NULL if the routine should be quiet and just return
              the number of NaN values found.
  @vtype      const char *
  @vio        in
  @endvar

  @returntype int
  @returndesc
              the total number of NaN values found, or<BR>
              -1 if NaNChecker was already called at this iteration,<BR>
              -2 if a NULL pointer was passed for 'GH' and/or 'vars',<BR>
              -3 if an unknown keyword was passed in 'action_if_found',<BR>
              -4 if the 'vars' string couldn't be parsed
  @endreturndesc
@@*/
extern "C" int NaNChecker_CheckVarsForNaN(const cGH *GH, int report_max,
                                          const char *vars,
                                          const char *check_for,
                                          const char *action_if_found) {
  CCTK_STRING verbose =
      (CCTK_STRING)CCTK_ParameterGet("verbose", "NaNChecker", NULL);
  assert(verbose);
  const CCTK_INT *ignore_restricted_points =
      (const CCTK_INT *)CCTK_ParameterGet("ignore_restricted_points",
                                          "NaNChecker", NULL);
  assert(ignore_restricted_points);

  t_nanchecker_info info;

  if (GH == NULL || vars == NULL) {
    CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
               "NULL pointer passed for 'GH' and/or 'vars' argument");
    return (-2);
  }
#if 0
  if (cctk_iteration == last_iteration_output)
  {
    CCTK_WARN (2, "Already called NaNChecker I/O method at this iteration");
    return (-1);
  }
#endif

  if (action_if_found && (!CCTK_Equals(action_if_found, "just warn") &&
                          !CCTK_Equals(action_if_found, "terminate") &&
                          !CCTK_Equals(action_if_found, "abort"))) {
    CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
               "Unknown keyword '%s' for 'action_if_found' argument",
               action_if_found);
    return (-3);
  }

  info.GH = GH;
  info.count = 0;
  info.bitmask = 0;
  info.verbose = CCTK_Equals(verbose, "all");
  /* don't report missing storage variables if we check everything since there
   * will be very many variables without storage */
  info.report_missing_storage = not CCTK_EQUALS(vars, "all");
  info.report_max = report_max;
  info.action_if_found = action_if_found;
  info.NaNmask = NULL;
  info.cctk_iteration = GH->cctk_iteration;
  info.ignore_restricted_points = *ignore_restricted_points;

  if (CCTK_Equals(check_for, "NaN")) {
    info.check_for_nan = true;
    info.check_for_inf = false;
  } else if (CCTK_Equals(check_for, "Inf")) {
    info.check_for_nan = false;
    info.check_for_inf = true;
  } else {
    if (!CCTK_Equals(check_for, "both")) {
      CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "Invalid value '%s' passed for 'check_for' parameter. "
                 "Defaulting to 'both' instead.",
                 check_for);
    }
    info.check_for_nan = true;
    info.check_for_inf = true;
  }

  if (CCTK_TraverseString(vars, CheckForNaN, &info, CCTK_GROUP_OR_VAR) < 0) {
    CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
               "Couldn't traverse 'vars' string '%s'", vars);
    return (-4);
  }

#if 0
  /* save the iteration of the last call */
  last_iteration_output = GH->cctk_iteration;
#endif

  if (info.count > 0 && info.action_if_found) {
    if (CCTK_Equals(info.action_if_found, "terminate")) {
      CCTK_WARN(1, "'action_if_found' parameter is set to 'terminate' - "
                   "scheduling graceful termination of Cactus");
      CCTK_TerminateNext(NULL);
    } else if (info.action_if_found &&
               CCTK_Equals(info.action_if_found, "abort")) {
      CCTK_WARN(1, "'action_if_found' parameter is set to 'abort' - "
                   "aborting Cactus now");
      CCTK_Abort(NULL, 0);
    }
  }

  return (info.count);
}

extern "C" void CCTK_FCALL
    CCTK_FNAME(NaNChecker_CheckVarsForNaN)(int *ierror, const cGH **GH,
                                           const int *report_max,
                                           THREE_FORTSTRING_ARG) {
  THREE_FORTSTRING_CREATE(vars, check_for, action_if_found);
  *ierror =
      NaNChecker_CheckVarsForNaN(*GH, *report_max, vars, check_for,
                                 *action_if_found ? action_if_found : NULL);
  free(vars);
  free(check_for);
  free(action_if_found);
}

/*@@
  @routine    NaNChecker_SetVarsToNaN
  @author     Thomas Radke
  @date       Sun 9 Dec 2001
  @desc
              User-callable routine to set CCTK grid variables to NaN values.
  @enddesc
  @calls      CCTK_TraverseString
              SetToNaN

  @var        GH
  @vdesc      Pointer to CCTK GH
  @vtype      const cGH *
  @vio        in
  @endvar
  @var        vars
  @vdesc      Groups and/or variables to set to NaN's
  @vtype      const char *
  @vio        in
  @endvar

  @returntype int
  @returndesc
               0 for success, or<BR>
              -1 if a NULL pointer was passed for 'GH' and/or 'vars',<BR>
              -2 if the 'vars' string couldn't be parsed
  @endreturndesc
@@*/
extern "C" int NaNChecker_SetVarsToNaN(const cGH *GH, const char *vars) {
  t_nanchecker_info info;

  if (GH == NULL || vars == NULL) {
    CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
               "NULL pointer passed for 'GH' and/or 'vars' argument");
    return (-1);
  }

  info.GH = GH;
  info.count = 0;
  if (CCTK_TraverseString(vars, SetToNaN, &info, CCTK_GROUP_OR_VAR) < 0) {
    CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
               "Couldn't traverse 'vars' string '%s'", vars);
    return (-2);
  }

  return (info.count);
}

extern "C" void CCTK_FCALL
    CCTK_FNAME(NaNChecker_SetVarsToNaN)(int *ierror, const cGH **GH,
                                        ONE_FORTSTRING_ARG) {
  ONE_FORTSTRING_CREATE(vars);
  *ierror = NaNChecker_SetVarsToNaN(*GH, vars);
  free(vars);
}

extern "C" void NaNChecker_SetupTest(CCTK_ARGUMENTS) {
  DECLARE_CCTK_PARAMETERS;
  DECLARE_CCTK_ARGUMENTS_NaNChecker_SetupTest;

  NaNChecker_SetVarsToNaN(cctkGH, CCTK_THORNSTRING "::TestGF");
}

/********************************************************************
 ********************    Internal Routines   ************************
 ********************************************************************/

/*@@
  @routine    PrintWarning
  @author     Thomas Radke
  @date       Sat 21 Apr 2001
  @desc
              Prints a warning for a Inf/NaN found in a variable
              at the given processor-local linear index.
              The warning includes the variable's fullname along with
              the global index of the NaN element in fortran order.
              If coordinates are available, the NaN's location on the grid
              is also output.
  @enddesc
  @calls      CCTK_VWarn

  @var        error_type
  @vdesc      string containing the error value found (NaN or Inf)
  @vtype      const char *
  @vio        in
  @endvar
  @var        verbose
  @vdesc      how much information to give
  @vtype      int
  @vio        in
  @endvar
  @var        linear_index
  @vdesc      processor-local linear index of the NaN/Inf
  @vtype      int
  @vio        in
  @endvar
  @var        reflevel
  @vdesc      current refinement level (0 for unigrid)
  @vtype      int
  @vio        in
  @endvar
  @var        map
  @vdesc      current map (0 for unipatch)
  @vtype      int
  @vio        in
  @endvar
  @var        cctk_iteration
  @vdesc      current iteration
  @vtype      int
  @vio        in
  @endvar
  @var        fp_type
  @vdesc      indicates if variable of of real or complex type
  @vtype      int
  @vio        in
  @endvar
  @var        fullname
  @vdesc      full name of the variable
  @vtype      const char *
  @vio        in
  @endvar
  @var        coords
  @vdesc      array of coordinates
  @vtype      const CCTK_REAL *const [ dimension of variable ]
  @vio        in
  @endvar
  @var        gdata
  @vdesc      size information on the variable to compute the global index
  @vtype      const cGroupDynamicData *
  @vio        in
  @endvar
@@*/
void PrintWarning(const char *error_type, int verbose, int linear_index,
                  int reflevel, int map, int cctk_iteration, int fp_type,
                  const CCTK_REAL *const coords[], const char *fullname,
                  const cGroupDynamicData *gdata) {
  const char *complex_part;

  if (fp_type == 2) {
    complex_part = linear_index & 1 ? "imag part of " : "real part of ";
    linear_index /= 2;
  } else {
    complex_part = "";
  }

  if (gdata->dim == 0) {
    CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
               "%s caught in %svariable '%s'", error_type, complex_part,
               fullname);
  } else if (verbose) {
    ostringstream index_buf, coord_buf;
    int amended_index = linear_index;
    for (int i = 0; i < gdata->dim; i++) {
      if (i > 0) {
        index_buf << ", ";
        if (coords) {
          coord_buf << ", ";
        }
      }
      index_buf << (amended_index % gdata->ash[i]) + gdata->lbnd[i] + 1;
      if (coords) {
        coord_buf << /* "%5.3e" */ coords[i][linear_index];
      }
      amended_index /= gdata->ash[i];
    }

    if (coords) {
      CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "%s caught in %svariable '%s' at index (%s) level %d map %d "
                 "with coordinates "
                 "(%s) in iteration %d",
                 error_type, complex_part, fullname, index_buf.str().c_str(),
                 reflevel, map, coord_buf.str().c_str(), cctk_iteration);
    } else {
      CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "%s caught in %svariable '%s' at (%s) level %d map %d in "
                 "iteration %d",
                 error_type, complex_part, fullname, index_buf.str().c_str(),
                 reflevel, map, cctk_iteration);
    }
  }
}

/*@@
  @routine    CHECK_DATA
  @date       Sat 21 Apr 2001
  @author     Thomas Radke
  @desc
              Macro to check a given typed array against NaN's.
              If isinf(3) is available on the system it will also check
              for Inf values.
  @enddesc
  @calls      PrintWarning

  @var        cctk_type
  @vdesc      CCTK variable type of variable to check
  @vtype      <CCTK vartype>
  @vio        in
  @endvar
@@*/
template <class cctk_type>
void CHECK_DATA(const cctk_type *_data, int nelems,
                const CCTK_REAL *CarpetWeights, const t_nanchecker_info *info,
                CCTK_INT &nans_found, int reflevel, int map,
                CCTK_INT cctk_iteration, int fp_type,
                const CCTK_REAL *const *coords, const char *fullname,
                const cGroupDynamicData &gdata, CCTK_INT gtype) {
/* now loop over all elements and check against NaN's */
#pragma omp parallel for schedule(dynamic)
  for (int _i = 0; _i < nelems; _i++) {
    if (!CarpetWeights || CarpetWeights[_i] > 0.0) {

      /* We call the C wrapper functions instead of the autoconfigured
         macros below, because some C++ compilers optimise away calls
         to isnan depending on the optimisation level, while C
         compilers do not appear to do this. */
      bool found_inf = false, found_nan = false;
      found_inf = CCTK_isinf(_data[_i]);
      found_nan = CCTK_isnan(_data[_i]);
      bool found_problem = (info->check_for_nan && found_nan) ||
                           (info->check_for_inf && found_inf);
      if (found_problem)
#pragma omp critical
      {
        // checking whether we are inside is somewhat expensive due to the
        // integer divisions, so doing it only for found issues reduces the
        // number of divisions from nelems to something like
        // gdata.ash-gdata.lsh if all "outside" points are NaN
        bool is_inside = true;
        int amended_index = _i;
        if (fp_type == 2)
          amended_index /= 2;
        for (int d = 0; d < gdata.dim; ++d) {
          int dir_index = amended_index % gdata.ash[d];
          is_inside &= dir_index < gdata.lsh[d];
          amended_index /= gdata.ash[d];
        }
        assert(amended_index == 0);
        if (is_inside) {
          nans_found++;
          if (info->action_if_found &&
              (info->report_max < 0 || nans_found <= info->report_max)) {
            PrintWarning(found_nan ? "NaN" : "Inf", info->verbose, _i, reflevel,
                         map, cctk_iteration, fp_type, coords, fullname, &gdata);
          }
          if (info->NaNmask && gtype == CCTK_GF) {
            info->NaNmask[_i] |= 1 << info->bitmask;
          }
        }
      }
    }
  }
}

/*@@
  @routine    CheckForNaN
  @date       Sat 21 Apr 2001
  @author     Thomas Radke
  @desc
              Checks a CCTK variable given by its index against NaN's.
              If an 'action_if_found' was given it will issue a warning
              each time a NaN was found and also terminate Cactus afterwards
              if requested.
              Note that only floating point typed variables are checked.
              <BR>
              This routine is called as a callback via CCTK_TraverseString().
  @enddesc
  @calls      CHECK_DATA

  @var        vindex
  @vdesc      index of variable to check
  @vtype      int
  @vio        in
  @endvar
  @var        optstring
  @vdesc      optional timelevel string appended to the group/variable name
  @vtype      const char *
  @vio        in
  @endvar
  @var        _info
  @vdesc      Pointer to NaNChecker info structure
  @vtype      void *
  @vio        in
  @endvar
@@*/
void CheckForNaN(int vindex, const char *optstring, void *_info) {
  t_nanchecker_info *info;
  int i, sum_handle;
  CCTK_INT nans_found, global_nans_found;
  int timelevel, fp_type, vtype, gtype, gindex, nelems;
  char *fullname, *endptr;
  const char *vtypename;
  char coord_system_name[10];
  const CCTK_REAL **coords;
  cGroupDynamicData gdata;
  const void *data;
  CCTK_REAL *CarpetWeights;
  int reflevel, map;
  int cctk_iteration, ignore_restricted_points;
  bool report_missing_storage;
  const char *restriction_mask;

  info = (t_nanchecker_info *)_info;
  cctk_iteration = info->cctk_iteration;
  report_missing_storage = info->report_missing_storage;
  ignore_restricted_points = info->ignore_restricted_points;
  restriction_mask = info->restriction_mask;

  vtype = CCTK_VarTypeI(vindex);
  fullname = CCTK_FullName(vindex);
  if (CCTK_IsFunctionAliased("GetRefinementLevel")) {
    reflevel = GetRefinementLevel(info->GH);
  } else {
    reflevel = 0;
  }
  if (CCTK_IsFunctionAliased("GetMap")) {
    map = GetMap(info->GH);
  } else {
    map = 0;
  }

  /* check if the variable type is some floating point */
  vtypename = CCTK_VarTypeName(vtype);
  if (strncmp(vtypename, "CCTK_VARIABLE_REAL", 18) == 0) {
    fp_type = nelems = 1;
  } else if (strncmp(vtypename, "CCTK_VARIABLE_COMPLEX", 22) == 0) {
    fp_type = nelems = 2;
  } else {
    CCTK_VWarn(9, __LINE__, __FILE__, CCTK_THORNSTRING,
               "CheckForNaN: Ignoring variable '%s' "
               "(not a floating-point variable)",
               fullname);
    free(fullname);
    return;
  }

  /* check if variable has storage assigned */
  gindex = CCTK_GroupIndexFromVarI(vindex);
  if (CCTK_QueryGroupStorageI(info->GH, gindex) <= 0) {
    if(report_missing_storage) {
      CCTK_VWarn(3, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "CheckForNaN: Ignoring variable '%s' (no storage)", fullname);
    }
    free(fullname);
    return;
  }

  /* get the timelevel to check */
  if (optstring) {
    if (strncmp(optstring, "timelevel=", 10)) {
      CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "CheckForNaN: Invalid option string '%s' given for variable "
                 "'%s'",
                 optstring, fullname);
      free(fullname);
      return;
    } else {
      timelevel = strtol(optstring + 10, &endptr, 10);
      if (*endptr != ']' || timelevel < 0 ||
          timelevel >= CCTK_MaxActiveTimeLevelsVI(info->GH, vindex)) {
        CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
                   "CheckForNaN: Invalid timelevel '%s' given for variable "
                   "'%s'",
                   optstring + 10, fullname);
        free(fullname);
        return;
      }
    }
  } else {
    /* default timelevel */
    timelevel = 0;
  }

  /* get the number of elements to check for this variable */
  gdata.dim = 0;
  coords = NULL;
  gtype = CCTK_GroupTypeI(gindex);
  if (gtype != CCTK_SCALAR) {
    CCTK_GroupDynamicData(info->GH, gindex, &gdata);
    if (gtype == CCTK_GF) {
      sprintf(coord_system_name, "cart%dd", gdata.dim);
      if (CCTK_CoordSystemHandle(coord_system_name) >= 0) {
        coords =
            (const CCTK_REAL **)malloc(gdata.dim * sizeof(const CCTK_REAL *));
      }
    }
    for (i = 0; i < gdata.dim; i++) {
      nelems *= gdata.lsh[i];
      if (coords) {
        coords[i] = (const CCTK_REAL *)CCTK_VarDataPtrI(
            info->GH, 0, CCTK_CoordIndex(i + 1, NULL, coord_system_name));
        if (!coords[i]) {
          free(coords);
          coords = NULL;
        }
      }
    }
  }

  /* get the pointer to the data (current time level) */
  data = CCTK_VarDataPtrI(info->GH, timelevel, vindex);
  if (ignore_restricted_points && gtype == CCTK_GF) {
    CarpetWeights =
        (CCTK_REAL *)(CCTK_VarDataPtr(info->GH, timelevel, restriction_mask));
    if (NULL == CarpetWeights) {
      static char *warned_about = NULL;
#pragma omp critical
      if (NULL == warned_about || strcmp(warned_about, restriction_mask) != 0) {
        if (NULL != warned_about)
          free(warned_about);
        warned_about = Util_Strdup(restriction_mask);
        CCTK_VWarn(CCTK_WARN_ALERT, __LINE__, __FILE__, CCTK_THORNSTRING,
                   "Could not obtain pointer to restriction mask '%s', "
                   "reverting to check all points",
                   restriction_mask);
      }
    }
  } else {
    CarpetWeights = NULL;
  }

  /* do the checking according to the variable's type */
  nans_found = 0;
  if (vtype == CCTK_VARIABLE_REAL || vtype == CCTK_VARIABLE_COMPLEX) {
    CHECK_DATA((const CCTK_REAL *)data, nelems, CarpetWeights, info, nans_found,
               reflevel, map, cctk_iteration, fp_type, coords, fullname, gdata,
               gtype);
  }
#ifdef HAVE_CCTK_REAL4
  else if (vtype == CCTK_VARIABLE_REAL4 || vtype == CCTK_VARIABLE_COMPLEX8) {
    CHECK_DATA((const CCTK_REAL4 *)data, nelems, CarpetWeights, info,
               nans_found, reflevel, map, cctk_iteration, fp_type, coords,
               fullname, gdata, gtype);
  }
#endif
#ifdef HAVE_CCTK_REAL8
  else if (vtype == CCTK_VARIABLE_REAL8 || vtype == CCTK_VARIABLE_COMPLEX16) {
    CHECK_DATA((const CCTK_REAL8 *)data, nelems, CarpetWeights, info,
               nans_found, reflevel, map, cctk_iteration, fp_type, coords,
               fullname, gdata, gtype);
  }
#endif
#ifdef HAVE_CCTK_REAL16
  else if (vtype == CCTK_VARIABLE_REAL16 || vtype == CCTK_VARIABLE_COMPLEX32) {
    CHECK_DATA((const CCTK_REAL16 *)data, nelems, CarpetWeights, info,
               nans_found, reflevel, map, cctk_iteration, fp_type, coords,
               fullname, gdata, gtype);
  }
#endif
  else {
    CCTK_VWarn(0, __LINE__, __FILE__, CCTK_THORNSTRING,
               "NanCheck: Unknown variable type '%s' for variable '%s'",
               vtypename, fullname);
  }

  /* Do more than just print a warning ? */
  if (nans_found > 0 && info->action_if_found && gdata.dim > 0) {
    if (info->NaNmask && gtype == CCTK_GF &&
        info->bitmask < int(8 * sizeof(CCTK_INT))) {
      CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "There were %d NaN/Inf value(s) found in variable '%s' "
                 "(NaNmask bitfield %d)",
                 (int)nans_found, fullname, info->bitmask);
    } else {
      CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "There were %d NaN/Inf value(s) found in variable '%s'",
                 (int)nans_found, fullname);
    }
  }
  /* check if the (global) bitmask needs to be incremented */
  if (info->NaNmask && gtype == CCTK_GF &&
      info->bitmask < int(8 * sizeof(CCTK_INT))) {
    sum_handle = CCTK_ReductionArrayHandle("sum");
    CCTK_ReduceLocalScalar(info->GH, -1, sum_handle, &nans_found,
                           &global_nans_found, CCTK_VARIABLE_INT);
    if (global_nans_found) {
      info->bitmask++;
    }
  }
  info->count += nans_found;

  if (coords) {
    free(coords);
  }
  free(fullname);
}

/*@@
  @routine    SetToNaN
  @date       Sun 9 Dec 2001
  @author     Thomas Radke
  @desc
              Initializes a CCTK variable given by its index to all NaN's.
              Note that only floating point typed variables are initialized.
              <BR>
              This routine is called as a callback via CCTK_TraverseString().
  @enddesc

  @var        vindex
  @vdesc      index of variable to initialize
  @vtype      int
  @vio        in
  @endvar
  @var        optstring
  @vdesc      optional timelevel string appended to the group/variable name
  @vtype      const char *
  @vio        in
  @endvar
  @var        _info
  @vdesc      Pointer to NaNChecker info structure
  @vtype      void *
  @vio        in
  @endvar
@@*/
void SetToNaN(int vindex, const char *optstring, void *_info) {
  t_nanchecker_info *info;
  int i, timelevel, vtype, gindex, nelems;
  const char *vtypename;
  char *fullname, *endptr;
  cGroupDynamicData gdata;

  info = (t_nanchecker_info *)_info;

  vtype = CCTK_VarTypeI(vindex);
  fullname = CCTK_FullName(vindex);

  /* check if the variable type is some floating point */
  vtypename = CCTK_VarTypeName(vtype);
  if (strncmp(vtypename, "CCTK_VARIABLE_REAL", 18) == 0) {
    nelems = 1;
  } else if (strncmp(vtypename, "CCTK_VARIABLE_COMPLEX", 22) == 0) {
    nelems = 2;
  } else {
    CCTK_VWarn(9, __LINE__, __FILE__, CCTK_THORNSTRING,
               "SetToNaN: Ignoring variable '%s' "
               "(not a floating-point variable)",
               fullname);
    free(fullname);
    return;
  }

  /* check if variable has storage assigned */
  gindex = CCTK_GroupIndexFromVarI(vindex);
  if (CCTK_QueryGroupStorageI(info->GH, gindex) <= 0) {
    CCTK_VWarn(3, __LINE__, __FILE__, CCTK_THORNSTRING,
               "SetToNaN: Ignoring variable '%s' (no storage)", fullname);
    free(fullname);
    return;
  }

  /* get the timelevel to initialize */
  if (optstring) {
    if (strncmp(optstring, "timelevel=", 10)) {
      CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "SetToNaN: Invalid option string '%s' given for variable "
                 "'%s'",
                 optstring, fullname);
      free(fullname);
      return;
    } else {
      timelevel = strtol(optstring + 10, &endptr, 10);
      if (timelevel < 0 ||
          timelevel >= CCTK_MaxActiveTimeLevelsVI(info->GH, vindex)) {
        CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
                   "SetToNaN: Invalid timelevel '%s' given for variable "
                   "'%s'",
                   optstring + 10, fullname);
        free(fullname);
        return;
      }
    }
  } else {
    /* default timelevel */
    timelevel = 0;
  }

  /* get the number of elements to initialize for this variable */
  if (CCTK_GroupTypeI(gindex) != CCTK_SCALAR) {
    CCTK_GroupDynamicData(info->GH, gindex, &gdata);
    for (i = 0; i < gdata.dim; i++) {
      nelems *= gdata.ash[i];
    }
  }

  /* do the initialization */
  memset(CCTK_VarDataPtrI(info->GH, timelevel, vindex), -1,
         CCTK_VarTypeSize(vtype) * nelems);

  /* count up the total number of variables initialized */
  info->count++;

  /* clean up */
  free(fullname);
}

/********************************************************************
 ********************    Schedule Wrappers    ***********************
 ********************************************************************/

extern "C" CCTK_INT NaNChecker_CheckVarsForNaN_Wrapper(
    CCTK_POINTER_TO_CONST const cctkGH_, CCTK_INT const report_max,
    CCTK_STRING const vars, CCTK_STRING const check_for,
    CCTK_STRING const action_if_found) {
  return NaNChecker_CheckVarsForNaN((cGH const *)cctkGH_, report_max, vars,
                                    check_for, action_if_found);
}

extern "C" CCTK_INT
NaNChecker_SetVarsToNaN_Wrapper(CCTK_POINTER_TO_CONST const cctkGH_,
                                CCTK_STRING const vars) {
  return NaNChecker_SetVarsToNaN((cGH const *)cctkGH_, vars);
}

} // end namespace NaNChecker
