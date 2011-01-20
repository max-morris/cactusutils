// Vectorise using Intel's or AMD's AVX

// Use the type __m256d directly, without introducing a wrapper class
// Use macros instead of inline functions



#if defined(EMULATE_AVX)
#  include "avxintrin_emu.h"
#else
#  include <immintrin.h>
#endif



// Vector type corresponding to CCTK_REAL
#define CCTK_REAL8_VEC __m256d

// Number of vector elements in a CCTK_REAL_VEC
#define CCTK_REAL8_VEC_SIZE 4



union k8const_t {
  unsigned long long i[4];
  double             d[4];
  __m256i            vi;
  __m256d            vd;
};

#define K8_ZERO 0x0000000000000000ULL
#define K8_IMIN 0x8000000000000000ULL
#define K8_IMAX 0x7fffffffffffffffULL



// Create vectors, extract vector elements

#define vec8_set1(a)      (_mm256_set1_pd(a))
#define vec8_set(a,b,c,d) (_mm256_set_pd(d,c,b,a)) // note reversed arguments

#define vec8_elt0(x) (_mm_cvtsd_f64(_mm256_extractf128_pd(x,0)))
#define vec8_elt1(x)                                            \
({                                                              \
  __m128d const xelt1=_mm256_extractf128_pd(x,0);               \
  _mm_cvtsd_f64(_mm_unpackhi_pd(xelt1,xelt1));                  \
})
#define vec8_elt2(x) (_mm_cvtsd_f64(_mm256_extractf128_pd(x,1)))
#define vec8_elt3(x)                                            \
({                                                              \
  __m128d const xelt3=_mm256_extractf128_pd(x,1);               \
  _mm_cvtsd_f64(_mm_unpackhi_pd(xelt3,xelt3));                  \
})

#define vec8_elt(x,d)                           \
({                                              \
  CCTK_REAL8_VEC const xelt=(x);                \
  CCTK_REAL8 aelt;                              \
  switch (d) {                                  \
  case 0: aelt=vec8_elt0(xelt); break;          \
  case 1: aelt=vec8_elt1(xelt); break;          \
  case 2: aelt=vec8_elt2(xelt); break;          \
  case 3: aelt=vec8_elt3(xelt); break;          \
  }                                             \
  aelt;                                         \
})



// Load and store vectors

// Load a vector from memory (aligned and unaligned); this loads from
// a reference to a scalar
#define vec8_load(p)  (_mm256_load_pd(&(p)))
#define vec8_loadu(p) (_mm256_loadu_pd(&(p)))

// Load a vector from memory that may or may not be aligned, as
// decided by the offset off and the vector size
// Implementation: Always use unaligned load
#define vec8_loadu_maybe(off,p)             (vec8_loadu(p))
#define vec8_loadu_maybe3(off1,off2,off3,p) (vec8_loadu(p))

// Store a vector to memory (aligned and non-temporal); this stores to
// a reference to a scalar
#define vec8_store(p,x)     (_mm256_store_pd(&(p),x))
#define vec8_storeu(p,x)    (_mm256_storeu_pd(&(p),x))
#define vec8_store_nta(p,x) (_mm256_stream_pd(&(p),x))

// Store a lower or higher partial vector (aligned and non-temporal);
// the non-temporal hint is probably ignored
static const k8const_t k8store_lo_union[5] =
  {
    {{ K8_ZERO, K8_ZERO, K8_ZERO, K8_ZERO, }},
    {{ K8_IMIN, K8_ZERO, K8_ZERO, K8_ZERO, }},
    {{ K8_IMIN, K8_IMIN, K8_ZERO, K8_ZERO, }},
    {{ K8_IMIN, K8_IMIN, K8_IMIN, K8_ZERO, }},
    {{ K8_IMIN, K8_IMIN, K8_IMIN, K8_IMIN, }},
  };
#define vec8_store_nta_partial_lo(p,x,n)                \
  (_mm256_maskstore_pd(&(p),k8store_lo_union[n].vi,x))
static const k8const_t k8store_hi_union[5] =
  {
    {{ K8_ZERO, K8_ZERO, K8_ZERO, K8_ZERO, }},
    {{ K8_ZERO, K8_ZERO, K8_ZERO, K8_IMIN, }},
    {{ K8_ZERO, K8_ZERO, K8_IMIN, K8_IMIN, }},
    {{ K8_ZERO, K8_IMIN, K8_IMIN, K8_IMIN, }},
    {{ K8_IMIN, K8_IMIN, K8_IMIN, K8_IMIN, }},
  };
#define vec8_store_nta_partial_hi(p,x,n)                \
  (_mm256_maskstore_pd(&(p),k8store_hi_union[n].vi,x))



// Functions and operators

static const k8const_t k8sign_mask_union =
  {{ K8_IMIN, K8_IMIN, K8_IMIN, K8_IMIN, }};
static const k8const_t k8abs_mask_union =
  {{ K8_IMAX, K8_IMAX, K8_IMAX, K8_IMAX, }};

// Operators
#define k8pos(x) (x)
#define k8neg(x) (_mm256_xor_pd(x,k8sign_mask_union.vd))

#define k8add(x,y) (_mm256_add_pd(x,y))
#define k8sub(x,y) (_mm256_sub_pd(x,y))
#define k8mul(x,y) (_mm256_mul_pd(x,y))
#define k8div(x,y) (_mm256_div_pd(x,y))

// Fused multiply-add, defined as [+-]x*y[+-]z
#define k8madd(x,y,z)  (k8add(k8mul(x,y),z))
#define k8msub(x,y,z)  (k8sub(k8mul(x,y),z))
#define k8nmadd(x,y,z) (k8sub(k8neg(z),k8mul(x,y)))
#define k8nmsub(x,y,z) (k8sub(z,k8mul(x,y)))

// Cheap functions
#define k8fabs(x)   (_mm256_and_pd(x,k8abs_mask_union.vd))
#define k8fmax(x,y) (_mm256_max_pd(x,y))
#define k8fmin(x,y) (_mm256_min_pd(x,y))
#define k8fnabs(x)  (_mm256_or_pd(x,k8sign_mask_union.vd))
#define k8sqrt(x)   (_mm256_sqrt_pd(x))

// Expensive functions
#define K8REPL(x,func)                          \
({                                              \
  CCTK_REAL8_VEC const xfunc=(x);               \
  vec8_set((vec8_elt0(xfunc)),                  \
           (vec8_elt1(xfunc)),                  \
           (vec8_elt2(xfunc)),                  \
           (vec8_elt3(xfunc)));                 \
})
#define K8REPL2(x,a,func)                       \
({                                              \
  CCTK_REAL8_VEC const xfunc=(x);               \
  CCTK_REAL8     const afunc=(a);               \
  vec8_set((vec8_elt0(xfunc),afunc),            \
           (vec8_elt1(xfunc),afunc),            \
           (vec8_elt2(xfunc),afunc),            \
           (vec8_elt3(xfunc),afunc));           \
})
#define k8exp(x) K8REPL(x,exp)
#define k8log(x) K8REPL(x,log)
#define k8pow(x,a) K8REPL2(x,a,exp)
