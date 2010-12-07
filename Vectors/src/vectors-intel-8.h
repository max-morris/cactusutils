// Vectorise using Intel's or AMD's SSE2

// Use the type __m128d directly, without introducing a wrapper class
// Use macros instead of inline functions



#include <emmintrin.h>



// Vector type corresponding to CCTK_REAL
#define CCTK_REAL8_VEC __m128d

// Number of vector elements in a CCTK_REAL_VEC
#define CCTK_REAL8_VEC_SIZE 2



// Create vectors, extract vector elements

#define vec8_set1(a)  (_mm_set1_pd(a))
#define vec8_set(a,b) (_mm_set_pd(b,a)) // note reversed arguments

#define vec8_elt0(x) (_mm_cvtsd_f64(x)) // this is a no-op
#define vec8_elt1(x)                            \
({                                              \
  CCTK_REAL8_VEC const xelt1=(x);               \
  vec8_elt0(_mm_unpackhi_pd(xelt1,xelt1));      \
})
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
#define vec8_load(p)  (_mm_load_pd(&(p)))
#define vec8_loadu(p) (_mm_loadu_pd(&(p)))

// Load a vector from memory that may or may not be aligned, as
// decided by the offset off and the vector size
// Implementation: Always use unaligned load
#define vec8_loadu_maybe(off,p)             (vec8_loadu(p))
#define vec8_loadu_maybe3(off1,off2,off3,p) (vec8_loadu(p))

// Store a vector to memory (aligned and non-temporal); this stores to
// a reference to a scalar
#define vec8_store(p,x)     (_mm_store_pd(&(p),x))
#define vec8_storeu(p,x)    (_mm_storeu_pd(&(p),x))
#define vec8_store_nta(p,x) (_mm_stream_pd(&(p),x))

// Store a lower or higher partial vector (aligned and non-temporal);
// the non-temporal hint is probably ignored
#define vec8_store_nta_partial_lo(p,x,n) (_mm_storel_pd(&(p),x))
#define vec8_store_nta_partial_hi(p,x,n) (_mm_storeh_pd(&(p)+1,x))



// Functions and operators

static const union {
  unsigned long long i[2];
  __m128d            v;
} k8sign_mask_union = {{ 0x8000000000000000ULL, 0x8000000000000000ULL }};
#define k8sign_mask (k8sign_mask_union.v)
static const union {
  unsigned long long i[2];
  __m128d            v;
} k8abs_mask_union = {{ 0x7fffffffffffffffULL, 0x7fffffffffffffffULL }};
#define k8abs_mask (k8sign_mask_union.v)

// Operators
#define k8pos(x) (x)
#define k8neg(x) (_mm_xor_pd(x,k8sign_mask))

#define k8add(x,y) (_mm_add_pd(x,y))
#define k8sub(x,y) (_mm_sub_pd(x,y))
#define k8mul(x,y) (_mm_mul_pd(x,y))
#define k8div(x,y) (_mm_div_pd(x,y))

// Fused multiply-add, defined as [+-]x*y[+-]z
#define k8madd(x,y,z)  (k8add(k8mul(x,y),z))
#define k8msub(x,y,z)  (k8sub(k8mul(x,y),z))
#define k8nmadd(x,y,z) (k8sub(k8neg(z),k8mul(x,y)))
#define k8nmsub(x,y,z) (k8sub(z,k8mul(x,y)))

// Cheap functions
#define k8fabs(x)   (_mm_and_pd(x,k8abs_mask))
#define k8fmax(x,y) (_mm_max_pd(x,y))
#define k8fmin(x,y) (_mm_min_pd(x,y))
#define k8fnabs(x)  (_mm_or_pd(x,k8sign_mask))
#define k8sqrt(x)   (_mm_sqrt_pd(x))

// Expensive functions
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
