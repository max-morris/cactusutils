/* (C) 2001-04-18 Erik Schnetter <schnetter@uni-tuebingen.de> */
/* $Header$ */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpi.h"

#include "petsc.h"

#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"

#include "TATPETSc.h"
#include "TATelliptic.h"



/* A new error handler that does nothing */
int TATPETSc_error_handler (int line, char *fun, char *file, char *dir, int n, int p, char *mess, void *ctx)
{
  return p;
}



void TATPETSc_initialize (void)
{
  DECLARE_CCTK_PARAMETERS;
  
  /* world communicator */
  MPI_Comm comm;
  
  static int argc;
  static char *args;
  static char **argv;
  
  int len;
  int i;
  
  int ierr;
  
  
  
  if (veryverbose) CCTK_INFO ("PetscInitialize");
  
  
  
  /* Create PETSc command line options */
  
  args = strdup(options);
  assert (args);
  
  len = strlen(args);
  argc = 2;
  for (i=0; i<len; ++i) {
    if (args[i]==' ') {
      args[i] = '\0';
      ++argc;
    }
  }
  
  argv = malloc(sizeof(*argv) * (argc+1));
  assert (argv);
  argv[0] = "Cactus";
  argv[1] = args;
  argc = 2;
  for (i=1; i<len; ++i) {
    if (args[i-1]=='\0') {
      argv[argc] = &args[i];
      ++argc;
    }
  }
  argv[argc] = 0;
  
  if (veryverbose) {
    CCTK_INFO ("PETSc command line arguments:");
    for (i=0; i<argc; ++i) {
      CCTK_VInfo (CCTK_THORNSTRING, "   %d: %s", i, argv[i]);
    }
    CCTK_INFO ("End of PETSc command line arguments.");
  }
  
  
  
  /* Initialise PETSc */
  comm = MPI_COMM_WORLD;
  ierr = PetscSetCommWorld (comm);
  CHKERRQ(ierr);
  
  ierr = PetscInitialize (&argc, &argv, PETSC_NULL, PETSC_NULL);
  CHKERRQ(ierr);
  
  
  
  /* Install a new error handler */
  ierr = PetscPushErrorHandler (TATPETSc_error_handler, 0);
  CHKERRQ(ierr);
  
  
  
  /* Register the solver */
  ierr = TATelliptic_RegisterSolver (TATPETSc_solve, "TATPETSc");
  assert (!ierr);
}
