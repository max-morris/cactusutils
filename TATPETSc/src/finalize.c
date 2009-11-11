/* (C) 2001-04-18 Erik Schnetter <schnetter@uni-tuebingen.de> */
/* $Header$ */

#include <petsc.h>

#include "cctk.h"
#include "cctk_Parameters.h"

#include "TATPETSc.h"



int TATPETSc_finalize (void)
{
  DECLARE_CCTK_PARAMETERS;
  
  int ierr;
  
  /* Finalise PETSc */
  if (veryverbose) CCTK_INFO ("PetscFinalize");
  
  ierr = PetscFinalize ();
  CHKERRQ(ierr);

  return 0;
}
