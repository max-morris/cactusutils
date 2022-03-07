// -*-C++-*-

#ifndef VECTORS_H
#define VECTORS_H

#include <cctk.h>

#define vec_static_assert(x)                                                   \
  namespace {                                                                  \
  typedef int vsa[(x) ? 1 : -1];                                               \
  }

#if VECTORISE

#if defined __AVX__ && !defined DISABLE_AVX // Intel AVX
#include "vectors-4-AVX.h"
#elif defined __SSE__ // Intel SSE
#include "vectors-4-SSE.h"
#elif defined __ALTIVEC__ // Power Altivec
#include "vectors-4-Altivec.h"
#endif

#if (defined __knl__ || defined __AVX512F__) &&                                \
    !defined DISABLE_AVX512 // Intel AVX512
#include "vectors-8-AVX512.h"
#elif (defined __MIC__ || defined __knl__) &&                                  \
    !defined DISABLE_AVX512 // Intel MIC
#include "vectors-8-MIC.h"
#elif defined __AVX__ && !defined DISABLE_AVX // Intel AVX
#include "vectors-8-AVX.h"
#elif defined __SSE2__ // Intel SSE2
#include "vectors-8-SSE2.h"
#elif defined __ALTIVEC__ && defined _ARCH_PWR7 // Power VSX
#include "vectors-8-VSX.h"
#endif

#endif

// Default implementation, do not vectorise
#ifndef CCTK_REAL4_VEC_SIZE
#include "vectors-4-default.h"
#endif
#ifndef CCTK_REAL8_VEC_SIZE
#include "vectors-8-default.h"
#endif

// Operation counters
#ifndef VEC_COUNT
#define VEC_COUNT(x) 0
#endif
// This expects variables declared as
//    ptrdiff_t vec_op_counter, vec_mem_counter;
#define vec_op_inc ((void)(VEC_COUNT(vec_op_counter += CCTK_REAL_VEC_SIZE)))
#define vec_mem_inc ((void)(VEC_COUNT(vec_mem_counter += CCTK_REAL_VEC_SIZE)))

// Define macros for CCTK_REAL

#if defined CCTK_REAL_PRECISION_4

#define vec_architecture vec4_architecture

#define CCTK_REAL_VEC CCTK_REAL4_VEC
#define CCTK_REAL_VEC_SIZE CCTK_REAL4_VEC_SIZE
#define CCTK_INTEGER CCTK_INTEGER4
#define CCTK_BOOLEAN CCTK_BOOLEAN4
#define CCTK_INTEGER_VEC CCTK_INTEGER4_VEC
#define CCTK_BOOLEAN_VEC CCTK_BOOLEAN4_VEC

#define vec_set1 vec4_set1
#define vec_set vec4_set

#define vec_elt vec4_elt
#define vec_elti vec4_elti
#define vec_eltb vec4_eltb

#define vec_load(p) (vec_mem_inc, vec4_load(p))
#define vec_loadu(p) (vec_mem_inc, vec4_loadu(p))
#define vec_loadu_maybe(off, p) (vec_mem_inc, vec4_loadu_maybe(off, p))
#define vec_loadu_maybe3(off1, off2, off3, p)                                  \
  (vec_mem_inc, vec4_loadu_maybe3(off1, off2, off3, p))
#define vec_store(p, x) (vec_mem_inc, vec4_store(p, x))
#define vec_store_nta(p, x) (vec_mem_inc, vec4_store_nta(p, x))
#define vec_store_partial_prepare vec4_store_partial_prepare
#define vec_store_nta_partial(p, x) (vec_mem_inc, vec4_store_nta_partial(p, x))
#define vec_storeu_partial(p, x) (vec_mem_inc, vec4_storeu_partial(p, x))
#define vec_store_nta_partial_lo vec4_store_nta_partial_lo
#define vec_store_nta_partial_hi vec4_store_nta_partial_hi
#define vec_store_nta_partial_mid vec4_store_nta_partial_mid

#define kneg(x) (vec_op_inc, k4neg(x))

#define kadd(x, y) (vec_op_inc, k4add(x, y))
#define ksub(x, y) (vec_op_inc, k4sub(x, y))
#define kmul(x, y) (vec_op_inc, k4mul(x, y))
#define kdiv(x, y) (vec_op_inc, k4div(x, y))

#define kmadd(x, y, z) (vec_op_inc, vec_op_inc, k4madd(x, y, z))
#define kmsub(x, y, z) (vec_op_inc, vec_op_inc, k4msub(x, y, z))
#define knmadd(x, y, z) (vec_op_inc, vec_op_inc, k4nmadd(x, y, z))
#define knmsub(x, y, z) (vec_op_inc, vec_op_inc, k4nmsub(x, y, z))

