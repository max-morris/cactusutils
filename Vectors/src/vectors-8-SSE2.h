// Vectorise using Intel's or AMD's SSE2

// Use the type __m128d directly, without introducing a wrapper class
// Use macros instead of inline functions



#include <assert.h>
#include <math.h>

#include <emmintrin.h>
#ifdef __SSE4_1__
// Intel's SSE 4.1
#  include <smmintrin.h>
#endif
#ifdef __SSE4A__
// AMD's SSE 4a
#  include <ammintrin.h>
#endif



#ifdef __SSE4_1__
#define vec8_architecture "SSE4.1 (64-bit precision)"
#elif defined(__SSE4A__)
#define vec8_architecture "SSE4A (64-bit precision)"
#else
#define vec8_architecture "SSE2 (64-bit precision)"
#endif

// Vector type corresponding to CCTK_REAL
#define CCTK_REAL8_VEC __m128d

// Number of vector elements in a CCTK_REAL_VEC
#define CCTK_REAL8_VEC_SIZE 2



// Create vectors, extract vector elements

#define vec8_set1(a)  (_mm_set1_pd(a))
#define vec8_set(a,b) (_mm_set_pd(b,a)) // note reversed arguments

// original order is 01
#define vec8_swap10(x_)                         \
  ({                                            \
    CCTK_REAL8_VEC const xx=(x_);               \
    CCTK_REAL8_VEC const x=xx;                  \
    _mm_shuffle_pd(x,x, _MM_SHUFFLE2(0,1));     \
  })

#define vec8_elt0(x) (((CCTK_REAL8 const*)&(x))[0])
#define vec8_elt1(x) (((CCTK_REAL8 const*)&(x))[1])
#define vec8_elt(x,d) (((CCTK_REAL8 const*)&(x))[d])



// Load and store vectors

// Load a vector from memory (aligned and unaligned); this loads from
// a reference to a scalar
#define vec8_load(p)  (_mm_load_pd(&(p)))
#define vec8_loadu(p) (_mm_loadu_pd(&(p)))
#if ! VECTORISE_ALWAYS_USE_ALIGNED_LOADS
#  define vec8_load_off1(p) vec_loadu(p)
#else
#  define vec8_load_off1(p_)                                    \
  ({                                                            \
    CCTK_REAL8 const& pp=(p_);                                  \
    CCTK_REAL8 const& p=pp;                                     \
    _mm_shuffle_pd(vec8_load((&p)[-1]),                         \
                   vec8_load((&p)[+1]), _MM_SHUFFLE2(0,1));     \
  })
#endif

// Load a vector from memory that may or may not be aligned, as
// decided by the offset off and the vector size
#if VECTORISE_ALWAYS_USE_UNALIGNED_LOADS
// Implementation: Always use unaligned load
#  define vec8_loadu_maybe(off,p)             vec8_loadu(p)
#  define vec8_loadu_maybe3(off1,off2,off3,p) vec8_loadu(p)
#else
#  define vec8_loadu_maybe(off,p_)              \
  ({                                            \
    CCTK_REAL8 const& pp=(p_);                  \
    CCTK_REAL8 const& p=pp;                     \
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
    CCTK_REAL8 const& pp=(p_);                          \
    CCTK_REAL8 const& p=pp;                             \
    ((off2) % CCTK_REAL8_VEC_SIZE != 0 or               \
     (off3) % CCTK_REAL8_VEC_SIZE != 0) ?               \
      vec8_loadu(p) :                                   \
      vec8_loadu_maybe(off1,p);                         \
  })
#  endif
#endif

// Store a vector to memory (aligned and non-temporal); this stores to
// a reference to a scalar
#define vec8_store(p,x)  (_mm_store_pd(&(p),x))
#define vec8_storeu(p,x) (_mm_storeu_pd(&(p),x))
#if ! VECTORISE_STREAMING_STORES
#  define vec8_store_nta(p,x) vec8_store(p,x)
#else
#  define vec8_store_nta(p,x) (_mm_stream_pd(&(p),x))
#endif

// Store a lower or higher partial vector (aligned and non-temporal)
#if ! VECTORISE_STREAMING_STORES
#  define vec8_store_nta_partial_lo(p,x,n) (_mm_storel_pd(&(p),x))
#  define vec8_store_nta_partial_hi(p,x,n) (_mm_storeh_pd(&(p)+1,x))
#else
#  if defined(__SSE4A__)
#    define vec8_store_nta_partial_lo(p,x,n) (_mm_stream_sd(&(p),x))
#    define vec8_store_nta_partial_hi(p,x,n)    \
  (_mm_stream_sd(&(p)+1, vec8_swap10(x)))
