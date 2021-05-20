// -*-C++-*-
// Vectorise using IBM's Altivec VSX (Power)

// Use the type vector double directly, without introducing a wrapper class

// See <http://pic.dhe.ibm.com/infocenter/comphelp/v111v131/index.jsp>

#include <altivec.h>
#include <cmath>

#define vec8_architecture "VSX"

// Vector type corresponding to CCTK_REAL
typedef vector double CCTK_REAL8_VEC;
typedef vector signed long long CCTK_INTEGER8_VEC;
typedef vector bool long long CCTK_BOOLEAN8_VEC;

// Number of vector elements in a CCTK_REAL_VEC
#define CCTK_REAL8_VEC_SIZE 2

vec_static_assert(sizeof(CCTK_REAL8_VEC) ==
                  sizeof(CCTK_REAL8) * CCTK_REAL8_VEC_SIZE);

// Integer and boolean types corresponding to this real type
typedef long long CCTK_INTEGER8;
typedef unsigned long long CCTK_BOOLEAN8;

// Create vectors, extract vector elements

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
vec8_set1(CCTK_REAL8 a) {
  return vec_splats(a);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_INTEGER8_VEC
vec8_set1i(CCTK_INT8 a) {
  return vec_splats(a);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
vec8_set(CCTK_REAL8 a0, CCTK_REAL8 a1) {
  CCTK_REAL8_VEC x;
  x[0] = a0;
  x[1] = a1;
  return x;
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8
vec8_elt0(CCTK_REAL8_VEC x) {
  return x[0];
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8
vec8_elt1(CCTK_REAL8_VEC x) {
  return x[1];
}
// #define vec8_elt(x,d)  ((x)[d])
// #define vec8_elti(x,d) ((x)[d])
// #define vec8_eltb(x,d) ((x)[d])
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8 vec8_elt(CCTK_REAL8_VEC x,
                                                               int d) {
  return x[d];
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_INTEGER8
vec8_elti(CCTK_INTEGER8_VEC x, int d) {
  return x[d];
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8
vec8_eltb(CCTK_BOOLEAN8_VEC x, int d) {
  return x[d];
}

// Load and store vectors

// Load a vector from memory (aligned and unaligned); this loads from
// a reference to a scalar
//
// vec_ld (is alwasy the AltiVec lvx which silently aligns the pointer)
// vec_vsx_ld allows unaligned access (lxvx on POWER9)
//
// openpowerfoundation.org/wp-content/uploads/resources/Vector-Intrinsics/Vector-Intrinsics-20180306.pdf
// 1.1.3.2. page 10
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
vec8_load(CCTK_REAL8 const &p) {
  return vec_ld(0, (CCTK_REAL8_VEC*)&p);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
vec8_loadu(CCTK_REAL8 const &p) {
  return vec_vsx_ld(0, &p);
}

// Load a vector from memory that may or may not be aligned, as
// decided by the offset and the vector size
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
vec8_loadu_maybe(std::ptrdiff_t off, CCTK_REAL8 const &p) {
  return vec8_loadu(p);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
vec8_loadu_maybe3(std::ptrdiff_t off1, std::ptrdiff_t off2, std::ptrdiff_t off3,
                  CCTK_REAL8 const &p) {
  return vec8_loadu(p);
}

// Store a vector to memory (aligned and non-temporal); this stores to
// a reference to a scalar
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void vec8_store(CCTK_REAL8 &p,
                                                           CCTK_REAL8_VEC x) {
  vec_st(x, 0 , (CCTK_REAL8_VEC*)&p);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void vec8_storeu(CCTK_REAL8 &p,
                                                            CCTK_REAL8_VEC x) {
  vec_vsx_st(x, 0 , &p);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
vec8_store_nta(CCTK_REAL8 &p, CCTK_REAL8_VEC x) {
  // stvxl instruction doesn't exist for double precision
  vec8_store(p, x);
}

// Store a partial vector (aligned and non-temporal)
#define vec8_store_partial_prepare_fixed(i, imin, imax)                        \
  vec8_store_partial_prepare(i, imin, imax)

#define vec8_store_partial_prepare(i, imin, imax)                              \
  bool const v8stp_lo = (i) >= (imin);                                         \
  bool const v8stp_hi = (i) + CCTK_REAL8_VEC_SIZE - 1 < (imax)
#define vec8_store_nta_partial(p_, x_)                                         \
  ({                                                                           \
    CCTK_REAL8 &p__ = (p_);                                                    \
    CCTK_REAL8 &p = p__;                                                       \
    CCTK_REAL8_VEC const x__ = (x_);                                           \
    CCTK_REAL8_VEC const x = x__;                                              \
    /* if (CCTK_BUILTIN_EXPECT(v8stp_lo and v8stp_hi, true)) { */              \
    if (v8stp_lo and v8stp_hi) {                                               \
      vec8_store(p, x);                                                        \
    } else if (v8stp_lo) {                                                     \
      (&p)[0] = vec8_elt0(x);                                                  \
    } else if (v8stp_hi) {                                                     \
      (&p)[1] = vec8_elt1(x);                                                  \
    }                                                                          \
  })

#define vec8_storeu_partial(p, x) vec8_storeu_partial_(v8stp_lo, v8stp_hi, p, x)
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
vec8_storeu_partial_(bool const lo, bool const hi, CCTK_REAL8 &p,
                     CCTK_REAL8_VEC const x) {
  // if (CCTK_BUILTIN_EXPECT(lo and hi, true)) {
  if (lo and hi) {
    vec8_storeu(p, x);
  } else if (lo) {
    (&p)[0] = vec8_elt(x, 0);
  } else if (hi) {
    (&p)[1] = vec8_elt(x, 1);
  }
}

// Store a lower or higher partial vector (aligned and non-temporal);
// the non-temporal hint is probably ignored
#define vec8_store_nta_partial_lo(p, x, n) ((&(p))[0] = (x)[0])
#define vec8_store_nta_partial_hi(p, x, n) ((&(p))[1] = (x)[1])
#define vec8_store_nta_partial_mid(p, x, nlo, nhi) (assert(0))

// Functions and operators

// Operators
#define k8neg(x) (-(x))

#define k8add(x, y) ((x) + (y))
#define k8sub(x, y) ((x) - (y))
#define k8mul(x, y) ((x) * (y))
#define k8div(x, y) ((x) / (y))

// Fused multiply-add, defined as [+-]x*y[+-]z
#define k8madd(x, y, z) (vec_madd(x, y, z))
#define k8msub(x, y, z) (vec_msub(x, y, z))
#define k8nmadd(x, y, z) (vec_nmadd(x, y, z))
#define k8nmsub(x, y, z) (vec_nmsub(x, y, z))

// Cheap functions
// IBM
// https://www.ibm.com/support/knowledgecenter/en/SSGH2K_13.1.2/com.ibm.xlc1312.aix.doc/compiler_ref/vec_cpsgn.html
// says x's sign is copied but experiment shows that gcc copys y's sign
#define k8copysign(x, y) (vec_cpsgn(x, y))
#define k8fabs(x) (vec_abs(x))
#define k8fmax(x, y) (vec_max(x, y))
#define k8fmin(x, y) (vec_min(x, y))
#define k8fnabs(x) (vec_nabs(x))
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8signbit(CCTK_REAL8_VEC const x) {
  return vec_cmplt((CCTK_INTEGER8_VEC)x, vec8_set1i(0));
}
#define k8sqrt(x) (vec_sqrt(x))

// Reduction operations

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bool
k8all(CCTK_BOOLEAN8_VEC const x) {
  return x[0] & x[1];
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bool
k8any(CCTK_BOOLEAN8_VEC const x) {
  return x[0] | x[1];
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8
k8maximum(CCTK_REAL8_VEC const x) {
  return fmax(x[0], x[1]);
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8
k8minimum(CCTK_REAL8_VEC const x) {
  return fmin(x[0], x[1]);
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8
k8sum(CCTK_REAL8_VEC const x) {
  return x[0] + x[1];
}

// Expensive functions
#define K8REPL(f, x) vec8_set(f(vec8_elt0(x)), f(vec8_elt1(x)))
#define K8REPL2S(f, x, a) vec8_set(f(vec8_elt0(x), a), f(vec8_elt1(x), a))
#define K8REPL2I(f, x, i) vec8_set(f(vec8_elt0(x), i), f(vec8_elt1(x), i))
#define K8REPL2(f, x, y)                                                       \
  vec8_set(f(vec8_elt0(x), vec8_elt0(y)), f(vec8_elt1(x), vec8_elt1(y)))

#define k8acos(x) K8REPL(acos, x)
#define k8acosh(x) K8REPL(acosh, x)
#define k8asin(x) K8REPL(asin, x)
#define k8asinh(x) K8REPL(asinh, x)
#define k8atan(x) K8REPL(atan, x)
#define k8atan2(x, y) K8REPL2(atan2, x, y)
#define k8atanh(x) K8REPL(atanh, x)
#define k8cos(x) K8REPL(cos, x)
#define k8cosh(x) K8REPL(cosh, x)
#define k8exp(x) K8REPL(exp, x)
#define k8fmod(x, y) K8REPL2(fmod, x, y)
#define k8log(x) K8REPL(log, x)
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8pow(CCTK_REAL8_VEC x, CCTK_REAL8 a) {
  return K8REPL2S(std::pow, x, a);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8pown(CCTK_REAL8_VEC x, CCTK_INT8 i) {
  return K8REPL2I(std::pow, x, i);
}
#define k8sin(x) K8REPL(sin, x)
#define k8sinh(x) K8REPL(sinh, x)
#define k8tan(x) K8REPL(tan, x)
#define k8tanh(x) K8REPL(tanh, x)

// canonical true is -1LL, canonical false is 0LL
// truth values are interpreted bit-wise
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC k8lfalse1() {
  CCTK_BOOLEAN8_VEC dummy;
  return vec_xor(dummy, dummy);
}
#define k8lfalse (k8lfalse1())
#define k8ltrue (k8lnot(k8lfalse))

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8lnot(CCTK_BOOLEAN8_VEC x) {
  return vec_nor(x, x);
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8land(CCTK_BOOLEAN8_VEC x, CCTK_BOOLEAN8_VEC y) {
  return vec_and(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8lor(CCTK_BOOLEAN8_VEC x, CCTK_BOOLEAN8_VEC y) {
  return vec_or(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8lxor(CCTK_BOOLEAN8_VEC x, CCTK_BOOLEAN8_VEC y) {
  return vec_xor(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8ifthen(CCTK_BOOLEAN8_VEC x, CCTK_REAL8_VEC y, CCTK_REAL8_VEC z) {
  return vec_sel(z, y, x);
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8cmpeq(CCTK_REAL8_VEC x, CCTK_REAL8_VEC y) {
  return vec_cmpeq(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8cmpne(CCTK_REAL8_VEC x, CCTK_REAL8_VEC y) {
  return k8lnot(vec_cmpeq(x, y));
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8cmpgt(CCTK_REAL8_VEC x, CCTK_REAL8_VEC y) {
  return vec_cmpgt(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8cmpge(CCTK_REAL8_VEC x, CCTK_REAL8_VEC y) {
  return vec_cmpge(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8cmplt(CCTK_REAL8_VEC x, CCTK_REAL8_VEC y) {
  return vec_cmplt(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8cmple(CCTK_REAL8_VEC x, CCTK_REAL8_VEC y) {
  return vec_cmple(x, y);
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8sgn(CCTK_REAL8_VEC x) {
  CCTK_BOOLEAN8_VEC iszero = k8cmpeq(x, vec8_set1((CCTK_REAL8)0.0));
  CCTK_REAL8_VEC signedone = k8copysign(vec8_set1((CCTK_REAL8)1.0), x);
  return k8ifthen(iszero, vec8_set1((CCTK_REAL8)0.0), signedone);
}