#define kacos k4acos
#define kacosh k4acosh
#define kasin k4asin
#define kasinh k4asinh
#define katan k4atan
#define katan2 k4atan2
#define katanh k4atanh
#define kcopysign(x, y) (vec_op_inc, k4copysign(x, y))
#define kcos k4cos
#define kcosh k4cosh
#define kexp k4exp
#define kfabs(x) (vec_op_inc, k4fabs(x))
#define kfmax(x, y) (vec_op_inc, k4fmax(x, y))
#define kfmin(x, y) (vec_op_inc, k4fmin(x, y))
#define kfmod(x, y) (vec_op_inc, k4fmod(x, y))
#define kfnabs(x) (vec_op_inc, k4fnabs(x))
#define klog k4log
#define kpow k4pow
#define kpown k4pown
#define ksignbit(x) (vec_op_inc, k4signbit(x))
#define ksin k4sin
#define ksinh k4sinh
#define ksgn k4sgn
#define ksqrt k4sqrt
#define ktan k4tan
#define ktanh k4tanh

#define klfalse k4lfalse
#define kltrue k4ltrue
#define klnot k4lnot
#define kland k4land
#define klor k4lor
#define klxor k4lxor
#define kifthen k4ifthen

#define kcmpeq k4cmpeq
#define kcmpne k4cmpne
#define kcmpgt k4cmpgt
#define kcmpge k4cmpge
#define kcmplt k4cmplt
#define kcmple k4cmple

#define kall k4all
#define kany k4any
#define kmaximum k4maximum
#define kminimum k4minimum
#define ksum k4sum

#elif defined CCTK_REAL_PRECISION_8

#define vec_architecture vec8_architecture

#define CCTK_REAL_VEC CCTK_REAL8_VEC
#define CCTK_REAL_VEC_SIZE CCTK_REAL8_VEC_SIZE
#define CCTK_INTEGER CCTK_INTEGER8
#define CCTK_BOOLEAN CCTK_BOOLEAN8
#define CCTK_INTEGER_VEC CCTK_INTEGER8_VEC
#define CCTK_BOOLEAN_VEC CCTK_BOOLEAN8_VEC

#define vec_set1 vec8_set1
#define vec_set vec8_set

#define vec_elt vec8_elt
#define vec_elti vec8_elti
#define vec_eltb vec8_eltb

#define vec_load(p) (vec_mem_inc, vec8_load(p))
#define vec_loadu(p) (vec_mem_inc, vec8_loadu(p))
#define vec_loadu_maybe(off, p) (vec_mem_inc, vec8_loadu_maybe(off, p))
#define vec_loadu_maybe3(off1, off2, off3, p)                                  \
  (vec_mem_inc, vec8_loadu_maybe3(off1, off2, off3, p))
#define vec_store(p, x) (vec_mem_inc, vec8_store(p, x))
#define vec_store_nta(p, x) (vec_mem_inc, vec8_store_nta(p, x))
#define vec_store_partial_prepare vec8_store_partial_prepare
#define vec_store_partial_prepare_fixed vec8_store_partial_prepare_fixed
#define vec_store_nta_partial(p, x) (vec_mem_inc, vec8_store_nta_partial(p, x))
#define vec_storeu_partial(p, x) (vec_mem_inc, vec8_storeu_partial(p, x))
#define vec_store_nta_partial_lo vec8_store_nta_partial_lo
#define vec_store_nta_partial_hi vec8_store_nta_partial_hi
#define vec_store_nta_partial_mid vec8_store_nta_partial_mid

#define kneg(x) (vec_op_inc, k8neg(x))

#define kadd(x, y) (vec_op_inc, k8add(x, y))
#define ksub(x, y) (vec_op_inc, k8sub(x, y))
#define kmul(x, y) (vec_op_inc, k8mul(x, y))
#define kdiv(x, y) (vec_op_inc, k8div(x, y))

#define kmadd(x, y, z) (vec_op_inc, vec_op_inc, k8madd(x, y, z))
#define kmsub(x, y, z) (vec_op_inc, vec_op_inc, k8msub(x, y, z))
#define knmadd(x, y, z) (vec_op_inc, vec_op_inc, k8nmadd(x, y, z))
#define knmsub(x, y, z) (vec_op_inc, vec_op_inc, k8nmsub(x, y, z))

#define kacos k8acos
#define kacosh k8acosh
#define kasin k8asin
#define kasinh k8asinh
#define katan k8atan
#define katan2 k8atan2
#define katanh k8atanh
#define kcopysign(x, y) (vec_op_inc, k8copysign(x, y))
#define kcos k8cos
#define kcosh k8cosh
#define kexp k8exp
#define kfabs(x) (vec_op_inc, k8fabs(x))
#define kfmax(x, y) (vec_op_inc, k8fmax(x, y))
#define kfmin(x, y) (vec_op_inc, k8fmin(x, y))
#define kfmod(x, y) (vec_op_inc, k8fmod(x, y))
#define kfnabs(x) (vec_op_inc, k8fnabs(x))
#define klog k8log
#define kpow k8pow
#define kpown k8pown
#define ksignbit(x) (vec_op_inc, k8signbit(x))
#define ksin k8sin
#define ksinh k8sinh
#define ksgn k8sgn
#define ksqrt k8sqrt
#define ktan k8tan
#define ktanh k8tanh

