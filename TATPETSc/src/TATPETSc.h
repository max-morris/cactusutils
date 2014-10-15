/* (C) 2001-04-22 Erik Schnetter <schnetter@uni-tuebingen.de> */

#ifndef TATPETSC_H
#define TATPETSC_H



#include <mpi.h>

#include <petscdmda.h>
#include <petscsnes.h>

#include "cctk.h"



#define DIM 3



typedef enum { TATcopyin, TATcopyout } TATdir;
typedef enum { TATcopyvars, TATcopyvals } TATvarset;



#define MAGIC 0xcafebabeUL

typedef struct {
  unsigned long magic;
  const cGH *cctkGH;
  MPI_Comm comm;
  DM da;
  int nvars;
  int *var;
  int *val;
  CCTK_INT * restrict jac0;
  int nboundaryzones[2*DIM];
  cGroupDynamicData dyndata;
  int (*fun) (const cGH *cctkGH, int options_table, void *data);
  int (*bnd) (const cGH *cctkGH, int options_table, void *data);
  int (*jac) (const cGH *cctkGH, int options_table, void *data);
  void *data;
  int funcall_count;
  int jaccall_count;
} userdata;



/* private functions */
int TATPETSc_copy (Vec x, void *userptr, TATdir dir, TATvarset varset);
int TATPETSc_copyjac (Mat J, void *userptr);

int TATPETSc_function (SNES snes, Vec x, Vec f, void *userptr);
int TATPETSc_jacobian (SNES snes, Vec x, Mat J, Mat B, void *userptr);

PetscErrorCode TATPETSc_error_handler (MPI_Comm comm, int line,
                                       const char *func, const char *file,
                                       PetscErrorCode n, PetscErrorType p,
                                       const char *mess, void *ctx);



/* public functions */

int TATPETSc_solve (const cGH *cctkGH,
		    const int *var, const int *val, int nvars,
		    int options_table,
		    int (*fun) (const cGH *cctkGH, int options_table,
                                void *data),
		    int (*bnd) (const cGH *cctkGH, int options_table,
                                void *data),
		    void *data);



#endif /* !defined(TATPETSC_H) */
