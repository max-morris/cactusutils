/* (C) 2001-04-18 Erik Schnetter <schnetter@uni-tuebingen.de> */
/* $Header$ */

#include "petsc.h"

#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"

#include "TATPETSc.h"



void TATPETSc_finalize (CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS
  DECLARE_CCTK_PARAMETERS
  int dummy;
  
  int ierr;
  
  /* Finalise PETSc */
  if (veryverbose) CCTK_INFO ("PetscFinalize");
  
  ierr = PetscFinalize ();
  CHKERRQ(ierr);
}