#define klfalse k8lfalse
#define kltrue k8ltrue
#define klnot k8lnot
#define kland k8land
#define klor k8lor
#define klxor k8lxor
#define kifthen k8ifthen

#define kcmpeq k8cmpeq
#define kcmpne k8cmpne
#define kcmpgt k8cmpgt
#define kcmpge k8cmpge
#define kcmplt k8cmplt
#define kcmple k8cmple

#define kall k8all
#define kany k8any
#define kmaximum k8maximum
#define kminimum k8minimum
#define ksum k8sum

#else

#error "Unknown CCTK_REAL_PRECISION"

#endif

// Deprecated
#define kifmsb(a, b, c) kifthen(a, b, c)
#define kifneg(a, b, c) kifmsb(a, b, c)
#define kifpos(a, b, c) kifmsb(a, c, b)

#define kisgn(a) (-42424242)

#if CCTK_REAL_VEC_SIZE == 1
#define vec_index vec_set(0)
#elif CCTK_REAL_VEC_SIZE == 2
#define vec_index vec_set(0, 1)
#elif CCTK_REAL_VEC_SIZE == 4
#define vec_index vec_set(0, 1, 2, 3)
#elif CCTK_REAL_VEC_SIZE == 8
#define vec_index vec_set(0, 1, 2, 3, 4, 5, 6, 7)
#elif CCTK_REAL_VEC_SIZE == 16
#define vec_index vec_set(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)
#else
#error "Unsupported vector size"
#endif

// Define a class template for easier access from C++

#ifdef __cplusplus

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>

