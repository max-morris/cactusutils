/*@@
  @file       NaNCheck.c
   @date      Sat 21 Apr 2001
   @author    Thomas Radke
   @desc
              Routines to check CCTK real and complex variables
              against Not-a-Number values
   @enddesc
   @version   $Id$
 @@*/

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "cctk.h"
#include "cctk_WarnLevel.h"
#include "cctk_Parameters.h"
#include "cctk_Termination.h"

/* the rcsid and the macro to use it */
static const char *rcsid = "$Header$";
CCTK_FILEVERSION(CactusUtils_NaNChecker_NaNCheck_c)


/********************************************************************
 ********************    External Routines   ************************
 ********************************************************************/
int NaNChecker_NaNCheck (cGH *GH);


/********************************************************************
 ********************    Internal Routines   ************************
 ********************************************************************/
static void NaNCheck (int vindex, const char *optstring, void *arg);
static void PrintWarning (const char *error_type,
                          int linear_index,
                          int fp_type,
                          const CCTK_REAL *const coords[],
                          const char *fullname,
                          const cGroupDynamicData *gdata);


 /*@@
   @routine    NaNChecker_NaNCheck
   @author     Thomas Radke
   @date       Sat 21 Apr 2001
   @desc
               If it is time to check, loop through all variables to check
               for NaN values
   @enddesc
   @calls      CCTK_TraverseString
               NaNCheck

   @var        GH
   @vdesc      Pointer to CCTK GH
   @vtype      cGH *
   @vio        in
   @endvar

   @returntype int
   @returndesc
               0 if checking was not done during current iteration, or
               return code of @seeroutine CCTK_TraverseString otherwise
   @endreturndesc
@@*/
int NaNChecker_NaNCheck (cGH *GH)
{
  DECLARE_CCTK_PARAMETERS
  int retval;


  if (GH->cctk_iteration % check_every == 0)
  {
    retval = CCTK_TraverseString (check_vars, NaNCheck, GH, CCTK_GROUP_OR_VAR);
  }
  else
  {
    retval = 0;
  }

  return (retval);
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
   @var        linear_index
   @vdesc      processor-local linear index of the NaN/Inf
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
static void PrintWarning (const char *error_type,
                          int linear_index,
                          int fp_type,
                          const CCTK_REAL *const coords[],
                          const char *fullname,
                          const cGroupDynamicData *gdata)
{
  int i;
  char *index_string, *coord_string;
  const char *complex_part;


  if (fp_type == 2)
  {
    complex_part = linear_index & 1 ? "imag part of " : "real part of ";
    linear_index /= 2;
  }
  else
  {
    complex_part = "";
  }

  if (gdata->dim == 0)
  {
    CCTK_VWarn (1, __LINE__, __FILE__, CCTK_THORNSTRING,
                "%s caught in %svariable '%s'",
                error_type, complex_part, fullname);
  }
  else
  {
    /* assume max. 10 characters per index number and 40 characters per
       coordinate value (including separators) */
    index_string = (char *) malloc (5 * 10 * gdata->dim);
    coord_string = index_string + 10 * gdata->dim;

    sprintf (index_string, "%d",
             (linear_index % gdata->lsh[0]) + gdata->lbnd[0] + 1);
    if (coords)
    {
      sprintf (coord_string, "%5.3e", (double) coords[0][linear_index]);
    }
    for (i = 1; i < gdata->dim; i++)
    {
      linear_index /= gdata->lsh[i - 1];
      sprintf (index_string, "%s, %d", index_string,
               (linear_index % gdata->lsh[i]) + gdata->lbnd[i] + 1);
      if (coords)
      {
        sprintf (coord_string, "%s, %5.3e", coord_string,
                 (double) coords[i][linear_index]);
      }
    }

    if (coords)
    {
      CCTK_VWarn (2, __LINE__, __FILE__, CCTK_THORNSTRING,
                  "%s caught in %svariable '%s' at index (%s) with coordinates "
                  "(%s)", error_type, complex_part, fullname, index_string,
                  coord_string);
    }
    else
    {
      CCTK_VWarn (2, __LINE__, __FILE__, CCTK_THORNSTRING,
                  "%s caught in %svariable '%s' at (%s)",
                  error_type, complex_part, fullname, index_string);
    }

    free (index_string);
  }
}


 /*@@
   @routine    CHECK_DATA
   @date       Sat 21 Apr 2001
   @author     Thomas Radke
   @desc
               Macro to check a given typed array against NaN's.
               If finite(3) is available on the system it will also check
               for Inf values.
   @enddesc
   @calls      PrintWarning

   @var        cctk_type
   @vdesc      CCTK variable type of variable to check
   @vtype      <CCTK vartype>
   @vio        in
   @endvar
@@*/
#ifdef HAVE_FINITE

#define CHECK_DATA(cctk_type)                                                 \
{                                                                             \
  int _i;                                                                     \
  const cctk_type *_typed_data = (const cctk_type *) data;                    \
                                                                              \
                                                                              \
  /* now loop over all elements and check against NaN's */                    \
  for (_i = 0; _i < nelems; _i++)                                             \
  {                                                                           \
    if (! finite ((double) _typed_data[_i]))                                  \
    {                                                                         \
      nans_found++;                                                           \
      if (report_max < 0 || nans_found <= report_max)                         \
      {                                                                       \
        PrintWarning (isnan ((double) _typed_data[_i]) ? "NaN" : "Inf",       \
                      _i, fp_type, coords, fullname, &gdata);                 \
      }                                                                       \
    }                                                                         \
  }                                                                           \
}

#else

#define CHECK_DATA(cctk_type)                                                 \
{                                                                             \
  int _i;                                                                     \
  const cctk_type *_typed_data = (const cctk_type *) data;                    \
                                                                              \
                                                                              \
  /* now loop over all elements and check against NaN's */                    \
  for (_i = 0; _i < nelems; _i++)                                             \
  {                                                                           \
    if (isnan ((double) _typed_data[_i]))                                     \
    {                                                                         \
      nans_found++;                                                           \
      if (report_max < 0 || nans_found <= report_max)                         \
      {                                                                       \
        PrintWarning ("NaN", _i, fp_type, coords, fullname, &gdata);          \
      }                                                                       \
    }                                                                         \
  }                                                                           \
}

#endif /* HAVE_FINITE */


 /*@@
   @routine    NaNCheck
   @date       Sat 21 Apr 2001
   @author     Thomas Radke
   @desc
               Checks a CCTK variable given by its index against NaN's.
               It will issue a warning each time a NaN was found and also
               terminate Cactus afterwards if requested.
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
   @vdesc      optional string appended to the group/variable name
   @vtype      unused
   @vio        in
   @endvar
   @var        _GH
   @vdesc      Pointer to CCTK GH
   @vtype      void *
   @vio        in
   @endvar
@@*/
static void NaNCheck (int vindex, const char *optstring, void *_GH)
{
  DECLARE_CCTK_PARAMETERS
  const cGH *GH;
  int i, fp_type, nans_found;
  int vtype, gtype, gindex, nelems;
  char *fullname;
  const char *vtypename;
  char coord_system_name[10];
  const CCTK_REAL **coords;
  cGroupDynamicData gdata;
  const void *data;


  /* avoid compiler warning about unused parameters */
  optstring = optstring;

  GH = (cGH *) _GH;

  vtype = CCTK_VarTypeI (vindex);
  fullname = CCTK_FullName (vindex);

  /* check if the variable type is some floating point */
  vtypename = CCTK_VarTypeName (vtype);
  if (strncmp (vtypename, "CCTK_VARIABLE_REAL", 18) == 0)
  {
    fp_type = 1;
  }
  else if (strncmp (vtypename, "CCTK_VARIABLE_COMPLEX", 22) == 0)
  {
    fp_type = 2;
  }
  else
  {
    fp_type = 0;
  }

  if (fp_type)
  {
    gindex = CCTK_GroupIndexFromVarI (vindex);

    /* check if variable has storage assigned */
    if (CCTK_QueryGroupStorageI (GH, gindex))
    {
      /* get the number of elements to check for this variable */
      nelems = 1;
      gdata.dim = 0;
      coords = NULL;
      gtype = CCTK_GroupTypeI (gindex);
      if (gtype != CCTK_SCALAR)
      {
        CCTK_GroupDynamicData (GH, gindex, &gdata);
        if (gtype == CCTK_GF)
        {
          sprintf (coord_system_name, "cart%dd", gdata.dim);
          if (CCTK_CoordSystemHandle (coord_system_name) >= 0)
          {
            coords = (const CCTK_REAL **)
                     malloc (gdata.dim * sizeof (CCTK_REAL *));
          }
        }
        for (i = 0; i < gdata.dim; i++)
        {
          nelems *= gdata.lsh[i];
          if (coords)
          {
            coords[i] = (const CCTK_REAL *) CCTK_VarDataPtrI (GH, 0,
                        CCTK_CoordIndex (i + 1, NULL, coord_system_name));
            if (! coords[i]) 
            {
              free (coords);
              coords = NULL;
            }
          }
        }
      }

      /* simply double the number of elements for a CCTK_COMPLEX variable */
      if (fp_type == 2)
      {
        nelems *= 2;
      }

      /* get the pointer to the data (current time level) */
      data = CCTK_VarDataPtrI (GH, 0, vindex);

      /* do the checking according to the variable's type */
      nans_found = 0;
      if (vtype == CCTK_VARIABLE_REAL || CCTK_VARIABLE_COMPLEX)
      {
        CHECK_DATA (CCTK_REAL);
      }
#ifdef CCTK_REAL4
      else if (vtype == CCTK_VARIABLE_REAL4 || CCTK_VARIABLE_COMPLEX8)
      {
        CHECK_DATA (CCTK_REAL4);
      }
#endif
#ifdef CCTK_REAL8
      else if (vtype == CCTK_VARIABLE_REAL8 || CCTK_VARIABLE_COMPLEX16)
      {
        CHECK_DATA (CCTK_REAL8);
      }
#endif
#ifdef CCTK_REAL16
      else if (vtype == CCTK_VARIABLE_REAL16 || CCTK_VARIABLE_COMPLEX32)
      {
        CHECK_DATA (CCTK_REAL16);
      }
#endif
      else
      {
        CCTK_VWarn (0, __LINE__, __FILE__, CCTK_THORNSTRING,
                    "NanCheck: Unknown variable type '%s' for variable '%s'",
                    vtypename, fullname);
      }

      /* Do more than just print a warning ? */
      if (nans_found > 0)
      {
        if (gdata.dim > 0)
        {
          CCTK_VWarn (1, __LINE__, __FILE__, CCTK_THORNSTRING,
                      "There were %d NaN/Inf value(s) found in variable '%s'",
                      nans_found, fullname);
        }

        if (CCTK_Equals (action_if_found, "terminate"))
        {
          CCTK_TerminateNext (NULL);
        }
        else if (CCTK_Equals (action_if_found, "abort"))
        {
          CCTK_Abort (NULL, 0);
        }
      }

      if (coords)
      {
        free (coords);
      }
    }
    else
    {
      CCTK_VWarn (3, __LINE__, __FILE__, CCTK_THORNSTRING,
                  "NaNCheck: Ignoring variable '%s' (no storage)", fullname);
    }
  }
  else
  {
    CCTK_VWarn (9, __LINE__, __FILE__, CCTK_THORNSTRING,
                "NaNCheck: Ignoring variable '%s' "
                "(not a floating-point variable)", fullname);
  }

  free (fullname);
}
