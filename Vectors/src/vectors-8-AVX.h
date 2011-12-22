// Vectorise using Intel's or AMD's AVX

// Use the type __m256d directly, without introducing a wrapper class
// Use macros instead of inline functions



#if VECTORISE_EMULATE_AVX
#  include "avxintrin_emu.h"
#else
#  include <immintrin.h>
#endif
#ifdef __FMA4__
#  include <fma4intrin.h>
#endif



#ifdef __FMA4__
#  define vec8_architecture_FMA4 "+FMA4"
#else
#  define vec8_architecture_FMA4 ""
#endif
#define vec8_architecture "AVX" vec8_architecture_FMA4 " (64-bit precision)"



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

#define vec8_elt0(x) (((CCTK_REAL8 const*)&(x))[0])
#define vec8_elt1(x) (((CCTK_REAL8 const*)&(x))[1])
#define vec8_elt2(x) (((CCTK_REAL8 const*)&(x))[2])
#define vec8_elt3(x) (((CCTK_REAL8 const*)&(x))[3])
#define vec8_elt(x,d) (((CCTK_REAL8 const*)&(x))[d])



// Load and store vectors

// Load a vector from memory (aligned and unaligned); this loads from
// a reference to a scalar
#define vec8_load(p)  (_mm256_load_pd(&(p)))
#define vec8_loadu(p) (_mm256_loadu_pd(&(p)))
#if ! VECTORISE_ALWAYS_USE_ALIGNED_LOADS
#  define vec8_load_off1(p) vec_loadu(p)
#else
#  error "VECTORISE_ALWAYS_USE_ALIGNED_LOADS not yet supported"
#endif

// Load a vector from memory that may or may not be aligned, as
// decided by the offset off and the vector size
#if VECTORISE_ALWAYS_USE_UNALIGNED_LOADS
// Implementation: Always use unaligned load
#  define vec8_loadu_maybe(off,p)             (vec8_loadu(p))
#  define vec8_loadu_maybe3(off1,off2,off3,p) (vec8_loadu(p))
#else
#  define vec8_loadu_maybe(off,p_)              \
  ({                                            \
    CCTK_REAL8 const& p__=(p_);                 \
    CCTK_REAL8 const& p=p__;                    \
    (off) % CCTK_REAL8_VEC_SIZE == 0 ?          \
      vec8_load(p) :                            \
      vec8_load_off1(p);                        \
  })
#  if VECTORISE_ALIGNED_ARRAYS
// Assume all array x sizes are multiples of the vector size
#    define vec8_loadu_maybe3(off1,off2,off3,p) \
  vec8_loadu_maybe(off1,p)
#  else
#    define vec8_loadu_maybe3(off1,off2,off3,p_)        \
  ({                                                    \
    CCTK_REAL8 const& p__=(p_);                         \
    CCTK_REAL8 const& p=p__;                            \
    ((off2) % CCTK_REAL8_VEC_SIZE != 0 or               \
     (off3) % CCTK_REAL8_VEC_SIZE != 0) ?               \
      vec8_loadu(p) :                                   \
      vec8_loadu_maybe(off1,p);                         \
  })
#  endif
#endif

// Store a vector to memory (aligned and non-temporal); this stores to
// a reference to a scalar
#define vec8_store(p,x)  (_mm256_store_pd(&(p),x))
#define vec8_storeu(p,x) (_mm256_storeu_pd(&(p),x))
#if ! VECTORISE_STREAMING_STORES
#  define vec8_store_nta(p,x) (vec8_store(p,x))
#else
#  define vec8_store_nta(p,x) (_mm256_stream_pd(&(p),x))
#endif

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
static const k8const_t k8store_hi_union[5] =
  {
    {{ K8_ZERO, K8_ZERO, K8_ZERO, K8_ZERO, }},
    {{ K8_ZERO, K8_ZERO, K8_ZERO, K8_IMIN, }},
    {{ K8_ZERO, K8_ZERO, K8_IMIN, K8_IMIN, }},
    {{ K8_ZERO, K8_IMIN, K8_IMIN, K8_IMIN, }},
    {{ K8_IMIN, K8_IMIN, K8_IMIN, K8_IMIN, }},
  };
