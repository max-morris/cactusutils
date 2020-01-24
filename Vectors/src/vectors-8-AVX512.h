// -*-C++-*-
// Vectorise using Intel's AVX512

// Use the type __m512d directly, without introducing a wrapper class

#include <cstdlib>
#include <cstring>

#include <immintrin.h>

#ifdef __AVX512ER__
#define vec8_architecture_AVX512ER "+AVX512ER"
#else
#define vec8_architecture_AVX512ER ""
#endif
#define vec8_architecture                                                      \
  "AVX512" vec8_architecture_AVX512ER " (64-bit precision)"

// Vector type corresponding to CCTK_REAL
typedef __m512d CCTK_REAL8_VEC;
typedef __m512i CCTK_INTEGER8_VEC;
typedef __mmask8 CCTK_BOOLEAN8_VEC;

// Number of vector elements in a CCTK_REAL_VEC
#define CCTK_REAL8_VEC_SIZE 8

vec_static_assert(sizeof(CCTK_REAL8_VEC) ==
                  sizeof(CCTK_REAL8) * CCTK_REAL8_VEC_SIZE);

// Integer and boolean types corresponding to this real type
typedef CCTK_INT8 CCTK_INTEGER8;
typedef bool CCTK_BOOLEAN8;

#define k8sign (vec8_set1i((CCTK_INTEGER8)(1ULL << 63ULL)))
#define k8notsign (vec8_set1i(~(CCTK_INTEGER8)(1ULL << 63ULL)))

