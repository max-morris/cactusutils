// Vectorise using Intel's or AMD's SSE

// Use the type __m128 directly, without introducing a wrapper class
// Use macros instead of inline functions



#include <assert.h>
#include <math.h>

#include <xmmintrin.h>
#ifdef __SSE4_1__
// Intel's SSE 4.1
#  include <smmintrin.h>
#endif
#ifdef __SSE4A__
// AMD's SSE 4a
#  include <ammintrin.h>
#endif



#ifdef __SSE4_1__
#  define vec4_architecture "SSE4.1 (32-bit precision)"
#elif defined(__SSE4A__)
#  define vec4_architecture "SSE4A (32-bit precision)"
#else
#  define vec4_architecture "SSE (32-bit precision)"
#endif

// Vector type corresponding to CCTK_REAL
#define CCTK_REAL4_VEC __m128

// Number of vector elements in a CCTK_REAL_VEC
#define CCTK_REAL4_VEC_SIZE 4



// Create vectors, extract vector elements

#define vec4_set1(a)      (_mm_set1_ps(a))
#define vec4_set(a,b,c,d) (_mm_set_ps(d,c,b,a)) // note reversed arguments

// original order is 0123
#define vec4_swap1032(x_)                       \
  ({                                            \
    CCTK_REAL4_VEC const xx=(x_);               \
    CCTK_REAL4_VEC const x=xx;                  \
    _mm_shuffle_ps(x,x, _MM_SHUFFLE(2,3,0,1));  \
  })
#define vec4_swap2301(x_)                       \
  ({                                            \
    CCTK_REAL4_VEC const xx=(x_);               \
    CCTK_REAL4_VEC const x=xx;                  \
    _mm_shuffle_ps(x,x, _MM_SHUFFLE(1,0,3,2));  \
  })
#define vec4_swap3210(x_)                       \
  ({                                            \
    CCTK_REAL4_VEC const xx=(x_);               \
    CCTK_REAL4_VEC const x=xx;                  \
    _mm_shuffle_ps(x,x, _MM_SHUFFLE(0,1,2,3));  \
  })

#if defined(__PGI)
// _mm_cvtss_f32 does not exist on PGI compilers
#  define vec4_elt0(x)                          \
  ({                                            \
    CCTK_REAL4 a;                               \
    asm ("" : "=x" (a) : "0" (x));              \
    a;                                          \
  })
#else
#  define vec4_elt0(x) (_mm_cvtss_f32(x)) // this is a no-op
#endif
#define vec4_elt1(x) vec4_elt0(vec4_swap1032(x))
#define vec4_elt2(x) vec4_elt0(vec4_swap2301(x))
#define vec4_elt3(x) vec4_elt0(vec4_swap3210(x))
#if defined(__PGI)
#  define vec4_elt(x_,d)                        \
  ({                                            \
    CCTK_REAL4_VEC const xx=(x_);               \
    CCTK_REAL4_VEC const x=xx;                  \
    CCTK_REAL4 a;                               \
    if (d==0)      a=vec4_elt0(x);              \
    else if (d==1) a=vec4_elt1(x);              \
    else if (d==2) a=vec4_elt2(x);              \
    else if (d==3) a=vec4_elt3(x);              \
    a;                                          \
  })
#else
#  define vec4_elt(x_,d)                        \
  ({                                            \
    CCTK_REAL4_VEC const xx=(x_);               \
    CCTK_REAL4_VEC const x=xx;                  \
    CCTK_REAL4 a;                               \
    switch (d) {                                \
    case 0: a=vec4_elt0(x); break;              \
    case 1: a=vec4_elt1(x); break;              \
    case 2: a=vec4_elt2(x); break;              \
    case 3: a=vec4_elt3(x); break;              \
    }                                           \
    a;                                          \
  })
#endif



// Load and store vectors

// Load a vector from memory (aligned and unaligned); this loads from
// a reference to a scalar
#define vec4_load(p)  (_mm_load_ps(&(p)))
#define vec4_loadu(p) (_mm_loadu_ps(&(p)))
#if ! VECTORISE_ALWAYS_USE_ALIGNED_LOADS
#  define vec4_load_off1(p) vec_loadu(p)
#  define vec4_load_off2(p) vec_loadu(p)
#  define vec4_load_off3(p) vec_loadu(p)
#else
#  define vec4_load_off1(p_)                                            \
  ({                                                                    \
    CCTK_REAL4 const& pp=(p_);                                          \
    CCTK_REAL4 const& p=pp;                                             \
    CCTK_REAL4_VEC const lo=vec4_load((&p)[-1]);                        \
    CCTK_REAL4_VEC const hi=vec4_load((&p)[+3]);                        \
    assert(0);                                                          \
    CCTK_REAL4_VEC const hi2=_mm_suffle_ps(lo,hi, _MM_SHUFFLE(0,1,2,3)); \
    _mm_shuffle_ps(lo,hi2, _MM_SHUFFLE(2,1,3,0));                       \
  })
