/* (C) 2001-04-22 Erik Schnetter <schnetter@uni-tuebingen.de> */
/* $Header$ */

#ifndef TATPETSC_H
#define TATPETSC_H



#include "mpi.h"

#include "petscda.h"
#include "petscsnes.h"

#include "cctk.h"



#define DIM 3



typedef enum { TATcopyin, TATcopyout } TATdir;
typedef enum { TATcopyvars, TATcopyvals } TATvarset;



#define MAGIC 0xcafebabe

typedef struct {
  int magic;
  const cGH *cctkGH;
  MPI_Comm comm;
  DA da;
  int nvars;
  int *var;
  int *val;
  int solvebnds[2*DIM];
  cGroupDynamicData dyndata;
  int (*fun) (const cGH *cctkGH, void *data);
  int (*bnd) (const cGH *cctkGH, void *data);
  int (*jac) (const cGH *cctkGH, Mat *J, Mat *B, MatStructure *flag,
	      void *data);
  void *data;
  int funcall_count;
  int jaccall_count;
} userdata;



/* private functions */
int TATPETSc_copy (Vec x, void *userptr, TATdir dir, TATvarset varset);

int TATPETSc_function (SNES snes, Vec x, Vec f, void *userptr);
int TATPETSc_jacobian (SNES snes, Vec x, Mat *J, Mat *B, MatStructure *flag,
		       void *userptr);



/* public functions */

int TATPETSc_solve (const cGH *cctkGH,
		    const int *var, const int *val, int nvars,
		    int options_table,
		    int (*fun) (const cGH *cctkGH, void *data),
		    int (*bnd) (const cGH *cctkGH, void *data),
		    void *data);



#endif /* !defined(TATPETSC_H) */
