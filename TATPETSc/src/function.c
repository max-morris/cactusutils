/* (C) 2001-04-18 Erik Schnetter <schnetter@uni-tuebingen.de> */
/* $Header$ */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "petscda.h"
#include "petscsnes.h"

#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"

#include "TATPETSc.h"



int TATPETSc_function (SNES snes, Vec x, Vec f, void *userptr)
{
  DECLARE_CCTK_PARAMETERS
  int dummy;
  int ierr;
  
#if 1
  userdata *user = (userdata*)userptr;
#else
  DMMG dmmg = (DMMG)userptr;
  userdata *user = (userdata*)dmmg->user;
#endif
  const cGH *cctkGH = user->cctkGH;
  
  assert (user->magic==MAGIC);
  
  if (veryverbose) CCTK_INFO ("*** TATPETSc_function");
  
  ++user->funcall_count;
  
  TATPETSc_copy (x, userptr, TATcopyout, TATcopyvars);
  ierr = (user->bnd) (cctkGH, user->data);
  ierr = (user->fun) (cctkGH, user->data);
  TATPETSc_copy (f, userptr, TATcopyin, TATcopyvals);
  
  if (veryverbose) CCTK_INFO ("*** TATPETSc_function done.");
  
  return ierr;
}
