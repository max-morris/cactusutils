/* (C) 2001-04-18 Erik Schnetter <schnetter@uni-tuebingen.de> */
/* $Header$ */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>
#include <unistd.h>

#include "mpi.h"

#include "petscda.h"
#include "petscsnes.h"

#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"

#include "util_ErrorCodes.h"
#include "util_Table.h"

#include "Symmetry.h"

#include "TATPETSc.h"



int TATPETSc_solve (cGH *cctkGH,
		    const int *var, const int *val, int nvars,
		    int options_table,
		    int (*fun) (cGH *cctkGH, int options_table, void *data),
		    int (*bnd) (cGH *cctkGH, int options_table, void *data),
		    void *data)
{
  DECLARE_CCTK_PARAMETERS;
  
  CCTK_INT periodic[DIM];
  CCTK_INT solvebnds[2*DIM];
  CCTK_INT sw;
  CCTK_FN_POINTER ptmp;
  int (*jac) (const cGH *cctkGH, Mat *J, Mat *B,
	      MatStructure *flag, void *data);
  int (*get_coloring)(DA da, ISColoring *iscoloring,
		      Mat *J, void *data);
  
  /* world communicator */
  MPI_Comm comm;
  
  /* multigrid contest */
  DMMG *dmmg;
  
  /* nonlinear solver context */
  SNES snes;
  SNESConvergedReason reason;
  
  /* distributed array */
  DA da;
  
  /* matrix coloring */
  ISColoring iscoloring;
  MatFDColoring matfdcoloring;
  
  /* vectors */
  Vec x;			/* solution */
  Vec f;			/* function value */
  
  /* matrix */
  int NI[DIM];			/* number of grid points */
  int *lx[DIM];			/* local number of grid points */
  Mat J;			/* Jacobian */
  
  /* application data */
  userdata user;
  
  /* variable data */
  int group;
  cGroupDynamicData dyndata;
  
  int dim;			/* number of dimensions */
  int nprocs;			/* number of processors */
  
  const int *lbnd, *lsh;	/* [dim]: local lbnd and lsh */
  
  /* lbnd and lsh of all processors */
  int *all_lbnd, *all_lsh;	/* [nprocs][dim] as [nprocs*dim + dim] */
  
  /* number of processors per direction */
  int nprocs_dim[DIM];		/* [dim] */
  
  /* product_prefix of nprocs_dim */
  int proc_offset_dim[DIM];	/* [dim] */
  
  /* total number of grid points per direction */
  int npoints[DIM];		/* [dim] */
  
  /* number of grid points per direction and per processor */
  int *npoints_proc[DIM];	/* [dim][nprocs_dim[dim]] */
  
  int iters;			/* iterations needed */
  int liniters;			/* linear iterations needed */
  double residual;		/* L2-norm of residual */
  
  int n;			/* variable or processor counter */
  int d;			/* dimension counter */
  
  long clk_tck;
  struct tms tms_buffer;
  clock_t ticks0, ticks1, time0, time1;
  
  const char *msg;
  
  int ierr;
  
  
  
  if (! CCTK_IsThornActive(CCTK_THORNSTRING)) {
    CCTK_WARN (0, "Thorn " CCTK_THORNSTRING " has not been activated.  It is therefore not possible to call TATPETSc_solve.");
  }
  
  
  
  if (veryverbose) CCTK_INFO ("*** TATPETSc_solve");
  
  
  
  user.magic = MAGIC;
  user.cctkGH = cctkGH;
  
  comm = PETSC_COMM_WORLD;
  user.comm = comm;
  
  
  
  /* Check and copy arguments */
  
  assert (nvars>0);
  assert (var);
  assert (val);
  user.nvars = nvars;
  user.var = malloc(sizeof(*user.var) * user.nvars);
  assert (user.var);
  user.val = malloc(sizeof(*user.val) * user.nvars);
  assert (user.val);
  for (n=0; n<nvars; ++n) {
    assert (var[n]>=0 && var[n]<CCTK_NumVars());
    user.var[n] = var[n];
    assert (val[n]>=0 && val[n]<CCTK_NumVars());
    user.val[n] = val[n];
  }
  
  group = CCTK_GroupIndexFromVarI (user.var[0]);
  assert (group>=0);
  CCTK_GroupDynamicData (cctkGH, group, &user.dyndata);
  
  assert (user.dyndata.dim <= DIM);
  
  /* Check grid variable properties */
  for (n=0; n<nvars; ++n) {
    group = CCTK_GroupIndexFromVarI (user.var[n]);
    assert (group>=0);
    CCTK_GroupDynamicData (cctkGH, group, &dyndata);
    assert (dyndata.dim == user.dyndata.dim);
    for (d=0; d<user.dyndata.dim; ++d) {
      assert (dyndata.dim == user.dyndata.dim);
      assert (dyndata.gsh[d] == user.dyndata.gsh[d]);
      assert (dyndata.lsh[d] == user.dyndata.lsh[d]);
      assert (dyndata.lbnd[d] == user.dyndata.lbnd[d]);
      assert (dyndata.ubnd[d] == user.dyndata.ubnd[d]);
      assert (dyndata.bbox[2*d] == user.dyndata.bbox[2*d]);
      assert (dyndata.bbox[2*d+1] == user.dyndata.bbox[2*d+1]);
      assert (dyndata.nghostzones[d] == user.dyndata.nghostzones[d]);
    }
    group = CCTK_GroupIndexFromVarI (user.val[n]);
    assert (group>=0);
    CCTK_GroupDynamicData (cctkGH, group, &dyndata);
    assert (dyndata.dim == user.dyndata.dim);
    for (d=0; d<user.dyndata.dim; ++d) {
      assert (dyndata.dim == user.dyndata.dim);
      assert (dyndata.gsh[d] == user.dyndata.gsh[d]);
      assert (dyndata.lsh[d] == user.dyndata.lsh[d]);
      assert (dyndata.lbnd[d] == user.dyndata.lbnd[d]);
      assert (dyndata.ubnd[d] == user.dyndata.ubnd[d]);
      assert (dyndata.bbox[2*d] == user.dyndata.bbox[2*d]);
      assert (dyndata.bbox[2*d+1] == user.dyndata.bbox[2*d+1]);
      assert (dyndata.nghostzones[d] == user.dyndata.nghostzones[d]);
    }
  }
  
  dim = user.dyndata.dim;
  assert (dim>=0 && dim<=DIM);
  
  /* optional arguments */
  ierr = Util_TableGetIntArray (options_table, DIM, periodic, "periodic");
  if (ierr == UTIL_ERROR_TABLE_NO_SUCH_KEY) {
    for (d=0; d<dim; ++d) periodic[d] = 0;
    ierr = dim;
  }
  assert (ierr == dim);
  
  ierr = Util_TableGetIntArray (options_table, 2*DIM, solvebnds, "solvebnds");
  if (ierr == UTIL_ERROR_TABLE_NO_SUCH_KEY) {
    for (d=0; d<2*dim; ++d) solvebnds[d] = 0;
    ierr = 2*dim;
  }
  assert (ierr == 2*dim);
  
  ierr = Util_TableGetInt (options_table, &sw, "stencil_width");
  if (ierr == UTIL_ERROR_TABLE_NO_SUCH_KEY) {
    sw = 1;
    ierr = 1;
  }
  assert (ierr == 1);
  
  ierr = Util_TableGetFnPointer (options_table, &ptmp, "jacobian");
  if (ierr == UTIL_ERROR_TABLE_NO_SUCH_KEY) {
    ptmp = 0;
    ierr = 1;
  }
  assert (ierr == 1);
  jac = ptmp;
  
  ierr = Util_TableGetFnPointer (options_table, &ptmp, "get_coloring");
  if (ierr == UTIL_ERROR_TABLE_NO_SUCH_KEY) {
    ptmp = 0;
    ierr = 1;
  }
  assert (ierr == 1);
  get_coloring = ptmp;
  
  assert (solvebnds);
  for (d=0; d<2*user.dyndata.dim; ++d) {
    user.solvebnds[d] = solvebnds[d];
  }
  
  assert (fun);
  user.fun = fun;
  assert (bnd);
  user.bnd = bnd;
  user.jac = jac;
  user.data = data;
  user.funcall_count = 0;
  user.jaccall_count = 0;
  
  
  
  assert (sw>=0);
#if 0
  assert (mglevels>=2);
#endif
  
  
  
  /* Deinstall error handler temporarily */
  ierr = PetscPopErrorHandler ();
  CHKERRQ(ierr);
  
  
  
  /* Determine Cactus' processor layout of grid variables */
  
  lbnd = user.dyndata.lbnd;
  lsh  = user.dyndata.lsh;
  
  nprocs = CCTK_nProcs(cctkGH);
  assert (nprocs>0);
  
  all_lbnd = malloc(nprocs * dim * sizeof(*all_lbnd));
  assert (all_lbnd);
  assert (sizeof(*lbnd) == sizeof(MPI_INT));
  MPI_Allgather ((void*)lbnd, dim, MPI_INT, all_lbnd, dim, MPI_INT, comm);
  all_lsh = malloc(nprocs * dim * sizeof(*all_lsh));
  assert (all_lsh);
  assert (sizeof(*lsh) == sizeof(MPI_INT));
  MPI_Allgather ((void*)lsh, dim, MPI_INT, all_lsh, dim, MPI_INT, comm);
  
  for (d=0; d<dim; ++d) {
    nprocs_dim[d] = 1;
    for (n=1; n<nprocs; ++n) {
      if (all_lbnd[n*dim+d] > all_lbnd[(n-1)*dim+d]) ++nprocs_dim[d];
      if (all_lbnd[n*dim+d] < all_lbnd[(n-1)*dim+d]) break;
    }
  }
  
  if (dim>0) {
    proc_offset_dim[0] = 1;
    /* assume x loops fastest */
    for (d=1; d<dim; ++d) {
      proc_offset_dim[d] = proc_offset_dim[d-1] * nprocs_dim[d-1];
    }
  }
  
  for (d=0; d<dim; ++d) {
    int sum;
    npoints_proc[d] = malloc(nprocs_dim[d] * sizeof(*npoints_proc[d]));
    assert (npoints_proc[d]);
    sum = 0;
    for (n=0; n<nprocs_dim[d]; ++n) {
      npoints_proc[d][n] = all_lsh[(proc_offset_dim[d]*n)*dim+d];
      sum += npoints_proc[d][n] - 2*user.dyndata.nghostzones[d];
    }
    assert (sum == user.dyndata.gsh[d] - 2*user.dyndata.nghostzones[d]);
  }
  
  /* check all assumptions (for the current processor) */
  for (d=0; d<dim; ++d) {
    n = (CCTK_MyProc(cctkGH) / proc_offset_dim[d]) % nprocs_dim[d];
    assert (npoints_proc[d][n] == lsh[d]);
  }
  
  
  
  /* Create distributed array object */
  
  for (d=0; d<user.dyndata.dim; ++d) {
    int sum;
    /* global number of grid points */
    NI[d] = (user.dyndata.gsh[d]
	     - (user.dyndata.nghostzones[d]
		* (!user.solvebnds[2*d] + !user.solvebnds[2*d+1])));
    /* local number of grid points */
    lx[d] = malloc(nprocs_dim[d] * sizeof(*lx[d]));
    sum = 0;
    for (n=0; n<nprocs_dim[d]; ++n) {
      lx[d][n] = (npoints_proc[d][n]
		  - (user.dyndata.nghostzones[d]
		     * (!(n==0 && user.solvebnds[2*d])
			+ !(n==nprocs_dim[d]-1 && user.solvebnds[2*d+1]))));
      sum += lx[d][n];
    }
    assert (sum == NI[d]);
  }
  switch (user.dyndata.dim) {
  case 1:
    if (veryverbose) CCTK_INFO ("DACreate1d");
    ierr = DACreate1d (comm,
		       periodic[0] ? DA_XPERIODIC : DA_NONPERIODIC,
		       NI[0],
		       nvars	/* degrees of freedom */,
		       sw	/* stencil width */,
		       lx[0],
		       &da);
    CHKERRQ(ierr);
    break;
  case 2:
    if (veryverbose) CCTK_INFO ("DACreate2d");
    ierr = DACreate2d (comm,
		       periodic[0]
		       ? (periodic[1] ? DA_XYPERIODIC : DA_XPERIODIC)
		       : (periodic[1] ? DA_YPERIODIC : DA_NONPERIODIC),
		       DA_STENCIL_BOX,
		       NI[0],NI[1],
		       nprocs_dim[0],nprocs_dim[1],
		       nvars	/* degrees of freedom */,
		       sw	/* stencil width */,
		       lx[0],lx[1],
		       &da);
    CHKERRQ(ierr);
    break;
  case 3:
    if (veryverbose) CCTK_INFO ("DACreate3d");
    ierr = DACreate3d (comm,
		       periodic[0]
		       ? (periodic[1]
			  ? (periodic[2] ? DA_XYZPERIODIC : DA_XYPERIODIC)
			  : (periodic[2] ? DA_XZPERIODIC : DA_XPERIODIC))
		       : (periodic[1]
			  ? (periodic[2] ? DA_YZPERIODIC : DA_YPERIODIC)
			  : (periodic[2] ? DA_ZPERIODIC : DA_NONPERIODIC)),
		       DA_STENCIL_BOX,
		       NI[0],NI[1],NI[2],
		       nprocs_dim[0],nprocs_dim[1],nprocs_dim[2],
		       nvars	/* degrees of freedom */,
		       sw	/* stencil width */,
		       lx[0],lx[1],lx[2],
		       &da);
    CHKERRQ(ierr);
    break;
  default:
    abort();
  }
  user.da = da;
  
  for (n=0; n<nvars; ++n) {
    ierr = DASetFieldName (da, n, CCTK_VarName(var[n]));
    CHKERRQ(ierr);
  }
  
  
  
#if 1
  
  
  
  /* Extract global and local vectors from DA */
  
  if (veryverbose) CCTK_INFO ("DACreateGlobalVector");
  ierr = DACreateGlobalVector (da, &x);
  CHKERRQ(ierr);
  ierr = VecDuplicate (x, &f);
  CHKERRQ(ierr);
  
  
  
  /* Create nonlinear solver context */
  
  if (veryverbose) CCTK_INFO ("SNESCreate");
  ierr = SNESCreate (comm,
		     SNES_NONLINEAR_EQUATIONS /* problem type */,
		     &snes);
  CHKERRQ(ierr);
  
  
  
  /* Set function evaluation routine and vector */
  
  if (veryverbose) CCTK_INFO ("SNESSetFunction");
  ierr = SNESSetFunction (snes, f, TATPETSc_function, &user);
  CHKERRQ(ierr);
  
  
  
  if (jac) {
    /* Calculate Jacobian directly through a user given function */
    
    if (veryverbose) CCTK_INFO ("DAGetColoring");
    ierr = DAGetColoring (da, IS_COLORING_GLOBAL, MATMPIAIJ, PETSC_NULL, &J);
    CHKERRQ(ierr);
    if (veryverbose) CCTK_INFO ("SNESSetJacobian");
    ierr = SNESSetJacobian (snes, J, J, TATPETSc_jacobian, &user);
    CHKERRQ(ierr);
    
  } else {
    /* Approximate Jacobian numerically (automatically) */
    
    if (!get_coloring) {
      if (veryverbose) CCTK_INFO ("DAGetColoring");
      ierr = DAGetColoring (da, IS_COLORING_GLOBAL, MATMPIAIJ,
 			    &iscoloring, &J);
      CHKERRQ(ierr);
    } else {
      if (veryverbose) CCTK_INFO ("get_coloring");
      ierr = get_coloring (da, &iscoloring, &J, data);
      CHKERRQ(ierr);
    }
    if (veryverbose) CCTK_INFO ("MatFDColoringCreate");
    ierr = MatFDColoringCreate (J, iscoloring, &matfdcoloring);
    CHKERRQ(ierr);
    if (veryverbose) CCTK_INFO ("ISColoringDestroy");
    ierr = ISColoringDestroy (iscoloring);
    CHKERRQ(ierr);
    if (veryverbose) CCTK_INFO ("MatFDColoringSetFunction");
    ierr = MatFDColoringSetFunction (matfdcoloring,
				     (int(*)(void))TATPETSc_function, &user);
    CHKERRQ(ierr);
    if (veryverbose) CCTK_INFO ("MatFDColoringSetFromOptions");
    ierr = MatFDColoringSetFromOptions (matfdcoloring);
    CHKERRQ(ierr);
    if (veryverbose) CCTK_INFO ("SNESSetJacobian");
    ierr = SNESSetJacobian (snes, J, J, SNESDefaultComputeJacobianColor,
			    matfdcoloring);
    CHKERRQ(ierr);
    
  }
  
  
  
  /* Customise solver */
  
  if (veryverbose) CCTK_INFO ("SNESSetFromOptions");
  ierr = SNESSetFromOptions (snes);
  CHKERRQ(ierr);
  
  
  
  /* Make initial guess */
  
  ierr = TATPETSc_copy (x, &user, TATcopyin, TATcopyvars);
  CHKERRQ(ierr);
  
  
  
#else
  
  
  
  /* Create multigrid solver */
  
  if (veryverbose) CCTK_INFO ("DMMGCreate");
  ierr =  DMMGCreate (PETSC_COMM_WORLD, mglevels, &user, &dmmg);
  CHKERRQ(ierr);
  
  if (veryverbose) CCTK_INFO ("DMMGSetDM");
  ierr = DMMGSetDM (dmmg, (DM)da);
  CHKERRQ(ierr);
  
  if (veryverbose) CCTK_INFO ("DMMGSetSNES");
  ierr = DMMGSetSNES (dmmg, TATPETSc_function, PETSC_NULL);
  CHKERRQ(ierr);
  
  if (veryverbose) CCTK_INFO ("DMMGGetSNES");
  snes = DMMGGetSNES (dmmg);
  assert (snes);
  
  if (veryverbose) CCTK_INFO ("SNESGetSolution");
  ierr = SNESGetSolution (snes, &x);
  CHKERRQ(ierr);
  assert (x);
  
  if (veryverbose) CCTK_INFO ("SNESGetFunction");
  ierr = SNESGetFunction (snes, &f, 0, 0);
  CHKERRQ(ierr);
  assert (f);
  
#if 0
  if (veryverbose) CCTK_INFO ("DMMGSetInitialGuess");
  ierr = DMMGSetInitialGuess (dmmg, ???);
  CHKERRQ(ierr);
#endif
  
  
  
#endif
  
  
  
  /* Calculate initial residual */
  
  if (verbose) {
    ierr = TATPETSc_function (snes, x, f, &user);
    CHKERRQ(ierr);
    ierr = VecNorm (f, NORM_2, &residual);
    CHKERRQ(ierr);
    CCTK_VInfo (CCTK_THORNSTRING, "Initial residual L2-norm: %g", residual);
  }
  
  
  
  /* Solve the nonlinear system */
  
  clk_tck = sysconf(_SC_CLK_TCK);
  assert (clk_tck>0);
  ticks0 = times(&tms_buffer);
  assert (ticks0!=-1);
  time0 = tms_buffer.tms_utime + tms_buffer.tms_stime;
  
#if 1
  if (verbose) CCTK_INFO ("SNESSolve");
  ierr = SNESSolve (snes, x, &iters);
  CHKERRQ(ierr);
#else
  if (verbose) CCTK_INFO ("DMMGSolve");
  ierr = DMMGSolve (dmmg);
  CHKERRQ(ierr);
#endif
  
  ticks1 = times(&tms_buffer);
  assert (ticks1!=-1);
  time1 = tms_buffer.tms_utime + tms_buffer.tms_stime;
  
  ierr = SNESGetIterationNumber (snes, &iters);
  CHKERRQ(ierr);
  ierr = SNESGetNumberLinearIterations (snes, &liniters);
  CHKERRQ(ierr);
  
  if (verbose) {
    CCTK_VInfo (CCTK_THORNSTRING, "Number of Newton iterations: %d", iters);
    CCTK_VInfo (CCTK_THORNSTRING, "Number of linear iterations: %d", liniters);
    CCTK_VInfo (CCTK_THORNSTRING, "Number of function calls:    %d",
		user.funcall_count);
    if (jac) {
      CCTK_VInfo (CCTK_THORNSTRING, "Number of Jacobian calls:    %d",
		  user.jaccall_count);
    }
    CCTK_VInfo (CCTK_THORNSTRING, "Wall time: %g sec, CPU time: %g sec",
		(double)(ticks1-ticks0)/(double)clk_tck,
		(double)(time1-time0)/(double)clk_tck);
  }
  
  
  
  /* Analyse result */
  ierr = SNESGetConvergedReason (snes, &reason);
  CHKERRQ(ierr);
  
  switch (reason) {
  case SNES_CONVERGED_FNORM_ABS:      msg = "FNORM_ABS (F < F_minabs)"; break;
  case SNES_CONVERGED_FNORM_RELATIVE: msg = "FNORM_RELATIVE (F < F_mintol*F_initial)"; break;
  case SNES_CONVERGED_PNORM_RELATIVE: msg = "PNORM_RELATIVE (step size small)"; break;
  case SNES_CONVERGED_GNORM_ABS:      msg = "GNORM_ABS (grad F < grad F_min)"; break;
  case SNES_CONVERGED_TR_REDUCTION:   msg = "TR_REDUCTION"; break;
  case SNES_CONVERGED_TR_DELTA:       msg = "TR_DELTA"; break;
  case SNES_DIVERGED_FUNCTION_COUNT:  msg = "FUNCTION_COUNT"; break;
  case SNES_DIVERGED_FNORM_NAN:       msg = "FNORM_NAN"; break;
  case SNES_DIVERGED_MAX_IT:          msg = "MAX_IT"; break;
  case SNES_DIVERGED_LS_FAILURE:      msg = "LS_FAILURE"; break;
  case SNES_DIVERGED_TR_REDUCTION:    msg = "TR_REDUCTION"; break;
  case SNES_DIVERGED_LOCAL_MIN:       msg = "LOCAL_MIN (|| J^T b || is small, implies converged to local minimum of F())"; break;
  case SNES_CONVERGED_ITERATING:      msg = "ITERATING"; break;
  default:                            msg = "(unknown reason)";
  }
  
  if ((int)reason<=0) {
    CCTK_VWarn (1, __LINE__, __FILE__, CCTK_THORNSTRING, "SNESsolve diverged for reason %d: %s", (int)reason, msg);
  } else {
    if (verbose) {
      CCTK_VInfo (CCTK_THORNSTRING, "SNESsolve converged for reason %d: %s", (int)reason, msg);
    }
  }
  
  
  
  /* Calculate final residual */
  
  ierr = TATPETSc_function (snes, x, f, &user);
  CHKERRQ(ierr);
  if (verbose) {
    ierr = VecNorm (f, NORM_2, &residual);
    CHKERRQ(ierr);
    CCTK_VInfo (CCTK_THORNSTRING, "Final residual L2-norm: %g", residual);
  }
  
  
  
  /* Get solution and residual */
  
  ierr = TATPETSc_copy (x, &user, TATcopyout, TATcopyvars);
  CHKERRQ(ierr);
  ierr = TATPETSc_copy (f, &user, TATcopyout, TATcopyvals);
  CHKERRQ(ierr);
  
  
  
  /* Free work space */
  
#if 1
  
  if (veryverbose) CCTK_INFO ("VecDestroy");
  ierr = VecDestroy (x);
  CHKERRQ(ierr);
  ierr = VecDestroy (f);
  CHKERRQ(ierr);
  
  if (veryverbose) CCTK_INFO ("MatDestroy");
  ierr = MatDestroy (J);
  CHKERRQ(ierr);
  
  if (!jac) {
    if (veryverbose) CCTK_INFO ("MatFDColoringDestroy");
    ierr = MatFDColoringDestroy (matfdcoloring);
    CHKERRQ(ierr);
  }
  
  if (veryverbose) CCTK_INFO ("SNESDestroy");
  ierr = SNESDestroy (snes);
  CHKERRQ(ierr);
  
#else
  
  if (veryverbose) CCTK_INFO ("DMMGDestroy");
  ierr = DMMGDestroy (dmmg);
  CHKERRQ(ierr);
  
#endif
  
  if (veryverbose) CCTK_INFO ("DADestroy");
  ierr = DADestroy (da);
  CHKERRQ(ierr);
  
  user.nvars = 0;
  free (user.var);
  user.var = 0;
  free (user.val);
  user.val = 0;
  
  free (all_lbnd);
  free (all_lsh);
  for (d=0; d<user.dyndata.dim; ++d) free (npoints_proc[d]);
  for (d=0; d<user.dyndata.dim; ++d) free (lx[d]);
  
  /* Reinstall error handler */
  ierr = PetscPushErrorHandler (TATPETSc_error_handler, 0);
  CHKERRQ(ierr);
  
  if (veryverbose) CCTK_INFO ("*** TATPETSc_solve done.");
  
  return reason<0;
}