template <typename T> struct vecprops {
  typedef T scalar_t;
  typedef T vector_t;
  typedef bool bscalar_t;
  typedef bool bvector_t;

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr std::size_t size() {
    return 1;
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t load(scalar_t const &a) {
    return vec_mem_inc, a;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t loadu(scalar_t const &a) {
    return vec_mem_inc, a;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void store(scalar_t &a,
                                                        vector_t const &x) {
    vec_mem_inc, a = x;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void storeu(scalar_t &a,
                                                         vector_t const &x) {
    vec_mem_inc, a = x;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
  store_partial(scalar_t &a, vector_t const &x, ptrdiff_t i, ptrdiff_t imin,
                ptrdiff_t imax) {
    vec_mem_inc, a = x;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
  storeu_partial(scalar_t &a, vector_t const &x, ptrdiff_t i, ptrdiff_t imin,
                 ptrdiff_t imax) {
    vec_mem_inc, a = x;
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vector_t
  set1(scalar_t const &a) {
    return a;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr scalar_t
  elt(vector_t const &x, std::ptrdiff_t const d) {
    return x;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvector_t
  bset1(bscalar_t const &a) {
    return a;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bscalar_t
  belt(bvector_t const &x, std::ptrdiff_t const d) {
    return x;
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vector_t
  neg(vector_t const &x) {
    return vec_op_inc, -x;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vector_t
  add(vector_t const &x, vector_t const &y) {
    return vec_op_inc, x + y;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vector_t
  sub(vector_t const &x, vector_t const &y) {
    return vec_op_inc, x - y;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vector_t
  mul(vector_t const &x, vector_t const &y) {
    return vec_op_inc, x * y;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vector_t
  div(vector_t const &x, vector_t const &y) {
    return vec_op_inc, x / y;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vector_t
  madd(vector_t const &x, vector_t const &y, vector_t const &z) {
    return vec_op_inc, vec_op_inc, x * y + z;
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvector_t
  lnot(bvector_t const &x) {
    return vec_op_inc, !x;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvector_t
  land(bvector_t const &x, bvector_t const &y) {
    return vec_op_inc, x && y;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvector_t
  lor(bvector_t const &x, bvector_t const &y) {
    return vec_op_inc, x || y;
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvector_t
  cmpeq(vector_t const &x, vector_t const &y) {
    return vec_op_inc, x == y;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvector_t
  cmpne(vector_t const &x, vector_t const &y) {
    return vec_op_inc, x != y;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvector_t
  cmpgt(vector_t const &x, vector_t const &y) {
    return vec_op_inc, x > y;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvector_t
  cmpge(vector_t const &x, vector_t const &y) {
    return vec_op_inc, x >= y;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvector_t
  cmplt(vector_t const &x, vector_t const &y) {
    return vec_op_inc, x < y;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvector_t
  cmple(vector_t const &x, vector_t const &y) {
    return vec_op_inc, x <= y;
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t
  good_copysign(vector_t const &x, vector_t const &y) {
    return vec_op_inc, k4copysign(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vector_t
  fabs(vector_t const &x) {
    return vec_op_inc, std::fabs(x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vector_t
  ifthen(bvector_t const &c, vector_t const &x, vector_t const &y) {
    return vec_op_inc, c ? x : y;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vector_t
  fmax(vector_t const &x, vector_t const &y) {
    return vec_op_inc, std::fmax(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vector_t
  fmin(vector_t const &x, vector_t const &y) {
    return vec_op_inc, std::fmin(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vector_t
  sqrt(vector_t const &x) {
    return vec_op_inc, std::sqrt(x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vector_t
  pow(vector_t const &x, int n) {
    return vec_op_inc, std::pow(x, n);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vector_t
  pow(vector_t const &x, scalar_t const &y) {
    return vec_op_inc, std::pow(x, y);
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bool
  all(bvector_t const &x) {
    return vec_op_inc, x;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bool
  any(bvector_t const &x) {
    return vec_op_inc, x;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr scalar_t
  maximum(vector_t const &x) {
    return vec_op_inc, x;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr scalar_t
  minimum(vector_t const &x) {
    return vec_op_inc, x;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr scalar_t
  sum(vector_t const &x) {
    return vec_op_inc, x;
  }
};

template <> struct vecprops<CCTK_REAL4> {
  typedef CCTK_REAL4 scalar_t;
  typedef CCTK_REAL4_VEC vector_t;
  typedef CCTK_BOOLEAN4 bscalar_t;
  typedef CCTK_BOOLEAN4_VEC bvector_t;

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr std::size_t size() {
    return CCTK_REAL4_VEC_SIZE;
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t load(scalar_t const &a) {
    return vec_mem_inc, vec4_load(a);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t loadu(scalar_t const &a) {
    return vec_mem_inc, vec4_loadu(a);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void store(scalar_t &a,
                                                        vector_t const &x) {
    vec_mem_inc, vec4_store(a, x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void storeu(scalar_t &a,
                                                         vector_t const &x) {
    vec_mem_inc, vec4_storeu(a, x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
  store_partial(scalar_t &a, vector_t const &x, ptrdiff_t i, ptrdiff_t imin,
                ptrdiff_t imax) {
    vec_mem_inc;
    vec4_store_partial_prepare(i, imin, imax);
    vec4_store_nta_partial(a, x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
  storeu_partial(scalar_t &a, vector_t const &x, ptrdiff_t i, ptrdiff_t imin,
                 ptrdiff_t imax) {
    vec_mem_inc;
    vec4_store_partial_prepare(i, imin, imax);
    vec4_storeu_partial(a, x);
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t set1(scalar_t const &a) {
    return vec4_set1(a);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE scalar_t elt(vector_t const &x,
                                                          int const d) {
    return vec4_elt(x, d);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t
  bset1(bscalar_t const &a) {
    return a ? k4ltrue : k4lfalse;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bscalar_t
  belt(bvector_t const &x, std::ptrdiff_t const d) {
    return vec4_eltb(x, d);
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t neg(vector_t const &x) {
    return vec_op_inc, k4neg(x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t add(vector_t const &x,
                                                          vector_t const &y) {
    return vec_op_inc, k4add(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t sub(vector_t const &x,
                                                          vector_t const &y) {
    return vec_op_inc, k4sub(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t mul(vector_t const &x,
                                                          vector_t const &y) {
    return vec_op_inc, k4mul(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t div(vector_t const &x,
                                                          vector_t const &y) {
    return vec_op_inc, k4div(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t madd(vector_t const &x,
                                                           vector_t const &y,
                                                           vector_t const &z) {
    return vec_op_inc, vec_op_inc, k4madd(x, y, z);
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t
  lnot(bvector_t const &x) {
    return vec_op_inc, k4lnot(x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t
  land(bvector_t const &x, bvector_t const &y) {
    return vec_op_inc, k4land(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t lor(bvector_t const &x,
                                                           bvector_t const &y) {
    return vec_op_inc, k4lor(x, y);
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t
  cmpeq(vector_t const &x, vector_t const &y) {
    return vec_op_inc, k4cmpeq(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t
  cmpne(vector_t const &x, vector_t const &y) {
    return vec_op_inc, k4cmpne(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t
  cmpgt(vector_t const &x, vector_t const &y) {
    return vec_op_inc, k4cmpgt(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t
  cmpge(vector_t const &x, vector_t const &y) {
    return vec_op_inc, k4cmpge(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t
  cmplt(vector_t const &x, vector_t const &y) {
    return vec_op_inc, k4cmplt(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t
  cmple(vector_t const &x, vector_t const &y) {
    return vec_op_inc, k4cmple(x, y);
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t
  good_copysign(vector_t const &x, vector_t const &y) {
    return vec_op_inc, k4copysign(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t fabs(vector_t const &x) {
    return vec_op_inc, k4fabs(x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t
  ifthen(bvector_t const &c, vector_t const &x, vector_t const &y) {
    return vec_op_inc, k4ifthen(c, x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t fmax(vector_t const &x,
                                                           vector_t const &y) {
    return vec_op_inc, k4fmax(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t fmin(vector_t const &x,
                                                           vector_t const &y) {
    return vec_op_inc, k4fmin(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t sqrt(vector_t const &x) {
    return vec_op_inc, k4sqrt(x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t pow(vector_t const &x,
                                                          scalar_t const &a) {
    return vec_op_inc, k4pow(x, a);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t pow(vector_t const &x,
                                                          int n) {
    return vec_op_inc, k4pown(x, n);
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bool all(bvector_t const &x) {
    return vec_op_inc, k4all(x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bool any(bvector_t const &x) {
    return vec_op_inc, k4any(x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE scalar_t
  maximum(vector_t const &x) {
    return vec_op_inc, k4maximum(x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE scalar_t
  minimum(vector_t const &x) {
    return vec_op_inc, k4minimum(x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE scalar_t sum(vector_t const &x) {
    return vec_op_inc, k4sum(x);
  }
};

template <> struct vecprops<CCTK_REAL8> {
  typedef CCTK_REAL8 scalar_t;
  typedef CCTK_REAL8_VEC vector_t;
  typedef CCTK_BOOLEAN8 bscalar_t;
  typedef CCTK_BOOLEAN8_VEC bvector_t;

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr std::size_t size() {
    return CCTK_REAL8_VEC_SIZE;
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t load(scalar_t const &a) {
    return vec_mem_inc, vec8_load(a);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t loadu(scalar_t const &a) {
    return vec_mem_inc, vec8_loadu(a);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void store(scalar_t &a,
                                                        vector_t const &x) {
    vec_mem_inc, vec8_store(a, x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void storeu(scalar_t &a,
                                                         vector_t const &x) {
    vec_mem_inc, vec8_storeu(a, x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
  store_partial(scalar_t &a, vector_t const &x, ptrdiff_t i, ptrdiff_t imin,
                ptrdiff_t imax) {
    vec_mem_inc;
    vec8_store_partial_prepare(i, imin, imax);
    vec8_store_nta_partial(a, x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
  storeu_partial(scalar_t &a, vector_t const &x, ptrdiff_t i, ptrdiff_t imin,
                 ptrdiff_t imax) {
    vec_mem_inc;
    vec8_store_partial_prepare(i, imin, imax);
    vec8_storeu_partial(a, x);
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t set1(scalar_t const &a) {
    return vec8_set1(a);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE scalar_t elt(vector_t const &x,
                                                          int const d) {
    return vec8_elt(x, d);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t
  bset1(bscalar_t const &a) {
    return a ? k8ltrue : k8lfalse;
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bscalar_t
  belt(bvector_t const &x, std::ptrdiff_t const d) {
    return vec8_eltb(x, d);
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t neg(vector_t const &x) {
    return vec_op_inc, k8neg(x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t add(vector_t const &x,
                                                          vector_t const &y) {
    return vec_op_inc, k8add(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t sub(vector_t const &x,
                                                          vector_t const &y) {
    return vec_op_inc, k8sub(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t mul(vector_t const &x,
                                                          vector_t const &y) {
    return vec_op_inc, k8mul(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t div(vector_t const &x,
                                                          vector_t const &y) {
    return vec_op_inc, k8div(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t madd(vector_t const &x,
                                                           vector_t const &y,
                                                           vector_t const &z) {
    return vec_op_inc, vec_op_inc, k8madd(x, y, z);
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t
  lnot(bvector_t const &x) {
    return vec_op_inc, k8lnot(x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t
  land(bvector_t const &x, bvector_t const &y) {
    return vec_op_inc, k8land(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t lor(bvector_t const &x,
                                                           bvector_t const &y) {
    return vec_op_inc, k8lor(x, y);
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t
  cmpeq(vector_t const &x, vector_t const &y) {
    return vec_op_inc, k8cmpeq(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t
  cmpne(vector_t const &x, vector_t const &y) {
    return vec_op_inc, k8cmpne(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t
  cmpgt(vector_t const &x, vector_t const &y) {
    return vec_op_inc, k8cmpgt(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t
  cmpge(vector_t const &x, vector_t const &y) {
    return vec_op_inc, k8cmpge(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t
  cmplt(vector_t const &x, vector_t const &y) {
    return vec_op_inc, k8cmplt(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvector_t
  cmple(vector_t const &x, vector_t const &y) {
    return vec_op_inc, k8cmple(x, y);
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t
  good_copysign(vector_t const &x, vector_t const &y) {
    return vec_op_inc, k8copysign(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t fabs(vector_t const &x) {
    return vec_op_inc, k8fabs(x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t
  ifthen(bvector_t const &c, vector_t const &x, vector_t const &y) {
    return vec_op_inc, k8ifthen(c, x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t fmax(vector_t const &x,
                                                           vector_t const &y) {
    return vec_op_inc, k8fmax(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t fmin(vector_t const &x,
                                                           vector_t const &y) {
    return vec_op_inc, k8fmin(x, y);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t sqrt(vector_t const &x) {
    return vec_op_inc, k8sqrt(x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t pow(vector_t const &x,
                                                          scalar_t const &a) {
    return vec_op_inc, k8pow(x, a);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vector_t pow(vector_t const &x,
                                                          int n) {
    return vec_op_inc, k8pown(x, n);
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bool all(bvector_t const &x) {
    return vec_op_inc, k8all(x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bool any(bvector_t const &x) {
    return vec_op_inc, k8any(x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE scalar_t
  maximum(vector_t const &x) {
    return vec_op_inc, k8maximum(x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE scalar_t
  minimum(vector_t const &x) {
    return vec_op_inc, k8minimum(x);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE scalar_t sum(vector_t const &x) {
    return vec_op_inc, k8sum(x);
  }
};

// TODO: Also define ivectype
template <typename T> class vectype;
template <typename T> class bvectype;

template <typename T> class vectype {
  typedef vecprops<T> props;

public:
  typedef typename props::scalar_t scalar_t;
  typedef typename props::vector_t vector_t;
  typedef typename props::bscalar_t bscalar_t;
  typedef typename props::bvector_t bvector_t;
  vector_t v;

  vectype() {}
  constexpr vectype(vectype const &x) : v(x.v) {}
  constexpr vectype(vector_t const &x) : v(x) {}
  // Hide the constructor, which is necessary in case scalar_t and
  // vector_t are the same type (e.g. if vectorization is disabled)
  template <typename = void>
  constexpr vectype(scalar_t const &a) : v(props::set1(a)) {}
  template <typename = void> constexpr operator vector_t() const { return v; }
  template <typename = void> constexpr vectype &operator=(vectype const &x) {
    v = x.v;
    return *this;
  }

  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr std::size_t size() const {
    return props::size();
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vectype load(scalar_t const &a) {
    return props::load(a);
  }
  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vectype loadu(scalar_t const &a) {
    return props::loadu(a);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE void store(scalar_t &a) const {
    props::store(a, v);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE void storeu(scalar_t &a) const {
    props::storeu(a, v);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE void store_partial(scalar_t &a,
                                                         ptrdiff_t i,
                                                         ptrdiff_t imin,
                                                         ptrdiff_t imax) const {
    props::store_partial(a, v, i, imin, imax);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
  storeu_partial(scalar_t &a, ptrdiff_t i, ptrdiff_t imin,
                 ptrdiff_t imax) const {
    props::storeu_partial(a, v, i, imin, imax);
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE vectype set1(scalar_t const &a) {
    return props::set1(a);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr scalar_t
  elt(std::ptrdiff_t const d) const {
    return props::elt(*this, d);
  }

  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype operator+() const {
    return *this;
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype operator-() const {
    return props::neg(*this);
  }

  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype
  operator+(vectype const &x) const {
    return props::add(*this, x);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype
  operator-(vectype const &x) const {
    return props::sub(*this, x);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype
  operator*(vectype const &x) const {
    return props::mul(*this, x);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype
  operator/(vectype const &x) const {
    return props::div(*this, x);
  }

  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype
  operator+(T const &x) const {
    return props::add(*this, vectype(x));
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype
  operator-(T const &x) const {
    return props::sub(*this, vectype(x));
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype
  operator*(T const &x) const {
    return props::mul(*this, vectype(x));
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype
  operator/(T const &x) const {
    return props::div(*this, vectype(x));
  }

  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype &
  operator+=(vectype const &x) {
    return *this = *this + x;
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype &
  operator-=(vectype const &x) {
    return *this = *this - x;
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype &
  operator*=(vectype const &x) {
    return *this = *this * x;
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype &
  operator/=(vectype const &x) {
    return *this = *this / x;
  }

  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype<T>
  operator==(vectype const &x) const {
    return props::cmpeq(*this, x);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype<T>
  operator!=(vectype const &x) const {
    return props::cmpne(*this, x);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype<T>
  operator>(vectype const &x) const {
    return props::cmpgt(*this, x);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype<T>
  operator>=(vectype const &x) const {
    return props::cmpge(*this, x);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype<T>
  operator<(vectype const &x) const {
    return props::cmplt(*this, x);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype<T>
  operator<=(vectype const &x) const {
    return props::cmple(*this, x);
  }

  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype<T>
  operator==(T const &x) const {
    return props::cmpeq(*this, vectype(x));
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype<T>
  operator!=(T const &x) const {
    return props::cmpne(*this, vectype(x));
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype<T>
  operator>(T const &x) const {
    return props::cmpgt(*this, vectype(x));
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype<T>
  operator>=(T const &x) const {
    return props::cmpge(*this, vectype(x));
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype<T>
  operator<(T const &x) const {
    return props::cmplt(*this, vectype(x));
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype<T>
  operator<=(T const &x) const {
    return props::cmple(*this, vectype(x));
  }

  friend std::ostream &operator<<(std::ostream &os, vectype const &x) {
    os << "[";
    for (std::size_t d = 0; d < x.size(); ++d) {
      if (d != 0)
        os << ",";
      os << x.elt(d);
    }
    os << "]";
    return os;
  }
};

template <typename T> class bvectype {
  typedef vecprops<T> props;

public:
  typedef typename props::scalar_t scalar_t;
  typedef typename props::vector_t vector_t;
  typedef typename props::bscalar_t bscalar_t;
  typedef typename props::bvector_t bvector_t;
  bvector_t bv;

  bvectype() {}
  constexpr bvectype(bvectype const &x) : bv(x.bv) {}
  constexpr bvectype(bvector_t const &x) : bv(x) {}
  // Hide the constructor, which is necessary in case scalar_t and
  // vector_t are the same type (e.g. if vectorization is disabled)
  template <typename = int>
  explicit constexpr bvectype(bscalar_t const &a) : bv(props::bset1(a)) {}
  constexpr operator bvector_t() const { return bv; }
  constexpr bvectype &operator=(bvectype const &x) {
    bv = x.bv;
    return *this;
  }

  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr std::size_t size() const {
    return props::size();
  }

  static inline CCTK_ATTRIBUTE_ALWAYS_INLINE bvectype set1(bscalar_t const &a) {
    return props::bset1(a);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bscalar_t
  elt(std::ptrdiff_t const d) const {
    return props::belt(*this, d);
  }

  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype operator~() const {
    return props::lnot(*this);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype operator!() const {
    return ~*this;
  }

  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype
  operator&(bvectype const &x) const {
    return props::land(*this, x);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype
  operator|(bvectype const &x) const {
    return props::lor(*this, x);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype
  operator^(bvectype const &x) const {
    return props::lxor(*this, x);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype
  operator&&(bvectype const &x) const {
    return *this & x;
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype
  operator||(bvectype const &x) const {
    return *this | x;
  }

  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype &
  operator&=(bvectype const &x) {
    return *this = *this & x;
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype &
  operator|=(bvectype const &x) {
    return *this = *this | x;
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype &
  operator^=(bvectype const &x) {
    return *this = *this ^ x;
  }

  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvector_t
  operator==(bvectype const &x) const {
    return !(*this ^ x);
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvector_t
  operator!=(bvectype const &x) const {
    return *this ^ x;
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvector_t
  operator>(bvectype const &x) const {
    return *this & ~x;
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvector_t
  operator>=(bvectype const &x) const {
    return *this | ~x;
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvector_t
  operator<(bvectype const &x) const {
    return ~*this & x;
  }
  inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvector_t
  operator<=(bvectype const &x) const {
    return ~*this | x;
  }

  friend std::ostream &operator<<(std::ostream &os, bvectype const &x) {
    os << "[";
    for (std::size_t d = 0; d < x.size(); ++d) {
      if (d != 0)
        os << ",";
      os << x.elt(d);
    }
    os << "]";
    return os;
  }
};

// ops between scalars and vectype
// these cannot be members of vectype since they take a scalar_t as their first
// argument
template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype<T>
operator+(T const &a, vectype<T> const &b) {
  return vectype<T>(a) + b;
}
template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype<T>
operator-(T const &a, vectype<T> const &b) {
  return vectype<T>(a) - b;
}
template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype<T>
operator*(T const &a, vectype<T> const &b) {
  return vectype<T>(a) * b;
}
template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype<T>
operator/(T const &a, vectype<T> const &b) {
  return vectype<T>(a) / b;
}

// Cactus defines "copysign" to "Cactus::copysign"
namespace std {
namespace Cactus {
template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE vectype<T>
good_copysign(vectype<T> const &x, vectype<T> const &y) {
  return vecprops<T>::good_copysign(x, y);
}
} // namespace Cactus
} // namespace std

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE T ifthen(bool const &c, T const &x,
                                             T const &y) {
  return c ? x : y;
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype<T>
ifthen(bvectype<T> const &c, vectype<T> const &x, vectype<T> const &y) {
  return vecprops<T>::ifthen(c, x, y);
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bvectype<T>
ifthen(bvectype<T> const &c, bvectype<T> const &x, bvectype<T> const &y) {
  return c & x | ~c & y;
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype<T>
fabs(vectype<T> const &x) {
  return vecprops<T>::fabs(x);
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype<T>
fmax(vectype<T> const &x, vectype<T> const &y) {
  return vecprops<T>::fmax(x, y);
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype<T>
fmin(vectype<T> const &x, vectype<T> const &y) {
  return vecprops<T>::fmin(x, y);
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype<T>
sqrt(vectype<T> const &x) {
  return vecprops<T>::sqrt(x);
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype<T>
pow(vectype<T> const &x, typename vectype<T>::scalar_t const &a) {
  return vecprops<T>::pow(x, a);
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr vectype<T>
pow(vectype<T> const &x, int n) {
  return vecprops<T>::pown(x, n);
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bool all(bvectype<T> const &x) {
  return vecprops<T>::all(x);
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr bool any(bvectype<T> const &x) {
  return vecprops<T>::any(x);
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr T maximum(vectype<T> const &x) {
  return vecprops<T>::maximum(x);
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr T minimum(vectype<T> const &x) {
  return vecprops<T>::minimum(x);
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE constexpr T sum(vectype<T> const &x) {
  return vecprops<T>::sum(x);
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE vectype<T> vload(const T *a,
                                                     ptrdiff_t ind) {
  return vectype<T>::loadu(a[ind]);
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE vectype<T> vloada(const T *a,
                                                      ptrdiff_t ind) {
  return vectype<T>::load(a[ind]);
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE void vstore(T *a, ptrdiff_t ind,
                                                vectype<T> x) {
  return x.storeu(a[ind]);
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE void vstorea(T *a, ptrdiff_t ind,
                                                 vectype<T> x) {
  return x.store(a[ind]);
}

struct vmask {
  // This mask is active (i.e. will load or store) for those element where both
  // `mpos >= 0` and `mneg < 0`.
  const ptrdiff_t mpos, mneg;
  vmask(const vmask &) = default;
  vmask(vmask &&) = default;
  vmask &operator=(const vmask &) = default;
  vmask &operator=(vmask &&) = default;
  vmask(ptrdiff_t mpos, ptrdiff_t mneg) : mpos(mpos), mneg(mneg){};
  // Call this as `vmask(i, imin, imax)` where `i` is the current loop index,
  // `imin` is the first loop index where the mask should be active, and `imax`
  // is one past the last loop index where the mask should be active.
  vmask(ptrdiff_t i, ptrdiff_t imin, ptrdiff_t imax)
      : mpos(i - imin), mneg(i - imax){};
  template <typename T> bvectype<T> boolmask() const;
};

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE vectype<T>
vload_masked(const T *a, ptrdiff_t ind, vectype<T> x0, vmask mask) {
  return ifthen(mask.boolmask<T>(), vload(a, ind), x0);
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE vectype<T>
vloada_masked(const T *a, ptrdiff_t ind, vectype<T> x0, vmask mask) {
  return ifthen(mask.boolmask<T>(), vloada(a, ind), x0);
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
vstore_masked(T *a, ptrdiff_t ind, vectype<T> x, vmask mask) {
  return x.storeu_partial(a[ind], 0, -mask.mpos, -mask.mneg);
}

template <typename T>
inline CCTK_ATTRIBUTE_ALWAYS_INLINE void
vstorea_masked(T *a, ptrdiff_t ind, vectype<T> x, vmask mask) {
  return x.store_partial(a[ind], 0, -mask.mpos, -mask.mneg);
}

template <typename T> bvectype<T> vmask::boolmask() const {
  struct arr {
    T elts[vecprops<T>::size()];
  } CCTK_ATTRIBUTE_ALIGNED(sizeof(vectype<T>));
  arr m;
  vstorea(m.elts, 0, vectype<T>(T(0.0)));
  vstorea_masked(m.elts, 0, vectype<T>(T(1.0)), *this);
  return vloada(m.elts, 0) != vectype<T>(T(0.0));
}
#endif

// Cache information

// Size of a a cache line in bytes
#ifndef CCTK_CACHELINE_SIZE
// TODO: Determine this properly
#define CCTK_CACHELINE_SIZE 64
#endif

// Number of CCTK_REALs in a cache line
#define CCTK_REAL_CACHELINE_SIZE (CCTK_CACHELINE_SIZE / CCTK_REAL_PRECISION)
// If this fails, something is most likely wrong -- this would be a
// very weird (and inefficient?) architecture indeed
#if CCTK_REAL_CACHELINE_SIZE % CCTK_REAL_VEC_SIZE != 0
#error "The cache line size is not a multiple of sizeof(CCTK_REAL_VEC)"
#endif

// For Kranc

#ifdef KRANC_C

#undef KRANC_DIFF_FUNCTIONS
#if !VECTORISE_INLINE
#define KRANC_DIFF_FUNCTIONS
#endif

#undef ToReal
#define ToReal(x) (vec_set1(CCTK_REAL(x)))

#undef IfThen
#if (defined __PGI ||                                    \
     (defined __ALTIVEC__ && defined _ARCH_PWR7))
static inline CCTK_ATTRIBUTE_ALWAYS_INLINE CCTK_REAL_VEC
vec_IfThen(CCTK_BOOLEAN x, CCTK_REAL_VEC y, CCTK_REAL_VEC z) {
  if (x)
    return y;
  else
    return z;
}
#define IfThen(x, y, z) vec_IfThen(x, y, z)
#else
#define IfThen(x, y, z) ((x) ? CCTK_REAL_VEC(y) : CCTK_REAL_VEC(z))
#endif

#undef KRANC_GFOFFSET3D
#define KRANC_GFOFFSET3D(var, i, j, k)                                         \
  vec_loadu_maybe3((i), (j), (k),                                              \
                   *(CCTK_REAL const *)&((                                     \
                       char const *)(var))[cdi * (i) + cdj * (j) + cdk * (k)])

#endif // KRANC_C

#endif // #ifndef VECTORS_H
