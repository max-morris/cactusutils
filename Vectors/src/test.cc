#include "vectors.h"

#include <cctk.h>
#include <cctk_Arguments.h>
#include <cctk_Parameters.h>

#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>

using namespace std;

inline int my_signbit (CCTK_REAL const x)
{
  using namespace std;
  return signbit(x);
}

#define SCALARTEST(testname, vecexpr, scalarexpr)                       \
  do {                                                                  \
    if (verbose)                                                        \
      CCTK_VInfo (CCTK_THORNSTRING, "Test %s...", testname);            \
    CCTK_REAL res = (scalarexpr);                                       \
    CCTK_REAL vecres = (vecexpr);                                       \
    CCTK_REAL eps = numeric_limits<CCTK_REAL>::epsilon();               \
    assert(abs((CCTK_REAL)0.1) > 0);                                    \
    if((abs(vecres - res) < 10*eps*abs(res)) ||                         \
       (isnan(vecres) && isnan(res)))                                   \
      passed++;                                                         \
    else                                                                \
      CCTK_VParamWarn(CCTK_THORNSTRING,                                 \
                      "Failed test %s: expected %.17g, received %.17g", \
                      testname, (double)res, (double)vecres);           \
    numtests++;                                                         \
  } while(0)

#define VECTEST(testname, vecexpr, scalarexpr)                          \
  do {                                                                  \
    if (verbose)                                                        \
      CCTK_VInfo (CCTK_THORNSTRING, "Test %s...", testname);            \
    CCTK_REAL_VEC rv = (vecexpr);                                       \
    for(int i=0; i<CCTK_REAL_VEC_SIZE; i++) {                           \
      CCTK_REAL res = (scalarexpr);                                     \
      CCTK_REAL vecres = vec_elt(rv,i);                                 \
      CCTK_REAL eps = numeric_limits<CCTK_REAL>::epsilon();             \
      assert(abs((CCTK_REAL)0.1) > 0);                                  \
      if((abs(vecres - res) < 10*eps*abs(res)) ||                       \
         (isnan(vecres) && isnan(res)))                                 \
        passed++;                                                       \
      else                                                              \
        CCTK_VParamWarn(CCTK_THORNSTRING,                               \
                        "Failed test %s: "                              \
                        "for element %d, expected %.17g, received %.17g", \
                        testname, i, (double)res, (double)vecres);      \
      numtests++;                                                       \
    }                                                                   \
  } while(0)

