#ifndef VECTORS_H
#define VECTORS_H

#include <cctk.h>



#if defined(KRANC_VECTORS)

#  if defined(__SSE__)          // Intel SSE vector instructions
#    include "vectors-intel-4.h"
#  elif defined(__ALTIVEC__)    // Altivec (Power)
#    include "vectors-power-4.h"
#  endif

#  if defined(__SSE2__)         // Intel SSE2 vector instructions
#    include "vectors-intel-8.h"
#  elif defined(__ALTIVEC__) && defined(_ARCH_PWR7) // Altivec (Power)
#    include "vectors-power-8.h"
#  endif

#endif

// Default implementation, do not vectorise
#if ! defined(CCTK_REAL4_VEC_SIZE)
#  include "vectors-default-4.h"
#endif
#if ! defined(CCTK_REAL8_VEC_SIZE)
#  include "vectors-default-8.h"
#endif



// Define macros for CCTK_REAL

#if defined(CCTK_REAL_PRECISION_4)

#  define CCTK_REAL_VEC      CCTK_REAL4_VEC
#  define CCTK_REAL_VEC_SIZE CCTK_REAL4_VEC_SIZE

#  define vec_set1 vec4_set1
#  define vec_set  vec4_set

#  define vec_elt0 vec4_elt0
#  define vec_elt  vec4_elt

#  define vec_load                 vec4_load
#  define vec_loadu                vec4_loadu
#  define vec_loadu_maybe          vec4_loadu_maybe
#  define vec_loadu_maybe3         vec4_loadu_maybe3
#  define vec_store                vec4_store
#  define vec_store_nta            vec4_store_nta
#  define vec_store_nta_partial_lo vec4_store_nta_partial_lo
#  define vec_store_nta_partial_hi vec4_store_nta_partial_hi

#  define kpos k4pos
#  define kneg k4neg

#  define kadd k4add
#  define ksub k4sub
#  define kmul k4mul
#  define kdiv k4div

#  define kmadd  k4madd
#  define kmsub  k4msub
#  define knmadd k4nmadd
#  define knmsub k4nmsub

#  define kexp   k4exp
#  define kfabs  k4fabs
#  define kfmax  k4fmax
#  define kfmin  k4fmin
#  define kfnabs k4fnabs
#  define klog   k4log
#  define kpow   k4pow
#  define ksqrt  k4sqrt

#elif defined(CCTK_REAL_PRECISION_8)

#  define CCTK_REAL_VEC      CCTK_REAL8_VEC
#  define CCTK_REAL_VEC_SIZE CCTK_REAL8_VEC_SIZE

#  define vec_set1 vec8_set1
#  define vec_set  vec8_set

#  define vec_elt0 vec8_elt0
#  define vec_elt  vec8_elt

#  define vec_load                 vec8_load
#  define vec_loadu                vec8_loadu
#  define vec_loadu_maybe          vec8_loadu_maybe
#  define vec_loadu_maybe3         vec8_loadu_maybe3
#  define vec_store                vec8_store
#  define vec_store_nta            vec8_store_nta
#  define vec_store_nta_partial_lo vec8_store_nta_partial_lo
#  define vec_store_nta_partial_hi vec8_store_nta_partial_hi

#  define kpos k8pos
#  define kneg k8neg

#  define kadd k8add
#  define ksub k8sub
#  define kmul k8mul
#  define kdiv k8div

#  define kmadd  k8madd
#  define kmsub  k8msub
#  define knmadd k8nmadd
#  define knmsub k8nmsub

#  define kexp   k8exp
#  define kfabs  k8fabs
#  define kfmax  k8fmax
#  define kfmin  k8fmin
#  define kfnabs k8fnabs
#  define klog   k8log
#  define kpow   k8pow
#  define ksqrt  k8sqrt

#else

#  error "Unknown CCTK_REAL_PRECISION"

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
  static inline vector_t pos (vector_t const& x)
  {
    return +x;
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
  static inline vector_t pos (vector_t const& x)
  {
    return k4pos(x);
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
  static inline vector_t pos (vector_t const& x)
  {
    return k8pos(x);
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

#undef ToReal
#define ToReal(x) (vec_set1((CCTK_REAL)(x)))

#undef Sign
#define Sign(x) -999999999      // poison



#endif  // #ifndef VECTORS_H
