#include "cctk.h" 
#include "cctk_Arguments.h" 
#include "cctk_Parameters.h" 
#include "vectors.h"
#include <math.h>

#define VECTEST(testname, vecexpr, scalarexpr)                  \
{                                                               \
  CCTK_REAL_VEC rv = vecexpr;                                   \
  for(int i=0; i<CCTK_REAL_VEC_SIZE; i++) {                     \
    CCTK_REAL res = scalarexpr;                                 \
    CCTK_REAL vecres = vec_elt(rv,i);                           \
    if(vecres == res)                                           \
      passed++;                                                 \
    else                                                        \
      CCTK_VWarn(CCTK_WARN_ALERT, __LINE__, __FILE__,           \
        CCTK_THORNSTRING, "Failed test %s", testname);          \
    numtests++;                                                 \
  }                                                             \
}

void Vectors_Test(CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  CCTK_INT passed = 0, numtests=0;

  CCTK_REAL a[CCTK_REAL_VEC_SIZE];
  CCTK_REAL b[CCTK_REAL_VEC_SIZE];
  CCTK_REAL c[CCTK_REAL_VEC_SIZE];

  for(int i=0; i<CCTK_REAL_VEC_SIZE; i++) {
    a[i] =  (i+1)*1.23456789;
    b[i] = -(i+1)*9.87654321;
    c[i] =  (i+1)*1.01010101;
  }

  CCTK_REAL_VEC av = vec_loadu(a[0]);
  CCTK_REAL_VEC bv = vec_loadu(b[0]);
  CCTK_REAL_VEC cv = vec_loadu(c[0]);
  CCTK_REAL_VEC rv = vec_loadu(c[0]);

  /* TODO: Add individual tests for vec_set1, vec_set, vec_elt0, vec_elt
           vec_load, vec_loadu, vec_loadu_maybe, vec_loadu_maybe3
           vec_store, vec_store_nta, vec_store_nta_partial_lo,
           vec_store_nta_partial_hi, vec_store_nta_partial_mid */

  VECTEST("kpos",   kpos(av),           +a[i]               );
  VECTEST("kneg",   kneg(av),           -a[i]               );

  VECTEST("kadd",   kadd(av, bv),       a[i] + b[i]         );
  VECTEST("ksub",   ksub(av, bv),       a[i] - b[i]         );
  VECTEST("kmul",   kmul(av, bv),       a[i] * b[i]         );
  VECTEST("kdiv",   kdiv(av, bv),       a[i] / b[i]         );

  VECTEST("kmadd",  kmadd(av, bv, cv),   a[i] * b[i] + c[i] );
  VECTEST("kmsub",  kmsub(av, bv, cv),   a[i] * b[i] - c[i] );
  VECTEST("knmadd", knmadd(av, bv, cv), -a[i] * b[i] - c[i] );
  VECTEST("knmsub", knmsub(av, bv, cv), -a[i] * b[i] + c[i] );

  VECTEST("kexp",   kexp(av),           exp(a[i])           );
  VECTEST("kfabs",  kfabs(av),          fabs(a[i])          );
  VECTEST("kfmax",  kfmax(av, bv),      fmax(a[i], b[i])    );
  VECTEST("kfmin",  kfmin(av, bv),      fmin(a[i], b[i])    );
  VECTEST("kfnabs", kfnabs(av),         -fabs(a[i])         );
  VECTEST("klog",   klog(av),           log(a[i])           );
  VECTEST("kpow",   kpow(av, 3.14159),  pow(a[i], 3.14159)  );
  VECTEST("ksqrt",  ksqrt(av),          sqrt(a[i])          );

  VECTEST("kifpos positive", kifpos(av, bv, cv), signbit(a[i]) ? c[i] : b[i]);
  VECTEST("kifpos negative", kifpos(bv, bv, cv), signbit(b[i]) ? c[i] : b[i]);
  VECTEST("kifpos 0",     kifpos(ToReal(0.),bv,cv),  signbit(0.)?c[i]:b[i]);
  VECTEST("kifpos -0",    kifpos(ToReal(-0.),bv,cv), signbit(-0.)?c[i]:b[i]);

  if (passed != numtests)
    CCTK_VWarn(CCTK_WARN_ABORT, __LINE__, __FILE__, CCTK_THORNSTRING,
      "Failed %d correctness tests", numtests - passed);
  else
    CCTK_VInfo(CCTK_THORNSTRING, "%d/%d tests passed ", passed, numtests);

  return;
}
