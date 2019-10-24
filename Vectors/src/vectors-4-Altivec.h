// -*-C++-*-
// Vectorise using IBM's Altivec (Power)

// Use the type vector float directly, without introducing a wrapper class

#include <altivec.h>

#define vec4_architecture "Altivec"

// Vector type corresponding to CCTK_REAL
typedef vector float CCTK_REAL4_VEC;
typedef vector signed int CCTK_INTEGER4_VEC;
typedef vector bool int CCTK_BOOLEAN4_VEC;

// Number of vector elements in a CCTK_REAL_VEC
#define CCTK_REAL4_VEC_SIZE 4

vec_static_assert(sizeof(CCTK_REAL4_VEC) ==
                  sizeof(CCTK_REAL4) * CCTK_REAL4_VEC_SIZE);

// Integer and boolean types corresponding to this real type
typedef int CCTK_INTEGER4;
typedef unsigned int CCTK_BOOLEAN4;

// Create vectors, extract vector elements

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL4_VEC
vec4_set1(CCTK_REAL4 a) {
  return vec_splats(a);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_INTEGER4_VEC
vec4_set1i(CCTK_INT4 a) {
  return vec_splats(a);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL4_VEC
vec4_set(CCTK_REAL4 a0, CCTK_REAL4 a1, CCTK_REAL4 a2, CCTK_REAL4 a3) {
  CCTK_REAL4_VEC x;
  x[0] = a0;
  x[1] = a1;
  x[2] = a2;
  x[3] = a3;
  return x;
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL4
vec4_elt0(CCTK_REAL4_VEC x) {
  return x[0];
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL4
vec4_elt1(CCTK_REAL4_VEC x) {
  return x[1];
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL4
vec4_elt2(CCTK_REAL4_VEC x) {
  return x[2];
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL4
vec4_elt3(CCTK_REAL4_VEC x) {
  return x[3];
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL4 vec4_elt(CCTK_REAL4_VEC x,
                                                               int d) {
  return x[d];
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_INTEGER4
vec4_elti(CCTK_INTEGER4_VEC x, int d) {
  return x[d];
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN4
vec4_eltb(CCTK_BOOLEAN4_VEC x, int d) {
  return x[d];
}

// Load and store vectors

// Load a vector from memory (aligned and unaligned); this loads from
// a reference to a scalar
#define vec4_load(p) (*(CCTK_REAL4_VEC const *)&(p))
#define vec4_loadu(p) (*(CCTK_REAL4_VEC const *)&(p))

// Load a vector from memory that may or may not be aligned, as
// decided by the offset and the vector size
#define vec4_loadu_maybe(off, p) (vec4_loadu(p))
#define vec4_loadu_maybe3(off1, off2, off3, p) (vec4_loadu(p))

// Store a vector to memory (aligned and non-temporal); this stores to
// a reference to a scalar
#define vec4_store(p, x) (*(CCTK_REAL4_VEC *)&(p) = (x))
#define vec4_storeu(p, x) (*(CCTK_REAL4_VEC *)&(p) = (x))
#if !VECTORISE_STREAMING_STORES
#define vec4_store_nta(p, x) (vec4_store(p, x))
#else
// use stvxl instruction
#define vec4_store_nta(p, x) (vec_stl(x, 0, (CCTK_REAL4_VEC *)&(p)))
#endif

// Store a partial vector (aligned and non-temporal)
#define vec4_store_partial_prepare_fixed(i, imin, imax)                        \
  vec4_store_partial_prepare(i, imin, imax)

#define vec4_store_partial_prepare(i, imin, imax)                              \
  std::ptrdiff_t v4stp_lo_skip, v4stp_hi_skip;                                 \
  vec4_store_partial_prepare_(v4stp_lo_skip, v4stp_hi_skip, i, imin, imax)
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
vec4_store_partial_prepare_(std::ptrdiff_t &lo_skip, std::ptrdiff_t &hi_skip,
                            std::ptrdiff_t const i, std::ptrdiff_t const imin,
                            std::ptrdiff_t const imax) {
  lo_skip = std::max(std::ptrdiff_t(0), imin - i);
  hi_skip = std::max(std::ptrdiff_t(0), i + CCTK_REAL4_VEC_SIZE - imax);
}
#define vec4_store_nta_partial(p, x)                                           \
  vec4_store_nta_partial_(v4stp_lo_skip, v4stp_hi_skip, p, x)
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
vec4_store_nta_partial_(std::ptrdiff_t const lo_skip,
                        std::ptrdiff_t const hi_skip, CCTK_REAL4 &p,
                        CCTK_REAL4_VEC const x) {
  // if (CCTK_BUILTIN_EXPECT(lo_skip == 0 and hi_skip == 0, true)) {
  if (lo_skip == 0 and hi_skip == 0) {
    vec4_store_nta(p, x);
  } else {
    // these cases fall through
    switch (lo_skip) {
    case 0:
      (&p)[0] = vec4_elt(x, 0);
    case 1:
      if (hi_skip >= 3)
        break;
      (&p)[1] = vec4_elt(x, 1);
    case 2:
      if (hi_skip >= 2)
        break;
      (&p)[2] = vec4_elt(x, 2);
    case 3:
      if (hi_skip >= 1)
        break;
      (&p)[3] = vec4_elt(x, 3);
    }
  }
}

#define vec4_storeu_partial(p, x)                                              \
  vec4_storeu_partial_(v4stp_lo_skip, v4stp_hi_skip, p, x)
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
vec4_storeu_partial_(std::ptrdiff_t const lo_skip, std::ptrdiff_t const hi_skip,
                     CCTK_REAL4 &p, CCTK_REAL4_VEC const x) {
  // if (CCTK_BUILTIN_EXPECT(lo_skip == 0 and hi_skip == 0, true)) {
  if (lo_skip == 0 and hi_skip == 0) {
    vec4_storeu(p, x);
  } else {
    // these cases fall through
    switch (lo_skip) {
    case 0:
      (&p)[0] = vec4_elt(x, 0);
    case 1:
      if (hi_skip >= 3)
        break;
      (&p)[1] = vec4_elt(x, 1);
    case 2:
      if (hi_skip >= 2)
        break;
      (&p)[2] = vec4_elt(x, 2);
    case 3:
      if (hi_skip >= 1)
        break;
      (&p)[3] = vec4_elt(x, 3);
    }
  }
}

// Store a lower or higher partial vector (aligned and non-temporal);
// the non-temporal hint is probably ignored
#define vec4_store_nta_partial_lo(p_, x_, n)                                   \
  ({                                                                           \
    CCTK_REAL4 const &p__ = (p_);                                              \
    CCTK_REAL4_VEC const x__ = (x_);                                           \
    CCTK_REAL4 const &p = p__;                                                 \
    CCTK_REAL4_VEC const x = x__;                                              \
    switch (n) {                                                               \
    case 3:                                                                    \
      (&p)[2] = x[2];                                                          \
    case 2:                                                                    \
      (&p)[1] = x[1];                                                          \
    case 1:                                                                    \
      (&p)[0] = x[0];                                                          \
    }                                                                          \
  })
#define vec4_store_nta_partial_hi(p_, x_, n)                                   \
  ({                                                                           \
    CCTK_REAL4 const &p__ = (p_);                                              \
    CCTK_REAL4_VEC const x__ = (x_);                                           \
    CCTK_REAL4 const &p = p__;                                                 \
    CCTK_REAL4_VEC const x = x__;                                              \
    switch (n) {                                                               \
    case 3:                                                                    \
      (&p)[1] = x[1];                                                          \
    case 2:                                                                    \
      (&p)[2] = x[2];                                                          \
    case 1:                                                                    \
      (&p)[3] = x[3];                                                          \
    }                                                                          \
  })
#define vec4_store_nta_partial_mid(p_, x_, nlo_, nhi_)                         \
  ({                                                                           \
    CCTK_REAL4 const &p__ = (p_);                                              \
    CCTK_REAL4_VEC const x__ = (x_);                                           \
    int const nlo__ = (nlo_);                                                  \
    int const nhi__ = (nhi_);                                                  \
    CCTK_REAL4 const &p = p__;                                                 \
    CCTK_REAL4_VEC const x = x__;                                              \
    int const nlo = nlo__;                                                     \
    int const nhi = nhi__;                                                     \
    if (nlo == 3 and nhi == 3) {                                               \
      (&p)[1] = x[1];                                                          \
      (&p)[2] = x[2];                                                          \
    } else if (nlo == 2 and nhi == 3) {                                        \
      (&p)[1] = x[1];                                                          \
    } else if (nlo == 3 and nhi == 2) {                                        \
      (&p)[2] = x[2];                                                          \
    }                                                                          \
  })

// Functions and operators

// Operators
#define k4neg(x) (-(x))

#define k4add(x, y) ((x) + (y))
#define k4sub(x, y) ((x) - (y))
#define k4mul(x, y) ((x) * (y))
#define k4div(x, y) ((x) / (y))

// Fused multiply-add, defined as [+-]x*y[+-]z
#define k4madd(x, y, z) (vec_madd(x, y, z))
#define k4msub(x, y, z) (vec_msub(x, y, z))
#define k4nmadd(x, y, z) (vec_nmadd(x, y, z))
#define k4nmsub(x, y, z) (vec_nmsub(x, y, z))

// Cheap functions
#define k4copysign(x, y) (vec_cpsgn(y, x))
#define k4fabs(x) (vec_abs(x))
#define k4fmax(x, y) (vec_max(x, y))
#define k4fmin(x, y) (vec_min(x, y))
#define k4fnabs(x) (vec_nabs(x))
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN4_VEC
k4signbit(CCTK_REAL4_VEC const x) {
  return vec_cmplt((CCTK_INTEGER4_VEC)x, vec4_set1i(0));
}
#define k4sqrt(x) (vec_sqrt(x))

// Reduction operations

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bool
k4all(CCTK_BOOLEAN4_VEC const x) {
  return (x[0] & x[1]) & (x[2] & x[3]);
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bool
k4any(CCTK_BOOLEAN4_VEC const x) {
  return (x[0] | x[1]) | (x[2] | x[3]);
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL4
k4maximum(CCTK_REAL4_VEC const x) {
  return fmax(fmax(x[0], x[1]), fmax(x[2], x[3]));
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL4
k4minimum(CCTK_REAL4_VEC const x) {
  return fmin(fmin(x[0], x[1]), fmin(x[2], x[3]));
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL4
k4sum(CCTK_REAL4_VEC const x) {
  return (x[0] + x[1]) + (x[2] + x[3]);
}
// Expensive functions
#define K4REPL(f, x_)                                                          \
  ({                                                                           \
    CCTK_REAL4_VEC const x__ = (x_);                                           \
    CCTK_REAL4_VEC const x = x__;                                              \
    vec4_set(f(vec4_elt0(x)), f(vec4_elt1(x)), f(vec4_elt2(x)),                \
             f(vec4_elt3(x)));                                                 \
  })
#define K4REPL2S(f, x_, a_)                                                    \
  ({                                                                           \
    CCTK_REAL4_VEC const x__ = (x_);                                           \
    CCTK_REAL4 const a__ = (a_);                                               \
    CCTK_REAL4_VEC const x = x__;                                              \
    CCTK_REAL4 const a = a__;                                                  \
    vec4_set(f(vec4_elt0(x), a), f(vec4_elt1(x), a), f(vec4_elt2(x), a),       \
             f(vec4_elt3(x), a));                                              \
  })
#define K4REPL2I(f, x_, i_)                                                    \
  ({                                                                           \
    CCTK_REAL4_VEC const x__ = (x_);                                           \
    CCTK_INT4 const i__ = (i_);                                                \
    CCTK_REAL4_VEC const x = x__;                                              \
    CCTK_INT4 const i = i__;                                                   \
    vec4_set(f(vec4_elt0(x), i), f(vec4_elt1(x), i), f(vec4_elt2(x), i),       \
             f(vec4_elt3(x), i));                                              \
  })
#define K4REPL2(f, x_, y_)                                                     \
  ({                                                                           \
    CCTK_REAL4_VEC const x__ = (x_);                                           \
    CCTK_REAL4_VEC const y__ = (y_);                                           \
    CCTK_REAL4_VEC const x = x__;                                              \
    CCTK_REAL4_VEC const y = y__;                                              \
    vec4_set(f(vec4_elt0(x), vec4_elt0(y)), f(vec4_elt1(x), vec4_elt1(y)),     \
             f(vec4_elt2(x), vec4_elt2(y)), f(vec4_elt3(x), vec4_elt3(y)));    \
  })

#define k4acos(x) K4REPL(acosf, x)
#define k4acosh(x) K4REPL(acoshf, x)
#define k4asin(x) K4REPL(asinf, x)
#define k4asinh(x) K4REPL(asinhf, x)
#define k4atan(x) K4REPL(atanf, x)
#define k4atan2(x, y) K4REPL2(atan2f, x, y)
#define k4atanh(x) K4REPL(atanhf, x)
#define k4cos(x) K4REPL(cosf, x)
#define k4cosh(x) K4REPL(coshf, x)
#define k4exp(x) K4REPL(expf, x)
#define k4fmod(x, y) K4REPL2(fmodf, x, y)
#define k4log(x) K4REPL(logf, x)
#define k4pow(x, a) K4REPL2S(powf, x, a)
#define k4pown(x, i) K4REPL2I(powf, x, i)
#define k4sin(x) K4REPL(sinf, x)
#define k4sinh(x) K4REPL(sinhf, x)
#define k4tan(x) K4REPL(tanf, x)
#define k4tanh(x) K4REPL(tanhf, x)

// #define k4ifmsb(x, y, z)
//   (vec_sel((z), (y), vec_sra(vec_convert((x), &(vector int *)0), 31)))

// canonical true is -1, canonical false is 0
// truth values are interpreted bit-wise
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN4_VEC k4lfalse1() {
  CCTK_BOOLEAN4_VEC dummy;
  return vec_xor(dummy, dummy);
}
#define k4lfalse (k4lfalse1())
#define k4ltrue (k4lnot(k4lfalse))

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN4_VEC
k4lnot(CCTK_BOOLEAN4_VEC x) {
  return vec_nor(x, x);
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN4_VEC
k4land(CCTK_BOOLEAN4_VEC x, CCTK_BOOLEAN4_VEC y) {
  return vec_and(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN4_VEC
k4lor(CCTK_BOOLEAN4_VEC x, CCTK_BOOLEAN4_VEC y) {
  return vec_or(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN4_VEC
k4lxor(CCTK_BOOLEAN4_VEC x, CCTK_BOOLEAN4_VEC y) {
  return vec_xor(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL4_VEC
k4ifthen(CCTK_BOOLEAN4_VEC x, CCTK_REAL4_VEC y, CCTK_REAL4_VEC z) {
  return vec_sel(z, y, x);
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN4_VEC
k4cmpeq(CCTK_REAL4_VEC x, CCTK_REAL4_VEC y) {
  return vec_cmpeq(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN4_VEC
k4cmpne(CCTK_REAL4_VEC x, CCTK_REAL4_VEC y) {
  return k4lnot(vec_cmpeq(x, y));
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN4_VEC
k4cmpgt(CCTK_REAL4_VEC x, CCTK_REAL4_VEC y) {
  return vec_cmpgt(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN4_VEC
k4cmpge(CCTK_REAL4_VEC x, CCTK_REAL4_VEC y) {
  return vec_cmpge(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN4_VEC
k4cmplt(CCTK_REAL4_VEC x, CCTK_REAL4_VEC y) {
  return vec_cmplt(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN4_VEC
k4cmple(CCTK_REAL4_VEC x, CCTK_REAL4_VEC y) {
  return vec_cmple(x, y);
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL4_VEC
k4sgn(CCTK_REAL4_VEC x) {
  CCTK_BOOLEAN4_VEC iszero = k4cmpeq(x, vec4_set1((CCTK_REAL4)0.0));
  CCTK_REAL4_VEC signedone = k4copysign(vec4_set1((CCTK_REAL4)1.0), x);
  return k4ifthen(iszero, vec4_set1((CCTK_REAL4)0.0), signedone);
}