#  define vec4_load_off2(p_)                            \
  ({                                                    \
    CCTK_REAL4 const& pp=(p_);                          \
    CCTK_REAL4 const& p=pp;                             \
    CCTK_REAL4_VEC const lo=vec4_load((&p)[-2]);        \
    CCTK_REAL4_VEC const hi=vec4_load((&p)[+2]);        \
    _mm_shuffle_ps(lo,hi, _MM_SHUFFLE(1,0,3,2));        \
  })
#  define vec4_load_off1(p_)                                            \
  ({                                                                    \
    CCTK_REAL4 const& pp=(p_);                                          \
    CCTK_REAL4 const& p=pp;                                             \
    CCTK_REAL4_VEC const lo=vec4_load((&p)[-1]);                        \
    CCTK_REAL4_VEC const hi=vec4_load((&p)[+3]);                        \
    assert(0);                                                          \
    CCTK_REAL4_VEC const lo2=_mm_suffle_ps(lo,hi, _MM_SHUFFLE(0,1,2,3)); \
    _mm_shuffle_ps(lo2,hi, _MM_SHUFFLE(3,0,2,1));                       \
  })
#endif

// Load a vector from memory that may or may not be aligned, as
// decided by the offset off and the vector size
#if VECTORISE_ALWAYS_USE_UNALIGNED_LOADS
// Implementation: Always use unaligned load
#  define vec4_loadu_maybe(off,p)             vec4_loadu(p)
#  define vec4_loadu_maybe3(off1,off2,off3,p) vec4_loadu(p)
#else
#  define vec4_loadu_maybe(off,p_)              \
  ({                                            \
    CCTK_REAL4 const& pp=(p_);                  \
    CCTK_REAL4 const& p=pp;                     \
    (off) % CCTK_REAL4_VEC_SIZE == 0 ?          \
      vec4_load(p) :                            \
      vec4_loadu(p);                            \
  })
#  if VECTORISE_ALIGNED_ARRAYS
// Assume all array x sizes are multiples of the vector size
#    define vec4_loadu_maybe3(off1,off2,off3,p) \
  vec4_loadu_maybe(off1,p)
#  else
#    define vec4_loadu_maybe3(off1,off2,off3,p) \
  vec4_loadu_maybe((off1)|(off2)|(off3),p)
#  endif
#endif

// Store a lower or higher partial vector (aligned and non-temporal);
// the non-temporal hint is probably ignored
#if ! VECTORISE_STREAMING_STORES || ! defined(__SSE4A__)
#  define vec4_store_nta_partial_lo(p_,x_,n)                    \
  ({                                                            \
    CCTK_REAL4 const& pp=(p_);                                  \
    CCTK_REAL4 const& p=pp;                                     \
    CCTK_REAL4_VEC const xx=(x_);                               \
    CCTK_REAL4_VEC const x=xx;                                  \
    switch (n) {                                                \
    case 1: (&p)[0]=vec4_elt0(x); break;                        \
    case 2: _mm_storel_ps(&p,x); break;                         \
    case 3: _mm_storel_ps(&p,x); (&p)[2]=vec4_elt2(x); break;   \
    }                                                           \
  })
#  define vec4_store_nta_partial_hi(p_,x_,n)                    \
  ({                                                            \
    CCTK_REAL4 const& pp=(p_);                                  \
    CCTK_REAL4 const& p=pp;                                     \
    CCTK_REAL4_VEC const xx=(x_);                               \
    CCTK_REAL4_VEC const x=xx;                                  \
    switch (n) {                                                \
    case 1: (&p)[3]=vec4_elt3(x);  break;                       \
    case 2: _mm_storeh_ps(&p+2,x); break;                       \
    case 3: _mm_storeh_ps(&p+2,x); (&p)[1]=vec4_elt1(x); break; \
    }                                                           \
  })
#else
#  define vec4_store_nta_partial_lo(p_,x_,n)    \
  ({                                            \
    CCTK_REAL4 const& pp=(p_);                  \
    CCTK_REAL4 const& p=pp;                     \
    CCTK_REAL4_VEC const xx=(x_);               \
    CCTK_REAL4_VEC const x=xx;                  \
    switch (n) {                                \
    case 1:                                     \
      _mm_stream_ss(&p,x);                      \
      break;                                    \
    case 2:                                     \
      _mm_storel_ps(&p,x);                      \
      break;                                    \
    case 3:                                     \
      _mm_storel_ps(&p,x);                      \
      _mm_stream_ss(&p+2, vec4_swap2301(x));    \
      break;                                    \
    }                                           \
  })