extern "C"
void Vectors_Test(CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  CCTK_INFO ("Testing vectorisation... [errors may result in segfaults]");
  fflush (stdout);

  char testname[100];

  int passed = 0, numtests = 0;

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

  /* l and lv are similar to a and av, except that it is larger, and
     guaranteed to be aligned */
  CCTK_REAL_VEC lv[4];
  lv[0] = vec_loadu(a[0]);
  lv[1] = vec_loadu(b[0]);
  lv[2] = vec_loadu(c[0]);
  lv[3] = vec_loadu(a[0]);
  CCTK_REAL *const l = (CCTK_REAL*)&lv[0];

  /* s and sv are similar to a and av, but are aligned and not
     initialised */
  CCTK_REAL_VEC sv;
  CCTK_REAL *const s = (CCTK_REAL*)&sv;

  VECTEST("vec_set1", vec_set1(a[0]),  a[0]);
#if CCTK_REAL_VEC_SIZE == 1
  VECTEST("vec_set", vec_set(a[0]), a[i]);
#elif CCTK_REAL_VEC_SIZE == 2
  VECTEST("vec_set", vec_set(a[0],a[1]), a[i]);
#elif CCTK_REAL_VEC_SIZE == 4
  VECTEST("vec_set", vec_set(a[0],a[1],a[2],a[3]), a[i]);
#else
#  error "Unsupported vector size"
#endif
  SCALARTEST("vec_elt0", vec_elt0(av),    a[0]);
  for (int d=0; d<CCTK_REAL_VEC_SIZE; ++d) {
    snprintf (testname, sizeof testname, "vec_elt[%d]", d);
    SCALARTEST(testname, vec_elt(av,d), a[d]);
  }

  /* These tests will probably fail with a segfault, if they fail */
  VECTEST("vec_load", vec_load(*l), l[i]);
  for (int d=0; d<CCTK_REAL_VEC_SIZE; ++d) {
    snprintf (testname, sizeof testname, "vec_loadu[%d]", d);
    VECTEST(testname, vec_loadu(l[d]), l[i+d]);
  }
  for (int d=0; d<CCTK_REAL_VEC_SIZE; ++d) {
    snprintf (testname, sizeof testname, "vec_loadu_maybe[%d]", d);
    VECTEST(testname, vec_loadu_maybe(d,l[d]), l[i+d]);
  }
  for (int d1=0; d1<CCTK_REAL_VEC_SIZE; ++d1) {
    for (int d2=0; d2<CCTK_REAL_VEC_SIZE; ++d2) {
      for (int d3=0; d3<CCTK_REAL_VEC_SIZE; ++d3) {
        if (! (VECTORISE && VECTORISE_ALIGNED_ARRAYS) || (d2==0 && d3==0)) {
          snprintf (testname, sizeof testname,
                    "vec_loadu_maybe3[%d,%d,%d]", d1,d2,d3);
          VECTEST(testname,
                  vec_loadu_maybe3(d1,d2,d3,l[d1+d2+d3]), l[i+d1+d2+d3]);
        }
      }
    }
  }

  /* These tests may fail with a segfault, if they fail */
  sv = av; vec_store(*s, bv);
  VECTEST("vec_store", sv, b[i]);
  sv = av; vec_store_nta(*s, bv);
  VECTEST("vec_store_nta", sv, b[i]);
  int const arenaitems = 3;
  int const arenasize = arenaitems * CCTK_REAL_VEC_SIZE;
  for (int ilo=0; ilo<arenasize; ++ilo) {
    for (int ihi=0; ihi<arenasize; ++ihi) {
      // Initialise arena to a everywhere
      for (int n=0; n<arenaitems; ++n) {
        lv[n] = av;
      }
      // Set arena to b in the "interior"
      for (int i=(ilo & -CCTK_REAL_VEC_SIZE); i<ihi; i+=CCTK_REAL_VEC_SIZE) {
        vec_store_partial_prepare(i, ilo, ihi);
        vec_store_nta_partial(l[i], bv);
      }
      for (int i=0; i<arenasize; ++i) {
        snprintf (testname, sizeof testname,
                  "vec_store_nta_partial[%d,%d;%d]", ilo, ihi, i);
        SCALARTEST(testname, l[i],
                   (i>=ilo and i<ihi ? b : a)[i & (CCTK_REAL_VEC_SIZE-1)]);
      }
    }
  }
  /* The partial stores are not implemented for d==0 and
     d==CCTK_REAL_VEC_SIZE (because these are trivial) */
  for (int d=1; d<CCTK_REAL_VEC_SIZE-1; ++d) {
    sv = av; vec_store_nta_partial_lo(*s, bv, d);
    snprintf (testname, sizeof testname, "vec_store_nta_partial_lo[%d]", d);
    VECTEST(testname, sv, i<d ? b[i] : a[i]);
  }
  for (int d=1; d<CCTK_REAL_VEC_SIZE-1; ++d) {
    sv = av; vec_store_nta_partial_hi(*s, bv, d);
    snprintf (testname, sizeof testname, "vec_store_nta_partial_hi[%d]", d);
    VECTEST(testname, sv, i>=CCTK_REAL_VEC_SIZE-d ? b[i] : a[i]);
  }
  for (int dlo=1; dlo<CCTK_REAL_VEC_SIZE-1; ++dlo) {
    for (int dhi=1; dhi<CCTK_REAL_VEC_SIZE-1; ++dhi) {
      sv = av; vec_store_nta_partial_mid(*s, bv, dlo, dhi);
      snprintf (testname, sizeof testname,
                "vec_store_nta_partial_mid[%d,%d]", dlo, dhi);
      VECTEST(testname, sv, i<dlo && i>=CCTK_REAL_VEC_SIZE-dhi ? b[i] : a[i]);
    }
  }

  VECTEST("kneg",   kneg(av),           -a[i]               );

  VECTEST("kadd",   kadd(av, bv),       a[i] + b[i]         );
  VECTEST("ksub",   ksub(av, bv),       a[i] - b[i]         );
  VECTEST("kmul",   kmul(av, bv),       a[i] * b[i]         );
  VECTEST("kdiv",   kdiv(av, bv),       a[i] / b[i]         );

  VECTEST("kmadd",  kmadd(av, bv, cv),   a[i] * b[i] + c[i] );
  VECTEST("kmsub",  kmsub(av, bv, cv),   a[i] * b[i] - c[i] );
  VECTEST("knmadd", knmadd(av, bv, cv), -a[i] * b[i] - c[i] );
  VECTEST("knmsub", knmsub(av, bv, cv), -a[i] * b[i] + c[i] );

  VECTEST("kacos",  kacos(av),          acos(a[i])          );
  VECTEST("kacosh", kacosh(av),         acosh(a[i])         );
  VECTEST("kasin",  kasin(av),          asin(a[i])          );
  VECTEST("kasinh", kasinh(av),         asinh(a[i])         );
  VECTEST("katan",  katan(av),          atan(a[i])          );
  VECTEST("katan2", katan2(av, bv),     atan2(a[i], b[i])   );
  VECTEST("katanh", katanh(av),         atanh(a[i])         );
  VECTEST("kcos",   kcos(av),           cos(a[i])           );
  VECTEST("kcosh",  kcosh(av),          cosh(a[i])          );
  VECTEST("kexp",   kexp(av),           exp(a[i])           );
  VECTEST("kfabs",  kfabs(av),          fabs(a[i])          );
  VECTEST("kfmax",  kfmax(av, bv),      fmax(a[i], b[i])    );
  VECTEST("kfmin",  kfmin(av, bv),      fmin(a[i], b[i])    );
  VECTEST("kfnabs", kfnabs(av),         -fabs(a[i])         );
  VECTEST("klog",   klog(av),           log(a[i])           );
  VECTEST("kpow",   kpow(av, b[0]),     pow(a[i], b[0])     );
  VECTEST("ksin",   ksin(av),           sin(a[i])           );
  VECTEST("ksinh",  ksinh(av),          sinh(a[i])          );
  VECTEST("ksqrt",  ksqrt(av),          sqrt(a[i])          );
  VECTEST("ktan",   ktan(av),           tan(a[i])           );
  VECTEST("ktanh",  ktanh(av),          tanh(a[i])          );

  VECTEST("kifpos positive",
          kifpos(av, bv, cv), my_signbit(a[i]) ? c[i] : b[i]);
  VECTEST("kifpos negative",
          kifpos(bv, bv, cv), my_signbit(b[i]) ? c[i] : b[i]);
  VECTEST("kifpos 0",  kifpos(vec_set1(0.),bv,cv),  b[i]);
  VECTEST("kifpos -0", kifpos(vec_set1(-0.),bv,cv), c[i]);

  VECTEST("kifneg positive",
          kifneg(av, bv, cv), my_signbit(a[i]) ? b[i] : c[i]);
  VECTEST("kifneg negative",
          kifneg(bv, bv, cv), my_signbit(b[i]) ? b[i] : c[i]);
  VECTEST("kifneg 0",  kifneg(vec_set1(0.),bv,cv),  c[i]);
  VECTEST("kifneg -0", kifneg(vec_set1(-0.),bv,cv), b[i]);

  if (passed != numtests)
    CCTK_VWarn(CCTK_WARN_ALERT, __LINE__, __FILE__, CCTK_THORNSTRING,
      "Failed %d correctness tests", numtests - passed);
  else
    CCTK_VInfo(CCTK_THORNSTRING, "%d/%d tests passed ", passed, numtests);

  return;
}
