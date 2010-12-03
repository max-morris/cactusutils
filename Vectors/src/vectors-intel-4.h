// Vectorise using Intel's or AMD's SSE

// Use the type __m128 directly, without introducing a wrapper class
// Use macros instead of inline functions



#include <xmmintrin.h>



// Vector type corresponding to CCTK_REAL
#define CCTK_REAL4_VEC __m128

// Number of vector elements in a CCTK_REAL_VEC
#define CCTK_REAL4_VEC_SIZE 4



// Create vectors, extract vector elements

#define vec4_set1(a)      (_mm_set1_ps(a))
#define vec4_set(a,b,c,d) (_mm_set_ps(d,c,b,a)) // note reversed arguments

#if defined(__PGI) && defined (__amd64__)
// _mm_cvtss_f32 does not exist on PGI compilers
#  define vec4_elt0(x)                          \
({                                              \
  CCTK_REAL4 aelt0;                             \
  asm ("" : "=x" (aelt0) : "0" (x));            \
  aelt0;                                        \
})
#else
#  define vec4_elt0(x) (_mm_cvtss_f32(x)) // this is a no-op
#endif
#define vec4_elt1(x)                                            \
({                                                              \
  CCTK_REAL4_VEC const xelt1=(x);                               \
  vec4_elt0(_mm_shuffle_ps(xelt1,xelt1,_MM_SHUFFLE(1,0,3,2)));  \
})
#define vec4_elt2(x)                            \
({                                              \
  CCTK_REAL4_VEC const xelt2=(x);               \
  vec4_elt0(_mm_unpackhi_ps(xelt2,xelt2));      \
})
#define vec4_elt3(x)                                            \
({                                                              \
  CCTK_REAL4_VEC const xelt3=(x);                               \
  vec4_elt0(_mm_shuffle_ps(xelt3,xelt3,_MM_SHUFFLE(3,2,1,0)));  \
})
#define vec4_elt(x,d)                           \
({                                              \
  CCTK_REAL4_VEC const xelt=(x);                \
  CCTK_REAL4 aelt;                              \
  switch (d) {                                  \
  case 0: aelt=vec4_elt0(xelt); break;          \
  case 1: aelt=vec4_elt1(xelt); break;          \
  case 2: aelt=vec4_elt2(xelt); break;          \
  case 3: aelt=vec4_elt3(xelt); break;          \
  }                                             \
  aelt;                                         \
})



// Load and store vectors

// Load a vector from memory (aligned and unaligned); this loads from
// a reference to a scalar
#define vec4_load(p)  (_mm_load_ps(&(p)))
#define vec4_loadu(p) (_mm_loadu_ps(&(p)))

// Load a vector from memory that may or may not be aligned, as
// decided by the offset off and the vector size
// Implementation: Always use unaligned load
#define vec4_loadu_maybe(off,p)             (vec4_loadu(p))
#define vec4_loadu_maybe3(off1,off2,off3,p) (vec4_loadu(p))

// Store a vector to memory (aligned and non-temporal); this stores to
// a reference to a scalar
#define vec4_store(p,x)     (_mm_store_ps(&(p),x))
#define vec4_storeu(p,x)    (_mm_storeu_ps(&(p),x))
#define vec4_store_nta(p,x) (_mm_stream_ps(&(p),x))

// Store a lower or higher partial vector (aligned and non-temporal);
// the non-temporal hint is probably ignored
#define vec4_store_nta_partial_lo(p,x,n)        \
({                                              \
  switch (n) {                                  \
  case 3: (&(p))[2]=vec_elt2(p);                \
  case 2: _mm_storel_pi(&(p),x); break;         \
  case 1: (&(p))[0]=vec_elt0(p);                \
  }                                             \
})
#define vec4_store_nta_partial_hi(p,x,n)        \
({                                              \
  switch (n) {                                  \
  case 3: (&(p))[1]=vec_elt1(p);                \
  case 2: _mm_storeh_pi(&(p)+2,x); break;       \
  case 1: (&(p))[3]=vec_elt3(p);                \
  }                                             \
})



// Functions and operators

static const union {
  unsigned i[4];
  __m128   v;
} k4sign_mask_union = {{ 0x80000000U, 0x80000000U, 0x80000000U, 0x80000000U }};
#define k4sign_mask (k4sign_mask_union.v)

// Operators
#define k4pos(x) (x)
#define k4neg(x) (_mm_xor_ps(x,k4sign_mask))

#define k4add(x,y) (_mm_add_ps(x,y))
#define k4sub(x,y) (_mm_sub_ps(x,y))
#define k4mul(x,y) (_mm_mul_ps(x,y))
#define k4div(x,y) (_mm_div_ps(x,y))

// Fused multiply-add, defined as [+-]x*y[+-]z
#define k4madd(x,y,z)  (k4add(k4mul(x,y),z))
#define k4msub(x,y,z)  (k4sub(k4mul(x,y),z))
#define k4nmadd(x,y,z) (k4sub(k4neg(z),k4mul(x,y)))
#define k4nmsub(x,y,z) (k4sub(z,k4mul(x,y)))

// Cheap functions
#define k4fabs(x)   (_mm_andnot_ps(x,k4sign_mask))
#define k4fmax(x,y) (_mm_max_ps(x,y))
#define k4fmin(x,y) (_mm_min_ps(x,y))
#define k4fnabs(x)  (_mm_or_ps(x,k4sign_mask))
#define k4sqrt(x)   (_mm_sqrt_ps(x))

// Expensive functions
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
