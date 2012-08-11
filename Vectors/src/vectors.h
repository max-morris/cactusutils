#ifndef VECTORS_H
#define VECTORS_H

#include <cctk.h>



#if VECTORISE

/* TODO: support AVX */
#  if defined(__SSE__)          // Intel SSE
#    include "vectors-4-SSE.h"
#  elif defined(__ALTIVEC__)    // Power Altivec
#    include "vectors-4-Altivec.h"
#  endif

#  if defined(__AVX__)          // Intel AVX
#    include "vectors-8-AVX.h"
#  elif defined(__SSE2__)       // Intel SSE2
#    if VECTORISE_EMULATE_AVX
#      include "vectors-8-AVX.h"
#    else
#      include "vectors-8-SSE2.h"
#    endif
#  elif defined(_ARCH_QP)       // Blue Gene/Q QPX
#    include "vectors-8-QPX.h"
#  elif defined(__ALTIVEC__) && defined(_ARCH_PWR7) // Power VSX
#    include "vectors-8-VSX.h"
#  elif defined(_ARCH_450D)     // Blue Gene/P Double Hummer
#    include "vectors-8-DoubleHummer.h"
#  endif

#endif

// Default implementation, do not vectorise
#if ! defined(CCTK_REAL4_VEC_SIZE)
#  include "vectors-4-default.h"
#endif
#if ! defined(CCTK_REAL8_VEC_SIZE)
#  include "vectors-8-default.h"
#endif



// Define macros for CCTK_REAL

#if defined(CCTK_REAL_PRECISION_4)

#  define vec_architecture vec4_architecture

#  define CCTK_REAL_VEC      CCTK_REAL4_VEC
#  define CCTK_REAL_VEC_SIZE CCTK_REAL4_VEC_SIZE

#  define vec_set1 vec4_set1
#  define vec_set  vec4_set

#  define vec_elt0 vec4_elt0
#  define vec_elt  vec4_elt

#  define vec_load                  vec4_load
#  define vec_loadu                 vec4_loadu
#  define vec_loadu_maybe           vec4_loadu_maybe
#  define vec_loadu_maybe3          vec4_loadu_maybe3
#  define vec_store                 vec4_store
#  define vec_store_nta             vec4_store_nta
#  define vec_store_partial_prepare vec4_store_partial_prepare
#  define vec_store_nta_partial     vec4_store_nta_partial
#  define vec_store_nta_partial_lo  vec4_store_nta_partial_lo
#  define vec_store_nta_partial_hi  vec4_store_nta_partial_hi
#  define vec_store_nta_partial_mid vec4_store_nta_partial_mid

#  define kneg k4neg

#  define kadd k4add
#  define ksub k4sub
#  define kmul k4mul
#  define kdiv k4div

#  define kmadd  k4madd
#  define kmsub  k4msub
#  define knmadd k4nmadd
#  define knmsub k4nmsub

#  define kacos     k4acos
#  define kacosh    k4acosh
#  define kasin     k4asin
#  define kasinh    k4asinh
#  define katan     k4atan
#  define katan2    k4atan2
#  define katanh    k4atanh
#  define kcopysign k4copysign
#  define kcos      k4cos
#  define kcosh     k4cosh
#  define kexp      k4exp
#  define kfabs     k4fabs
#  define kfmax     k4fmax
#  define kfmin     k4fmin
#  define kfnabs    k4fnabs
#  define klog      k4log
#  define kpow      k4pow
#  define ksin      k4sin
#  define ksinh     k4sinh
#  define ksgn      k4sgn
#  define ksqrt     k4sqrt
#  define ktan      k4tan
#  define ktanh     k4tanh

#  define kifthen k4ifthen

#elif defined(CCTK_REAL_PRECISION_8)

#  define vec_architecture vec8_architecture

#  define CCTK_REAL_VEC      CCTK_REAL8_VEC
#  define CCTK_REAL_VEC_SIZE CCTK_REAL8_VEC_SIZE

#  define vec_set1 vec8_set1
#  define vec_set  vec8_set

#  define vec_elt0 vec8_elt0
#  define vec_elt  vec8_elt

#  define vec_load                  vec8_load
#  define vec_loadu                 vec8_loadu
#  define vec_loadu_maybe           vec8_loadu_maybe
#  define vec_loadu_maybe3          vec8_loadu_maybe3
#  define vec_store                 vec8_store
#  define vec_store_partial_prepare vec8_store_partial_prepare
#  define vec_store_nta             vec8_store_nta
#  define vec_store_nta_partial     vec8_store_nta_partial
#  define vec_store_nta_partial_lo  vec8_store_nta_partial_lo
#  define vec_store_nta_partial_hi  vec8_store_nta_partial_hi
#  define vec_store_nta_partial_mid vec8_store_nta_partial_mid

#  define kneg k8neg

#  define kadd k8add
#  define ksub k8sub
#  define kmul k8mul
#  define kdiv k8div

#  define kmadd  k8madd
#  define kmsub  k8msub
#  define knmadd k8nmadd
#  define knmsub k8nmsub

