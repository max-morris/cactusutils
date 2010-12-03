// Vectorise using IBM's  Altivec (Power)

// Use the type vector double directly, without introducing a wrapper class
// Use macros instead of inline functions



#include <altivec.h>



// Vector type corresponding to CCTK_REAL
#define CCTK_REAL4_VEC vector float

// Number of vector elements in a CCTK_REAL_VEC
#define CCTK_REAL4_VEC_SIZE 4



// Create vectors, extract vector elements

#define vec4_set1(a)  (vec_splats(a))
#define vec4_set(a,b,c,d)                       \
({                                              \
  CCTK_REAL4_VEC x;                             \
  x[0]=(a);                                     \
  x[1]=(b);                                     \
  x[2]=(c);                                     \
  x[3]=(d);                                     \
  x;                                            \
})

#define vec4_elt0(x) ((x)[0])
#define vec4_elt1(x) ((x)[1])
#define vec4_elt2(x) ((x)[2])
#define vec4_elt3(x) ((x)[3])
#define vec4_elt(x,d) ((x)[d])



// Load and store vectors

// Load a vector from memory (aligned and unaligned); this loads from
// a reference to a scalar
#define vec4_load(p)  (*(CCTK_REAL4_VEC const*)&(p))
#define vec4_loadu(p) (*(CCTK_REAL4_VEC const*)&(p))

// Load a vector from memory that may or may not be aligned, as
// decided by the offset and the vector size
#define vec4_loadu_maybe(off,p)             (vec4_loadu(p))
#define vec4_loadu_maybe3(off1,off2,off3,p) (vec4_loadu(p))

// Store a vector to memory (aligned and non-temporal); this stores to
// a reference to a scalar
#define vec4_store(p,x)     (*(CCTK_REAL4_VEC*)&(p)=(x))
#define vec4_storeu(p,x)    (*(CCTK_REAL4_VEC*)&(p)=(x))
// TODO: Use stvxl instruction?
#define vec4_store_nta(p,x) (*(CCTK_REAL4_VEC*)&(p)=(x))

// Store a lower or higher partial vector (aligned and non-temporal);
// the non-temporal hint is probably ignored
#define vec4_store_nta_partial_lo(p,x,n)        \
({                                              \
  switch (n) {                                  \
  case 3: ((&(p))[2]=(x)[2]);                   \
  case 2: ((&(p))[1]=(x)[1]);                   \
  case 1: ((&(p))[0]=(x)[0]);                   \
  }                                             \
})
#define vec4_store_nta_partial_hi(p,x,n)        \
({                                              \
  switch (n) {                                  \
  case 3: ((&(p))[1]=(x)[1]);                   \
  case 2: ((&(p))[2]=(x)[2]);                   \
  case 1: ((&(p))[3]=(x)[3]);                   \
  }                                             \
})



// Functions and operators

// Operators
#define k4pos(x) (+(x))
#define k4neg(x) (-(x))

#define k4add(x,y) ((x)+(y))
#define k4sub(x,y) ((x)-(y))
#define k4mul(x,y) ((x)*(y))
#define k4div(x,y) ((x)/(y))

// Fused multiply-add, defined as [+-]x*y[+-]z
#define k4madd(x,y,z)  (vec_madd(x,y,z))
#define k4msub(x,y,z)  (vec_msub(x,y,z))
#define k4nmadd(x,y,z) (vec_nmadd(x,y,z))
#define k4nmsub(x,y,z) (vec_nmsub(x,y,z))

// Cheap functions
#define k4fabs(x)   (vec_abs(x))
#define k4fmax(x,y) (vec_max(x,y))
#define k4fmin(x,y) (vec_min(x,y))
#define k4fnabs(x)  (vec_nabs(x))

#define k4exp(x)                                        \
({                                                      \
  CCTK_REAL4_VEC const xexp=(x);                        \
  vec4_set(exp(vec4_elt0(xexp)), exp(vec4_elt1(xexp)),  \
           exp(vec4_elt2(xexp)), exp(vec4_elt3(xexp))); \
})
#define k4log(x)                                        \
({                                                      \
  CCTK_REAL4_VEC const xlog=(x);                        \
  vec4_set(log(vec4_elt0(xlog)), log(vec4_elt1(xlog)),  \
           log(vec4_elt2(xlog)), log(vec4_elt3(xlog))); \
})
#define k4pow(x,a)                                                      \
({                                                                      \
  CCTK_REAL4_VEC const xpow=(x);                                        \
  CCTK_REAL4 const apow=(a);                                            \
  vec4_set(pow(vec4_elt0(xpow),apow), pow(vec4_elt1(xpow),apow),        \
           pow(vec4_elt2(xpow),apow), pow(vec4_elt3(xpow),apow));       \
})
#define k4sqrt(x)                                               \
({                                                              \
  CCTK_REAL4_VEC const xsqrt=(x);                               \
  vec4_set(sqrt(vec4_elt0(xsqrt)), sqrt(vec4_elt1(xsqrt)),      \
           sqrt(vec4_elt2(xsqrt)), sqrt(vec4_elt3(xsqrt)));     \
})