#  else
// TODO: use clflush once a whole cache line has been written (cache
// lines are usually larger than the CPU vector size)
#    define vec8_store_nta_partial_lo(p_,x,n)   \
  ({                                            \
    CCTK_REAL8& pp=(p_);                        \
    CCTK_REAL8& p=pp;                           \
    _mm_storel_pd(&p,x);                        \
    /* _mm_clflush(&p); */                      \
  })
#    define vec8_store_nta_partial_hi(p_,x,n)   \
  ({                                            \
    CCTK_REAL8& pp=(p_);                        \
    CCTK_REAL8& p=pp;                           \
    _mm_storeh_pd(&p+1,x);                      \
    /* _mm_clflush(&p+1); */                    \
  })
#  endif
#endif
#if 0
// This is slower; we would need a non-temporal read
#define vec8_store_nta_partial_lo(p,x,n)        \
  vec8_store_nta(p, _mm_loadh_pd(x,&(p)+1))
#define vec8_store_nta_partial_hi(p,x,n)        \
  vec8_store_nta(p, _mm_loadl_pd(x,&(p)))
#endif
#define vec8_store_nta_partial_mid(p,x,nlo,nhi) assert(0)



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
#define k8abs_mask (k8abs_mask_union.v)

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
#define K8REPL(f,x_)                            \
  ({                                            \
    CCTK_REAL8_VEC const xx=(x_);               \
    CCTK_REAL8_VEC const x=xx;                  \
    vec8_set(f(vec8_elt0(x)),                   \
             f(vec8_elt1(x)));                  \
  })
#define K8REPL2(f,x_,a_)                        \
  ({                                            \
    CCTK_REAL8_VEC const xx=(x_);               \
    CCTK_REAL8_VEC const x=xx;                  \
    CCTK_REAL8     const aa=(a_);               \
    CCTK_REAL8     const a=aa;                  \
    vec8_set(f(vec8_elt0(x),a),                 \
             f(vec8_elt1(x),a));                \
  })

#define k8exp(x)   K8REPL(exp,x)
#define k8log(x)   K8REPL(log,x)
#define k8pow(x,a) K8REPL2(pow,x,a)

// Choice   [sign(x)>0 ? y : z]
#ifdef __SSE4_1__
#  define k8ifpos(x,y,z) (_mm_blendv_pd(y,z,x))
#elif 0
#  define k8ifpos(x_,y_,z_)                     \
  ({                                            \
    CCTK_REAL8_VEC const xx=(x_);               \
    CCTK_REAL8_VEC const x=xx;                  \
    CCTK_REAL8_VEC const yy=(y_);               \
    CCTK_REAL8_VEC const y=yy;                  \
    CCTK_REAL8_VEC const zz=(z_);               \
    CCTK_REAL8_VEC const z=zz;                  \
    int const m = _mm_movemask_pd(x);           \
    CCTK_REAL8_VEC r;                           \
    switch (m) {                                \
    case 0: r = y; break;                       \
    case 1: r = _mm_move_sd(y,z); break;        \
    case 2: r = _mm_move_sd(z,y); break;        \
    case 3: r = z; break;                       \
    }                                           \
    r;                                          \
  })
#else
#  ifdef __cplusplus
#    define k8sgn(x) ({ using namespace std; signbit(x); })
#  else
#    define k4sgn(x) (signbit(x))
#  endif
#  define k8ifpos(x_,y_,z_)                                             \
  ({                                                                    \
    CCTK_REAL8_VEC const xx=(x_);                                       \
    CCTK_REAL8_VEC const x=xx;                                          \
    CCTK_REAL8_VEC const yy=(y_);                                       \
    CCTK_REAL8_VEC const y=yy;                                          \
    CCTK_REAL8_VEC const zz=(z_);                                       \
    CCTK_REAL8_VEC const z=zz;                                          \
    vec8_set(k8sgn(vec8_elt0(x)) ? vec8_elt0(z) : vec8_elt0(y),   \
             k8sgn(vec8_elt1(x)) ? vec8_elt1(z) : vec8_elt1(y));  \
  })
#endif
