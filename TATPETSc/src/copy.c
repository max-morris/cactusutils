/* (C) 2001-04-18 Erik Schnetter <schnetter@uni-tuebingen.de> */
/* $Header$ */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpi.h"

#include "petscda.h"

#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"

#include "TATPETSc.h"



static const char *rcsid = "$Header$";
      
      
      
int TATPETSc_copy (Vec x, void *userptr, TATdir dir, TATvarset varset)
{
  DECLARE_CCTK_PARAMETERS;
  
  userdata *user = (userdata*)userptr;
  cGH *cctkGH = user->cctkGH;
  
  const int nvars = user->nvars;
  
  /* Address of PETSc vector */
  double *xx;
  
  /* Cactus variables */
  CCTK_REAL **var;
  
  /* PETSc grid extent */
  int XM[DIM];			/* global extent */
  int xs[DIM], xm[DIM];		/* offset, local extent */
  
  int ylow, yhigh;		/* local ownership of Cactus vector */
  
  /* Cactus grid extent */
  int NI[DIM];			/* global extent */
  int ni[DIM];			/* local extent including ghost zones */
  int gi[DIM];			/* number of ghost zones */
  int i0[DIM];			/* local-to-global offset */
  int bi[DIM], ei[DIM];		/* lower and upper number of ghost zones */
  int mi[DIM];			/* local extent without ghost zones */
  
  /* Intermediate storage (same size as Cactus) */
  int i1[DIM];			/* local-to-global offset */
  
  int i,j,k;
  int n;
  int d;
  int ind;
  
  int ierr;
  
  assert (user->magic==MAGIC);
  
  if (veryverbose) CCTK_INFO ("*** TATPETSc_copy");
  
  assert (dir==TATcopyin || dir==TATcopyout);
  assert (varset==TATcopyvars || varset==TATcopyvals);
  
  if (veryverbose) {
    CCTK_VInfo (CCTK_THORNSTRING, "copy %s %s PETSc",
		varset==TATcopyvars ? "variables" : "function values",
		dir==TATcopyin ? "into" : "out of");
  }
  
  
  
  assert (user);
  assert (user->nvars >= 0);
  assert (user->var);
  assert (user->val);
  
  
  
  /* Get grid boundaries */
  
  for (d=0; d<user->dyndata.dim; ++d) {
    
    /* Local Cactus boundaries */
    ni[d] = user->dyndata.lsh[d];
    i0[d] = user->dyndata.lbnd[d];
    gi[d] = user->dyndata.nghostzones[d];
    bi[d] = user->dyndata.bbox[2*d  ] && user->solvebnds[2*d  ] ? 0 : gi[d];
    ei[d] = user->dyndata.bbox[2*d+1] && user->solvebnds[2*d+1] ? 0 : gi[d];
    mi[d] = ni[d]-bi[d]-ei[d];
    
    /* intermediate storage */
    i1[d] = i0[d] + (!user->dyndata.bbox[2*d] && user->solvebnds[2*d]) * gi[d];
    assert (i1[d]>=0);
    
    /* Global Cactus boundaries */
    NI[d] = (user->dyndata.gsh[d]
	     - (!user->solvebnds[2*d] + !user->solvebnds[2*d+1]) * gi[d]);
  }
  
  for (d=user->dyndata.dim; d<DIM; ++d) {
    
    /* Local Cactus boundaries */
    ni[d] = 1;
    i0[d] = 0;
    gi[d] = 0;
    bi[d] = 0;
    ei[d] = 0;
    mi[d] = ni[d]-bi[d]-ei[d];
    
    /* intermediate storage */
    i1[d] = i0[d];
    
    /* Global Cactus boundaries */
    NI[d] = 1;
  }
  
  /* Local PETSc boundaries */
  ierr = DAGetCorners (user->da, &xs[0],&xs[1],&xs[2], &xm[0],&xm[1],&xm[2]);
  CHKERRQ(ierr);
  
  /* Global PETSc boundaries */
  ierr = DAGetInfo (user->da,
		    PETSC_NULL,
		    &XM[0],&XM[1],&XM[2],
		    PETSC_NULL,PETSC_NULL,PETSC_NULL,
		    PETSC_NULL,PETSC_NULL,
		    PETSC_NULL,PETSC_NULL);
  CHKERRQ(ierr);
  
  if (veryverbose) {
    CCTK_VInfo (CCTK_THORNSTRING,
		"Dimension: dim=%d", user->dyndata.dim);
    CCTK_VInfo (CCTK_THORNSTRING,
		"Global Cactus shape: extent=[%d,%d,%d]",
		NI[0],NI[1],NI[2]);
    CCTK_VInfo (CCTK_THORNSTRING,
		"Global PETSc shape: extent=[%d,%d,%d]",
		XM[0],XM[1],XM[2]);
    CCTK_VInfo (CCTK_THORNSTRING,
		"Local Cactus shape: start=[%d,%d,%d], extent=[%d,%d,%d]",
		i0[0],i0[1],i0[2], ni[0],ni[1],ni[2]);
    CCTK_VInfo (CCTK_THORNSTRING,
		"Local PETSc shape: start=[%d,%d,%d], extent=[%d,%d,%d]",
		xs[0],xs[1],xs[2], xm[0],xm[1],xm[2]);
  }
  
  /* Global shapes must match */
  for (d=0; d<user->dyndata.dim; ++d) {
    assert (XM[d] == NI[d]);
  }
  
  /* Local shapes must match */
  for (d=0; d<user->dyndata.dim; ++d) {
    assert (xs[d] == i0[d]);
    assert (xm[d] == mi[d]);
  }
  
  
  
  /* Get local PETSc vector */
  if (veryverbose) CCTK_INFO ("VecGetArray");
  ierr = VecGetArray (x, &xx);
  CHKERRQ(ierr);
  
  
  
  /* Get Cactus variable pointers */
  var = malloc(sizeof(*var) * nvars);
  assert (var);
  for (n=0; n<nvars; ++n) {
    switch (varset) {
    case TATcopyvars:
      /* variables */
      var[n] = CCTK_VarDataPtrI(user->cctkGH, 0, user->var[n]);
      assert (var[n]);
      break;
    case TATcopyvals:
      /* values */
      var[n] = CCTK_VarDataPtrI(user->cctkGH, 0, user->val[n]);
      assert (var[n]);
      break;
    default:
      abort();
    }
  }
  
  
  
  switch (dir) {
    
  case TATcopyin:
    /* copy in */
    
    /* Copy Cactus variable into PETSc variable */
    for (k=0; k<mi[2]; ++k) {
      for (j=0; j<mi[1]; ++j) {
	for (i=0; i<mi[0]; ++i) {
	  for (n=0; n<nvars; ++n) {
	    ind = bi[0]+i + ni[0]*(bi[1]+j + ni[1]*(bi[2]+k));
	    xx[n+nvars*(i+mi[0]*(j+mi[1]*k))] = var[n][ind];
	  }
	}
      }
    }
    
    break;
    
  case TATcopyout:
    /* copy out */
    
    /* Copy Cactus variable */
    for (k=0; k<mi[2]; ++k) {
      for (j=0; j<mi[1]; ++j) {
	for (i=0; i<mi[0]; ++i) {
	  for (n=0; n<nvars; ++n) {
	    ind = bi[0]+i + ni[0]*(bi[1]+j + ni[1]*(bi[2]+k));
	    var[n][ind] = xx[n+nvars*(i+mi[0]*(j+mi[1]*k))];
	  }
	}
      }
    }
    
    /* This loop possibly does too much work: it might synchronise
       groups several times */
    for (n=0; n<user->nvars; ++n) {
      CCTK_SyncGroupWithVarI (cctkGH, user->var[n]);
    }
    
    break;
    
  default:
    abort();
  }
  
  
  
  /* Clean up */
  if (veryverbose) CCTK_INFO ("Destroy");
  ierr = VecRestoreArray (x, &xx)
  CHKERRQ(ierr);
  
  free (var);
  var = 0;
  
  
  
  if (veryverbose) CCTK_INFO ("*** TATPETSc_copy done.");
  
  return 0;
}
