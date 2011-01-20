// Vectorise using IBM's Blue Gene/P Double Hummer (Power)

// Use the type double _Complex directly, without introducing a wrapper class
// Use macros instead of inline functions



#include <builtins.h>



// Vector type corresponding to CCTK_REAL
#define CCTK_REAL8_VEC double _Complex

// Number of vector elements in a CCTK_REAL_VEC
#define CCTK_REAL8_VEC_SIZE 2



// Create vectors, extract vector elements

#define vec8_set1(a)  (__cmplx(a,a))
#define vec8_set(a,b) (__cmplx(a,b))

#define vec8_elt0(x) (__creal(x))
#define vec8_elt1(x) (__cimag(x))
#define vec8_elt(x,d)                           \
({                                              \
  CCTK_REAL8_VEC const xelt=(x);                \
  CCTK_REAL8 aelt;                              \
  switch (d) {                                  \
  case 0: aelt=vec8_elt0(xelt); break;          \
  case 1: aelt=vec8_elt1(xelt); break;          \
  }                                             \
  aelt;                                         \
})



// Load and store vectors

// Load a vector from memory (aligned and unaligned); this loads from
// a reference to a scalar
#define vec8_load(p)  (__lfpd((double *)&(p)))
#define vec8_loadu(p) (__lfpd((double *)&(p))) // this may not work

// Load a vector from memory that may or may not be aligned, as
// decided by the offset and the vector size
#define vec8_loadu_maybe(off,p)             (vec8_loadu(p))
#define vec8_loadu_maybe3(off1,off2,off3,p) (vec8_loadu(p))

// Store a vector to memory (aligned and non-temporal); this stores to
// a reference to a scalar
#define vec8_store(p,x)     (__stfpd(&(p),x))
#define vec8_storeu(p,x)    (__stfpd(&(p),x)) // this may not work
#define vec8_store_nta(p,x) (__stfpd(&(p),x)) // this doesn't avoid the cache

// Store a lower or higher partial vector (aligned and non-temporal);
// the non-temporal hint is probably ignored
#define vec8_store_nta_partial_lo(p,x,n) ((&(p))[0]=vec8_elt0(x))
#define vec8_store_nta_partial_hi(p,x,n) ((&(p))[1]=vec8_elt1(x))



// Functions and operators

// Operators
#define k8pos(x) (x)
#define k8neg(x) (__fpneg(x))

#define k8add(x,y) (__fpadd(x,y))
#define k8sub(x,y) (__fpsub(x,y))
#define k8mul(x,y) (__fpmul(x,y))
#define k8div(x,y) (__fpmul(x,__fpre(y)))

// Fused multiply-add, defined as [+-]x*y[+-]z
#define k8madd(x,y,z)  (__fpmadd(z,x,y))
#define k8msub(x,y,z)  (__fpmsub(z,x,y))
#define k8nmadd(x,y,z) (__fpnmadd(z,x,y))
#define k8nmsub(x,y,z) (__fpnmsub(z,x,y))

// Cheap functions
#define k8fabs(x)   (__fpabs(x))
#define k8fmax(x,y) (__fpsel(__fpsub(y,x),x,y))
#define k8fmin(x,y) (__fpsel(__fpsub(x,y),x,y))
#define k8fnabs(x)  (__fpnabs(x))

#define k8exp(x)                                        \
({                                                      \
  CCTK_REAL8_VEC const xexp=(x);                        \
  vec8_set(exp(vec8_elt0(xexp)), exp(vec8_elt1(xexp))); \
})
#define k8log(x)                                        \
({                                                      \
  CCTK_REAL8_VEC const xlog=(x);                        \
  vec8_set(log(vec8_elt0(xlog)), log(vec8_elt1(xlog))); \
})
#define k8pow(x,a)                                                      \
({                                                                      \
  CCTK_REAL8_VEC const xpow=(x);                                        \
  CCTK_REAL8 const apow=(a);                                            \
  vec8_set(pow(vec8_elt0(xpow),apow), pow(vec8_elt1(xpow),apow));       \
})
#define k8sqrt(x)                                               \
({                                                              \
  CCTK_REAL8_VEC const xsqrt=(x);                               \
  vec8_set(sqrt(vec8_elt0(xsqrt)), sqrt(vec8_elt1(xsqrt)));     \
})