#if defined(__GNUC__) && __GNUC__==4 && __GNUC_MINOR__<=4
// gcc 4.4 uses a wrong prototype for _mm256_maskstore_pd
#  define vec8_store_nta_partial_lo(p,x,n)                              \
  (_mm256_maskstore_pd(&(p),_mm256_castsi256_pd(k8store_lo_union[n].vi),x))
#  define vec8_store_nta_partial_hi(p,x,n)                              \
  (_mm256_maskstore_pd(&(p),_mm256_castsi256_pd(k8store_hi_union[n].vi),x))
#  define vec8_store_nta_partial_mid(p,x,nlo,nhi)                       \
  (_mm256_maskstore_pd                                                  \
   (&(p),                                                               \
    _mm256_castsi256_pd(k8store_lo_union[nlo].vi & k8store_hi_union[nhi].vi), \
    x))
#else
#  define vec8_store_nta_partial_lo(p,x,n)              \
  (_mm256_maskstore_pd(&(p),k8store_lo_union[n].vi,x))
#  define vec8_store_nta_partial_hi(p,x,n)              \
  (_mm256_maskstore_pd(&(p),k8store_hi_union[n].vi,x))
#  define vec8_store_nta_partial_mid(p,x,nlo,nhi)               \
  (_mm256_maskstore_pd                                          \
   (&(p),                                                       \
    k8store_lo_union[nlo].vi & k8store_hi_union[nhi].vi,        \
    x))
#endif



// Functions and operators

static const k8const_t k8sign_mask_union =
  {{ K8_IMIN, K8_IMIN, K8_IMIN, K8_IMIN, }};
static const k8const_t k8abs_mask_union =
  {{ K8_IMAX, K8_IMAX, K8_IMAX, K8_IMAX, }};

// Operators
#define k8neg(x) (_mm256_xor_pd(x,k8sign_mask_union.vd))

#define k8add(x,y) (_mm256_add_pd(x,y))
#define k8sub(x,y) (_mm256_sub_pd(x,y))
#define k8mul(x,y) (_mm256_mul_pd(x,y))
#define k8div(x,y) (_mm256_div_pd(x,y))

// Fused multiply-add, defined as [+-]x*y[+-]z
#ifdef __FMA4__
#  define k8madd(x,y,z)  (_mm256_macc_pd(x,y,z))
#  define k8msub(x,y,z)  (_mm256_msub_pd(x,y,z))
#  define k8nmadd(x,y,z) (_mm256_nmsub_pd(x,y,z))
#  define k8nmsub(x,y,z) (_mm256_nmacc_pd(x,y,z))
#else
#  define k8madd(x,y,z)  (k8add(k8mul(x,y),z))
#  define k8msub(x,y,z)  (k8sub(k8mul(x,y),z))
#  define k8nmadd(x,y,z) (k8sub(k8neg(z),k8mul(x,y)))
#  define k8nmsub(x,y,z) (k8sub(z,k8mul(x,y)))
#endif

// Cheap functions
#define k8fabs(x)   (_mm256_and_pd(x,k8abs_mask_union.vd))
#define k8fmax(x,y) (_mm256_max_pd(x,y))
#define k8fmin(x,y) (_mm256_min_pd(x,y))
#define k8fnabs(x)  (_mm256_or_pd(x,k8sign_mask_union.vd))
#define k8sqrt(x)   (_mm256_sqrt_pd(x))

// Expensive functions
#define K8REPL(f,x)                             \
({                                              \
  CCTK_REAL8_VEC const xfunc=(x);               \
  vec8_set(f(vec8_elt0(xfunc)),                 \
           f(vec8_elt1(xfunc)),                 \
           f(vec8_elt2(xfunc)),                 \
           f(vec8_elt3(xfunc)));                \
})
#define K8REPL2(f,x,a)                          \
({                                              \
  CCTK_REAL8_VEC const xfunc=(x);               \
  CCTK_REAL8     const afunc=(a);               \
  vec8_set(f(vec8_elt0(xfunc),afunc),           \
           f(vec8_elt1(xfunc),afunc),           \
           f(vec8_elt2(xfunc),afunc),           \
           f(vec8_elt3(xfunc),afunc));          \
})
#define k8exp(x)   K8REPL(exp,x)
#define k8log(x)   K8REPL(log,x)
#define k8pow(x,a) K8REPL2(pow,x,a)

#define k8ifpos(x,y,z) (_mm256_blendv_pd(y,z,x))