// Create vectors, extract vector elements

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
vec8_set1(CCTK_REAL8 const a) {
  return _mm512_set1_pd(a);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_INTEGER8_VEC
vec8_set1i(CCTK_INT8 const a) {
  return _mm512_set1_epi64(a);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
vec8_set1b(CCTK_BOOLEAN8 const a) {
  return a ? 0xff : 0x00;
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
vec8_set(CCTK_REAL8 const a0, CCTK_REAL8 const a1, CCTK_REAL8 const a2,
         CCTK_REAL8 const a3, CCTK_REAL8 const a4, CCTK_REAL8 const a5,
         CCTK_REAL8 const a6, CCTK_REAL8 const a7) {
  return _mm512_set_pd(a7, a6, a5, a4, a3, a2, a1,
                       a0); // note reversed arguments
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8
vec8_elt(CCTK_REAL8_VEC const x, std::ptrdiff_t const d) {
  CCTK_REAL8 e;
  std::memcpy(&e, &((char const *)&x)[d * sizeof e], sizeof e);
  return e;
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8
vec8_eltb(CCTK_BOOLEAN8_VEC const x, std::ptrdiff_t const d) {
  return x & (1 << d);
}

// Load and store vectors

// Load a vector from memory (aligned and unaligned); this loads from
// a reference to a scalar
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
vec8_load(CCTK_REAL8 const &p) {
  return _mm512_load_pd(&p);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
vec8_loadu(CCTK_REAL8 const &p) {
  return _mm512_loadu_pd(&p);
}
#if VECTORISE_ALWAYS_USE_ALIGNED_LOADS
#error "VECTORISE_ALWAYS_USE_ALIGNED_LOADS is not yet supported"
#endif

// Load a vector from memory that may or may not be aligned, as
// decided by the offset off and the vector size
#if VECTORISE_ALWAYS_USE_UNALIGNED_LOADS
// Implementation: Always use unaligned load
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
vec8_loadu_maybe(std::ptrdiff_t const off, CCTK_REAL8 const &p) {
  return vec8_loadu(p);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
vec8_loadu_maybe3(std::ptrdiff_t const off1, std::ptrdiff_t const off2,
                  std::ptrdiff_t const off3, CCTK_REAL8 const &p) {
  return vec8_loadu(p);
}
#else
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
vec8_loadu_maybe(std::ptrdiff_t const off, CCTK_REAL8 const &p) {
  return off % CCTK_REAL8_VEC_SIZE == 0 ? vec8_load(p) : vec8_loadu(p);
}
#if VECTORISE_ALIGNED_ARRAYS
// Assume all array x sizes are multiples of the vector size
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
vec8_loadu_maybe3(std::ptrdiff_t const off1, std::ptrdiff_t const off2,
                  std::ptrdiff_t const off3, CCTK_REAL8 const &p) {
  return vec8_loadu_maybe(off1, p);
}
#else
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
vec8_loadu_maybe3(std::ptrdiff_t const off1, std::ptrdiff_t const off2,
                  std::ptrdiff_t const off3, CCTK_REAL8 const &p) {
  return off2 % CCTK_REAL8_VEC_SIZE != 0 or off3 % CCTK_REAL8_VEC_SIZE != 0
             ? vec8_loadu(p)
             : vec8_loadu_maybe(off1, p);
}
#endif
#endif

// Store a vector to memory (aligned and non-temporal); this stores to
// a reference to a scalar
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
vec8_store(CCTK_REAL8 &p, CCTK_REAL8_VEC const x) {
  _mm512_store_pd(&p, x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
vec8_storeu(CCTK_REAL8 &p, CCTK_REAL8_VEC const x) {
  _mm512_storeu_pd(&p, x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
vec8_store_nta(CCTK_REAL8 &p, CCTK_REAL8_VEC const x) {
#if 0 && VECTORISE_STREAMING_STORES
  // Possible implementations when using streaming stores include:
  // non-temporal hint:
  //   _mm512_extstore_pd(&p, x, _MM_DOWNCONV_PD_NONE, _MM_HINT_NT);
  // no-read hint:
  //   _mm512_storenr_pd(&p, x);
  //   _mm_clevict(&p, _MM_HINT_T1);
  // no-read hint, not globally ordered (requires fence?):
  //   _mm512_storenrngo_pd(&p, x);
  //   _mm_clevict(&p, _MM_HINT_T1);
  // However, these all seem slower, so we don't use streaming stores.
#else
  _mm512_store_pd(&p, x);
#endif
}

// Store a partial vector (aligned and non-temporal)
#define vec8_store_partial_prepare(i, imin, imax)                              \
  __mmask8 v8stp_mask;                                                         \
  vec8_store_partial_prepare_(v8stp_mask, i, imin, imax)
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
vec8_store_partial_prepare_(__mmask8 &mask, std::ptrdiff_t const i,
                            std::ptrdiff_t const imin,
                            std::ptrdiff_t const imax) {
  __mmask8 m = -1;
  if (i < imin) {
    /* clear lower imin-i bits */
    m &= 0xff << (imin - i);
  }
  if (i + CCTK_REAL8_VEC_SIZE > imax) {
    /* clear upper i+CCTK_REAL8_VEC_SIZE-imax bits */
    m &= 0xff >> (i + CCTK_REAL8_VEC_SIZE - imax);
  }
  mask = m;
}

#define vec8_store_partial_prepare_fixed vec8_store_partial_prepare

#define vec8_store_nta_partial(p, x) vec8_store_nta_partial_(v8stp_mask, p, x)
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
vec8_store_nta_partial_(__mmask8 const mask, CCTK_REAL8 &p,
                        CCTK_REAL8_VEC const x) {
  if (mask == 0xff)
    vec8_store_nta(p, x);
  else
    _mm512_mask_store_pd(&p, mask, x);
}

#define vec8_storeu_partial(p, x) vec8_storeu_partial_(v8stp_mask, p, x)
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
vec8_storeu_partial_(__mmask8 const mask, CCTK_REAL8 &p,
                     CCTK_REAL8_VEC const x) {
  if (mask == 0xff)
    vec8_storeu(p, x);
  else
    _mm512_mask_storeu_pd(&p, mask, x);
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
vec8_store_nta_partial_lo(CCTK_REAL8 &p, CCTK_REAL8_VEC const x,
                          ptrdiff_t const n) {
  _mm512_mask_store_pd(&p, 0xff >> (8 - n), x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
vec8_store_nta_partial_hi(CCTK_REAL8 &p, CCTK_REAL8_VEC const x,
                          ptrdiff_t const n) {
  _mm512_mask_store_pd(&p, 0xff << (8 - n), x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
vec8_store_nta_partial_mid(CCTK_REAL8 &p, CCTK_REAL8_VEC const x,
                           ptrdiff_t const nlo, ptrdiff_t const nhi) {
  _mm512_mask_store_pd(&p, (0xff >> (8 - nlo)) & (0xff << (8 - nhi)), x);
}

// Functions and operators

// Operators
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8neg(CCTK_REAL8_VEC const x) {
  // Could also multiply by -1
  // Could also invert sign bit
  return _mm512_sub_pd(_mm512_set1_pd(0.0), x);
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8add(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y) {
  return _mm512_add_pd(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8sub(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y) {
  return _mm512_sub_pd(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8mul(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y) {
  return _mm512_mul_pd(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8div(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y) {
#if 0
  // This is accurate, but slow; it is internally evaluated unvectorized
  return _mm512_div_pd(x, y);
#endif
#if 0
  // Starting from a reciprocal is facter, but leads to some round-off error
  // Approximate solution
  CCTK_REAL8_VEC const r0 = _mm512_mul_pd(x, _mm512_rcp28_pd(y));
  // One Newton iteration
  // Note: Don't rewrite this expression, this may introduce
  // cancellation errors
  // r += r * (x - y * r)
  CCTK_REAL8_VEC const r1 = _mm512_fmadd_pd(r0, _mm512_fnmadd_pd(y, r0, x), r0);
  return r1;
#endif
#if defined __AVX512ER__
  // Best algorithm: Start with an approximate result, then perform Newton
  // iteration until convergence. Theoretically the result should be exact.
  // Approximate solution
  CCTK_REAL8_VEC const r0 = _mm512_rcp28_pd(y);
  // One Newton iteration
  // Note: Don't rewrite this expression, this may introduce
  // cancellation errors
  // r += r * (1 - y * r)
  CCTK_REAL8_VEC const r1 =
      _mm512_fmadd_pd(r0, _mm512_fnmadd_pd(y, r0, vec8_set1(1.0)), r0);
  return _mm512_mul_pd(x, r1);
#else
  // Best algorithm: Start with an approximate result, then perform Newton
  // iteration until convergence. Theoretically the result should be exact.
  // Approximate solution
  CCTK_REAL8_VEC const r0 = _mm512_rcp14_pd(y);
  // Two Newton iterations
  CCTK_REAL8_VEC const r1 =
      _mm512_fmadd_pd(r0, _mm512_fnmadd_pd(y, r0, vec8_set1(1.0)), r0);
  CCTK_REAL8_VEC const r2 =
      _mm512_fmadd_pd(r1, _mm512_fnmadd_pd(y, r1, vec8_set1(1.0)), r1);
  return _mm512_mul_pd(x, r2);
#endif
}

// Fused multiply-add, defined as [+-]x*y[+-]z
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8madd(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y, CCTK_REAL8_VEC const z) {
  return _mm512_fmadd_pd(x, y, z);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8msub(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y, CCTK_REAL8_VEC const z) {
  return _mm512_fmsub_pd(x, y, z);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC k8nmadd(
    CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y, CCTK_REAL8_VEC const z) {
  return _mm512_fnmsub_pd(x, y, z);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC k8nmsub(
    CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y, CCTK_REAL8_VEC const z) {
  return _mm512_fnmadd_pd(x, y, z);
}

// Cheap functions
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8copysign(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y) {
  CCTK_INTEGER8_VEC ix = _mm512_castpd_si512(x);
  CCTK_INTEGER8_VEC iy = _mm512_castpd_si512(y);
  CCTK_INTEGER8_VEC ir = _mm512_or_epi64(_mm512_and_epi64(k8notsign, ix),
                                         _mm512_and_epi64(k8sign, iy));
  return _mm512_castsi512_pd(ir);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8fabs(CCTK_REAL8_VEC const x) {
  // Could also do k8fmax(x, k8neg(x))
  CCTK_INTEGER8_VEC ix = _mm512_castpd_si512(x);
  CCTK_INTEGER8_VEC ir = _mm512_and_epi64(k8notsign, ix);
  return _mm512_castsi512_pd(ir);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8fmax(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y) {
  return _mm512_max_pd(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8fmin(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y) {
  return _mm512_min_pd(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8fnabs(CCTK_REAL8_VEC const x) {
  // Could also do k8fmin(x, k8neg(x))
  CCTK_INTEGER8_VEC ix = _mm512_castpd_si512(x);
  CCTK_INTEGER8_VEC ir = _mm512_or_epi64(k8sign, ix);
  return _mm512_castsi512_pd(ir);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8signbit(CCTK_REAL8_VEC const x) {
  CCTK_INTEGER8_VEC ix = _mm512_castpd_si512(x);
  return _mm512_test_epi64_mask(k8sign, ix);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8sqrt(CCTK_REAL8_VEC const x); // implemented below

// Expensive functions

// These implementations are very expensive
#define K8REPL(f, x)                                                           \
  vec8_set(f(vec8_elt(x, 0)), f(vec8_elt(x, 1)), f(vec8_elt(x, 2)),            \
           f(vec8_elt(x, 3)), f(vec8_elt(x, 4)), f(vec8_elt(x, 5)),            \
           f(vec8_elt(x, 6)), f(vec8_elt(x, 7)));
#define K8REPL2S(f, x, a)                                                      \
  vec8_set(f(vec8_elt(x, 0), a), f(vec8_elt(x, 1), a), f(vec8_elt(x, 2), a),   \
           f(vec8_elt(x, 3), a), f(vec8_elt(x, 4), a), f(vec8_elt(x, 5), a),   \
           f(vec8_elt(x, 6), a), f(vec8_elt(x, 7), a));
#define K8REPL2(f, x, y)                                                       \
  vec8_set(                                                                    \
      f(vec8_elt(x, 0), vec8_elt(y, 0)), f(vec8_elt(x, 1), vec8_elt(y, 1)),    \
      f(vec8_elt(x, 2), vec8_elt(y, 2)), f(vec8_elt(x, 3), vec8_elt(y, 3)),    \
      f(vec8_elt(x, 4), vec8_elt(y, 4)), f(vec8_elt(x, 5), vec8_elt(y, 5)),    \
      f(vec8_elt(x, 6), vec8_elt(y, 6)), f(vec8_elt(x, 7), vec8_elt(y, 7)));

#if defined __ICC
// The Intel compiler provides intrinsics for these

// These implementations lead to an ICE with icpc 13.0.1
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8acos(CCTK_REAL8_VEC const x) {
  return _mm512_acos_pd(x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8acosh(CCTK_REAL8_VEC const x) {
  return _mm512_acosh_pd(x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8asin(CCTK_REAL8_VEC const x) {
  return _mm512_asin_pd(x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8asinh(CCTK_REAL8_VEC const x) {
  return _mm512_asinh_pd(x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8atan(CCTK_REAL8_VEC const x) {
  return _mm512_atan_pd(x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8atan2(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y) {
  return _mm512_atan2_pd(x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8atanh(CCTK_REAL8_VEC const x) {
  return _mm512_atanh_pd(x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8cos(CCTK_REAL8_VEC const x) {
  return _mm512_cos_pd(x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8cosh(CCTK_REAL8_VEC const x) {
  return _mm512_cosh_pd(x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8exp(CCTK_REAL8_VEC const x) {
  return _mm512_exp_pd(x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8fmod(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y) {
  return K8REPL2(fmod, x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8log(CCTK_REAL8_VEC const x) {
  return _mm512_log_pd(x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8pow(CCTK_REAL8_VEC const x, CCTK_REAL8 const a) {
  return _mm512_pow_pd(x, _mm512_set1_pd(a));
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8pown(CCTK_REAL8_VEC const x, int const i) {
  return _mm512_pow_pd(x, _mm512_set1_pd(CCTK_REAL8(i)));
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8sin(CCTK_REAL8_VEC const x) {
  return _mm512_sin_pd(x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8sinh(CCTK_REAL8_VEC const x) {
  return _mm512_sinh_pd(x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8tan(CCTK_REAL8_VEC const x) {
  return _mm512_tan_pd(x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8tanh(CCTK_REAL8_VEC const x) {
  return _mm512_tanh_pd(x);
}

#else

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8acos(CCTK_REAL8_VEC const x) {
  return K8REPL(acos, x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8acosh(CCTK_REAL8_VEC const x) {
  return K8REPL(acosh, x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8asin(CCTK_REAL8_VEC const x) {
  return K8REPL(asin, x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8asinh(CCTK_REAL8_VEC const x) {
  return K8REPL(asinh, x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8atan(CCTK_REAL8_VEC const x) {
  return K8REPL(atan, x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8atan2(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y) {
  return K8REPL2(atan2, x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8atanh(CCTK_REAL8_VEC const x) {
  return K8REPL(atanh, x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8cos(CCTK_REAL8_VEC const x) {
  return K8REPL(cos, x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8cosh(CCTK_REAL8_VEC const x) {
  return K8REPL(cosh, x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8exp(CCTK_REAL8_VEC const x) {
  return K8REPL(exp, x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8fmod(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y) {
  return K8REPL2(fmod, x, y);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8log(CCTK_REAL8_VEC const x) {
  return K8REPL(log, x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8pow(CCTK_REAL8_VEC const x, CCTK_REAL8 const a) {
  return K8REPL2S(pow, x, a);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8pown(CCTK_REAL8_VEC x, int i) {
  // return k8repl2si(pow, x, i);
  if (i < 0) {
    x = k8div(vec8_set1(1), x);
    i = -i;
  }
  CCTK_REAL8_VEC r = vec8_set1(1);
  while (i != 0) {
    if (i & 1)
      r = k8mul(x, r);
    x = k8mul(x, x);
    i >>= 1;
  }
  return r;
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8sin(CCTK_REAL8_VEC const x) {
  return K8REPL(sin, x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8sinh(CCTK_REAL8_VEC const x) {
  return K8REPL(sinh, x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8tan(CCTK_REAL8_VEC const x) {
  return K8REPL(tan, x);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8tanh(CCTK_REAL8_VEC const x) {
  return K8REPL(tanh, x);
}

#endif

// TODO: try k8lxor(x,x) and k8lxnor(x,x)
// #define k8lfalse (CCTK_BOOLEAN8_VEC(0))
// #define k8ltrue (CCTK_BOOLEAN8_VEC(-1))
#define k8lfalse (vec8_set1b(false))
#define k8ltrue (vec8_set1b(true))
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8lnot(CCTK_BOOLEAN8_VEC const x) {
  // return _mm512_knot(x);
  return ~x;
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8land(CCTK_BOOLEAN8_VEC const x, CCTK_BOOLEAN8_VEC const y) {
  // return _mm512_kand(x, y);
  return x & y;
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8lor(CCTK_BOOLEAN8_VEC const x, CCTK_BOOLEAN8_VEC const y) {
  // return _mm512_kor(x, y);
  return x | y;
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8lxor(CCTK_BOOLEAN8_VEC const x, CCTK_BOOLEAN8_VEC const y) {
  // return _mm512_kxor(x, y);
  return x ^ y;
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC k8ifthen(
    CCTK_BOOLEAN8_VEC const x, CCTK_REAL8_VEC const y, CCTK_REAL8_VEC const z) {
// This leads to an ICE
// return _mm512_mask_blend_pd(x, z, y);
#if 1
  // This works:
  return _mm512_mask_mov_pd(z, x, y);
#else
  // Intel suggests this:
  return x == 0 ? z : _mm512_mask_blend_pd(x, z, y);
#endif
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8cmpeq(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y) {
  return _mm512_cmp_pd_mask(x, y, _MM_CMPINT_EQ);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8cmpne(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y) {
  return _mm512_cmp_pd_mask(x, y, _MM_CMPINT_NE);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8cmpgt(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y) {
  return _mm512_cmp_pd_mask(x, y, _MM_CMPINT_GT);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8cmpge(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y) {
  return _mm512_cmp_pd_mask(x, y, _MM_CMPINT_GE);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8cmplt(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y) {
  return _mm512_cmp_pd_mask(x, y, _MM_CMPINT_LT);
}
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_BOOLEAN8_VEC
k8cmple(CCTK_REAL8_VEC const x, CCTK_REAL8_VEC const y) {
  return _mm512_cmp_pd_mask(x, y, _MM_CMPINT_LE);
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8sgn(CCTK_REAL8_VEC const x) {
  CCTK_BOOLEAN8_VEC const iszero = k8cmpeq(x, vec8_set1(0.0));
  CCTK_BOOLEAN8_VEC const isneg = k8cmplt(x, vec8_set1(0.0));
  return k8ifthen(iszero, vec8_set1(0.0),
                  k8ifthen(isneg, vec8_set1(-1.0), vec8_set1(+1.0)));
}

// Reduction operations

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bool
k8all(CCTK_BOOLEAN8_VEC const x) {
  // return mm512_kortestc(x, x);
  return x == 0xff;
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bool
k8any(CCTK_BOOLEAN8_VEC const x) {
  // return !bool(_mm512_kortestz(x, x));
  return x != 0x00;
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8
k8maximum(CCTK_REAL8_VEC const x1) {
  __m256d const x2 =
      _mm256_max_pd(_mm512_castpd512_pd256(x1), _mm512_extractf64x4_pd(x1, 1));
  __m128d const x4 =
      _mm_max_pd(_mm256_castpd256_pd128(x2), _mm256_extractf128_pd(x2, 1));
  return _mm_cvtsd_f64(_mm_max_sd(x4, _mm_shuffle_pd(x4, x4, 0b01)));
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8
k8minimum(CCTK_REAL8_VEC const x1) {
  __m256d const x2 =
      _mm256_min_pd(_mm512_castpd512_pd256(x1), _mm512_extractf64x4_pd(x1, 1));
  __m128d const x4 =
      _mm_min_pd(_mm256_castpd256_pd128(x2), _mm256_extractf128_pd(x2, 1));
  return _mm_cvtsd_f64(_mm_min_sd(x4, _mm_shuffle_pd(x4, x4, 0b01)));
}

static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8
k8sum(CCTK_REAL8_VEC const x1) {
  __m256d const x2 =
      _mm256_add_pd(_mm512_castpd512_pd256(x1), _mm512_extractf64x4_pd(x1, 1));
  __m128d const x4 =
      _mm_add_pd(_mm256_castpd256_pd128(x2), _mm256_extractf128_pd(x2, 1));
  return _mm_cvtsd_f64(_mm_hadd_pd(x4, x4));
}

// sqrt calls k8cmpeq and k8ifthen
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL8_VEC
k8sqrt(CCTK_REAL8_VEC const x) {
#if 0
  // This is accurate, but slow; it is internally evaluated unvectorized
  return _mm512_sqrt_pd(x);
#endif
#if defined __AVX512ER__
  // We start with an approximate result, then perform Goldschmidt iterations
  // <https://en.wikipedia.org/wiki/Methods_of_computing_square_roots>.
  // Theoretically the result should be exact.
  // Initialisation
  CCTK_REAL8_VEC const y0 = _mm512_rsqrt28_pd(x);
  CCTK_REAL8_VEC const x0 = k8mul(x, y0);
  CCTK_REAL8_VEC const h0 = k8mul(vec8_set1(0.5), y0);
  // Step
  CCTK_REAL8_VEC const r0 = k8nmsub(x0, h0, vec8_set1(0.5));
  CCTK_REAL8_VEC const x1 = k8madd(x0, r0, x0);
  return k8ifthen(k8cmpeq(x, vec8_set1(0.0)), x, x1);
#else
  // We start with an approximate result, then perform Goldschmidt iterations
  // <https://en.wikipedia.org/wiki/Methods_of_computing_square_roots>.
  // Theoretically the result should be exact.
  // Initialisation
  CCTK_REAL8_VEC const y0 = _mm512_rsqrt14_pd(x);
  CCTK_REAL8_VEC const x0 = k8mul(x, y0);
  CCTK_REAL8_VEC const h0 = k8mul(vec8_set1(0.5), y0);
  // Step
  CCTK_REAL8_VEC const r0 = k8nmsub(x0, h0, vec8_set1(0.5));
  CCTK_REAL8_VEC const x1 = k8madd(x0, r0, x0);
  CCTK_REAL8_VEC const h1 = k8madd(h0, r0, h0);
  // Step
  CCTK_REAL8_VEC const r1 = k8nmsub(x1, h1, vec8_set1(0.5));
  CCTK_REAL8_VEC const x2 = k8madd(x1, r1, x1);
  return k8ifthen(k8cmpeq(x, vec8_set1(0.0)), x, x2);
#endif
}