#  define kacos     k8acos
#  define kacosh    k8acosh
#  define kasin     k8asin
#  define kasinh    k8asinh
#  define katan     k8atan
#  define katan2    k8atan2
#  define katanh    k8atanh
#  define kcopysign k8copysign
#  define kcos      k8cos
#  define kcosh     k8cosh
#  define kexp      k8exp
#  define kfabs     k8fabs
#  define kfmax     k8fmax
#  define kfmin     k8fmin
#  define kfnabs    k8fnabs
#  define klog      k8log
#  define kpow      k8pow
#  define ksin      k8sin
#  define ksinh     k8sinh
#  define ksgn      k8sgn
#  define ksqrt     k8sqrt
#  define ktan      k8tan
#  define ktanh     k8tanh

#  define kifthen k8ifthen

#else

#  error "Unknown CCTK_REAL_PRECISION"

#endif



#define kifmsb(a,b,c) kifthen(a,b,c)
#define kifneg(a,b,c) kifmsb(a,b,c)
#define kifpos(a,b,c) kifmsb(a,c,b)



#if CCTK_REAL_VEC_SIZE == 1
#  define vec_index vec_set(0)
#elif CCTK_REAL_VEC_SIZE == 2
#  define vec_index vec_set(0,1)
#elif CCTK_REAL_VEC_SIZE == 4
#  define vec_index vec_set(0,1,2,3)
#elif CCTK_REAL_VEC_SIZE == 8
#  define vec_index vec_set(0,1,2,3,4,5,6,7)
#elif CCTK_REAL_VEC_SIZE == 16
#  define vec_index vec_set(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15)
#else
#  error "Unsupported vector size"
#endif

  

// Define a class template for easier access from C++

#ifdef __cplusplus

template<typename T>
struct vecprops {
  typedef T scalar_t;
  typedef T vector_t;
  static inline int size()
  {
    return 1;
  }
  static inline vector_t load (scalar_t const& a)
  {
    return a;
  }
  static inline vector_t loadu (scalar_t const& a)
  {
    return a;
  }
  static inline scalar_t elt (vector_t const& x, int const d)
  {
    return x;
  }
  static inline vector_t neg (vector_t const& x)
  {
    return -x;
  }
  static inline vector_t add (vector_t const& x, vector_t const& y)
  {
    return x+y;
  }
  static inline vector_t sub (vector_t const& x, vector_t const& y)
  {
    return x-y;
  }
  static inline vector_t mul (vector_t const& x, vector_t const& y)
  {
    return x*y;
  }
  static inline vector_t div (vector_t const& x, vector_t const& y)
  {
    return x/y;
  }
};

template<>
struct vecprops<CCTK_REAL4> {
  typedef CCTK_REAL4     scalar_t;
  typedef CCTK_REAL4_VEC vector_t;
  static inline int size()
  {
    return CCTK_REAL4_VEC_SIZE;
  }
  static inline vector_t load (scalar_t const& a)
  {
    return vec4_load(a);
  }
  static inline vector_t loadu (scalar_t const& a)
  {
    return vec4_loadu(a);
  }
  static inline scalar_t elt (vector_t const& x, int const d)
  {
    return vec4_elt(x,d);
  }
  static inline vector_t neg (vector_t const& x)
  {
    return k4neg(x);
  }
  static inline vector_t add (vector_t const& x, vector_t const& y)
  {
    return k4add(x,y);
  }
  static inline vector_t sub (vector_t const& x, vector_t const& y)
  {
    return k4sub(x,y);
  }
  static inline vector_t mul (vector_t const& x, vector_t const& y)
  {
    return k4mul(x,y);
  }
  static inline vector_t div (vector_t const& x, vector_t const& y)
  {
    return k4div(x,y);
  }
};

template<>
struct vecprops<CCTK_REAL8> {
  typedef CCTK_REAL8     scalar_t;
  typedef CCTK_REAL8_VEC vector_t;
  static inline int size()
  {
    return CCTK_REAL8_VEC_SIZE;
  }
  static inline vector_t load (scalar_t const& a)
  {
    return vec8_load(a);
  }
  static inline vector_t loadu (scalar_t const& a)
  {
    return vec8_loadu(a);
  }
  static inline scalar_t elt (vector_t const& x, int const d)
  {
    return vec8_elt(x,d);
  }
  static inline vector_t neg (vector_t const& x)
  {
    return k8neg(x);
  }
  static inline vector_t add (vector_t const& x, vector_t const& y)
  {
    return k8add(x,y);
  }
  static inline vector_t sub (vector_t const& x, vector_t const& y)
  {
    return k8sub(x,y);
  }
  static inline vector_t mul (vector_t const& x, vector_t const& y)
  {
    return k8mul(x,y);
  }
  static inline vector_t div (vector_t const& x, vector_t const& y)
  {
    return k8div(x,y);
  }
};

#endif



// For Kranc

#ifdef KRANC_C

#  undef KRANC_DIFF_FUNCTIONS
#  if ! VECTORISE_INLINE
#    define KRANC_DIFF_FUNCTIONS
#  endif

#  undef ToReal
#  define ToReal(x) (vec_set1(CCTK_REAL(x)))

#  undef KRANC_GFOFFSET3D
#  define KRANC_GFOFFSET3D(var,i,j,k)                                   \
  vec_loadu_maybe3((i),(j),(k),                                         \
                   *(CCTK_REAL const*)&                                 \
                   ((char const*)(var))[cdi*(i)+cdj*(j)+cdk*(k)])

#endif  // KRANC_C

#endif  // #ifndef VECTORS_H
