/* (C) 2001-04-18 Erik Schnetter <schnetter@uni-tuebingen.de> */
/* $Header$ */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <petscda.h>
#include <petscmat.h>
#include <petscsnes.h>

#include "cctk.h"
#include "cctk_Parameters.h"

#include "TATPETSc.h"
      
      
      
int TATPETSc_jacobian (SNES snes, Vec x, Mat *J, Mat *B,
		       MatStructure *flag, void *userptr)
{
  DECLARE_CCTK_PARAMETERS;
  int ierr;
  
#if 1
  userdata *user = (userdata*)userptr;
#else
  DMMG dmmg = (DMMG)userptr;
  userdata *user = (userdata*)dmmg->user;
#endif
  const cGH *cctkGH = user->cctkGH;
  
  if (veryverbose) CCTK_INFO ("*** TATPETSc_jacobian");
  
  assert (user);
  assert (user->magic==MAGIC);
  
  ++user->jaccall_count;
  
  TATPETSc_copy (x, userptr, TATcopyout, TATcopyvars);
  ierr = (user->jac) (cctkGH, -1, user->data);
  TATPETSc_copyjac (*J, userptr);
  
  *flag = SAME_NONZERO_PATTERN;
  
  if (veryverbose) CCTK_INFO ("*** TATPETSc_jacobian done.");
  
  return ierr;
}