#  define vec4_store_nta_partial_hi(p_,x_,n)    \
  ({                                            \
    CCTK_REAL4 const& pp=(p_);                  \
    CCTK_REAL4 const& p=pp;                     \
    CCTK_REAL4_VEC const xx=(x_);               \
    CCTK_REAL4_VEC const x=xx;                  \
    switch (n) {                                \
    case 1:                                     \
      _mm_stream_ss(&p+3, vec4_swap3210(x));    \
      break;                                    \
    case 2:                                     \
      _mm_storeh_ps(&p+2,x);                    \
      break;                                    \
    case 3:                                     \
      _mm_storeh_ps(&p+2,x);                    \
      _mm_stream_ss(&p+1, vec4_swap1032(x));    \
      break;                                    \
    }                                           \
  })
#endif



// Functions and operators

static const union {
  unsigned i[4];
  __m128   v;
} k4sign_mask_union = {{ 0x80000000U, 0x80000000U, 0x80000000U, 0x80000000U }};
#define k4sign_mask (k4sign_mask_union.v)
static const union {
  unsigned i[4];
  __m128   v;
} k4abs_mask_union = {{ 0x7fffffffU, 0x7fffffffU, 0x7fffffffU, 0x7fffffffU }};
#define k4abs_mask (k4abs_mask_union.v)

// Operators
#define k4pos(x) (x)
#define k4neg(x) (_mm_xor_ps(x,k4sign_mask))
// #define k4inv(x)
// TODO: provide k4inv via rcp and Newton-Raphson
// This is described in AMD's publication 47414.
// This should apply for AVX as well.

#define k4add(x,y) (_mm_add_ps(x,y))
#define k4sub(x,y) (_mm_sub_ps(x,y))
#define k4mul(x,y) (_mm_mul_ps(x,y))
// TODO: use k4inv and k4mul instead
#define k4div(x,y) (_mm_div_ps(x,y))

// Fused multiply-add, defined as [+-]x*y[+-]z
#define k4madd(x,y,z)  (k4add(k4mul(x,y),z))
#define k4msub(x,y,z)  (k4sub(k4mul(x,y),z))
#define k4nmadd(x,y,z) (k4sub(k4neg(z),k4mul(x,y)))
#define k4nmsub(x,y,z) (k4sub(z,k4mul(x,y)))

// Cheap functions
#define k4fabs(x)   (_mm_and_ps(x,k4abs_mask))
#define k4fmax(x,y) (_mm_max_ps(x,y))
#define k4fmin(x,y) (_mm_min_ps(x,y))
#define k4fnabs(x)  (_mm_or_ps(x,k4sign_mask))
// TODO: maybe use rsqrt and Newton-Raphson
#define k4sqrt(x)   (_mm_sqrt_ps(x))

// Expensive functions
#define K4REPL(f,x_)                            \
  ({                                            \
    CCTK_REAL4_VEC const xx=(x_);               \
    CCTK_REAL4_VEC const x=xx;                  \
    vec4_set(f(vec4_elt0(x)),                   \
             f(vec4_elt1(x)),                   \
             f(vec4_elt2(x)),                   \
             f(vec4_elt3(x)));                  \
  })
#define K4REPL2(f,x_,a_)                        \
  ({                                            \
    CCTK_REAL4_VEC const xx=(x_);               \
    CCTK_REAL4_VEC const x=xx;                  \
    CCTK_REAL4     const aa=(a_);               \
    CCTK_REAL4     const a=aa;                  \
    vec4_set(f(vec4_elt0(x),a),                 \
             f(vec4_elt1(x),a),                 \
             f(vec4_elt2(x),a),                 \
             f(vec4_elt3(x),a));                \
  })

#define k4exp(x)   K4REPL(exp,x)
#define k4log(x)   K4REPL(log,x)
#define k4pow(x,a) K4REPL2(pow,x,a)

// Choice   [sign(x)>0 ? y : z]
#ifdef __SSE4_1__
#  define k4ifpos(x,y,z) (_mm_blendv_ps(y,z,x))
#else
#  ifdef __cplusplus
#    define k4sgn(x) ({ using namespace std; signbit(x); })
#  else
#    define k4sgn(x) (signbit(x))
#  endif
#  define k4ifpos(x,y,z)                                                \
  ({                                                                    \
    CCTK_REAL4_VEC const xx=(x_);                                       \
    CCTK_REAL4_VEC const x=xx;                                          \
    CCTK_REAL4_VEC const yy=(y_);                                       \
    CCTK_REAL4_VEC const y=yy;                                          \
    CCTK_REAL4_VEC const zz=(z_);                                       \
    CCTK_REAL4_VEC const z=zz;                                          \
    vec4_set(k4sgn(vec4_elt0(x)) ? vec4_elt0(z) : vec4_elt0(y),   \
             k4sgn(vec4_elt1(x)) ? vec4_elt1(z) : vec4_elt1(y),   \
             k4sgn(vec4_elt2(x)) ? vec4_elt2(z) : vec4_elt2(y),   \
             k4sgn(vec4_elt3(x)) ? vec4_elt3(z) : vec4_elt3(y));  \
  })
#endif
