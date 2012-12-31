// Vectorise using IBM's Blue Gene/Q QPX (Power)

// Use the type vector4double directly, without introducing a wrapper class
// Use macros instead of inline functions

// Note: bgxlC_r does not like const declarations, so we need to cast
// them away and/or omit them everywhere

// See <http://pic.dhe.ibm.com/infocenter/compbg/v121v141/index.jsp>



#include <assert.h>

#ifdef __cplusplus
#  include <builtins.h>
#endif



#define vec8_architecture "QPX"

// Vector type corresponding to CCTK_REAL
// We use a struct to avoid the "const" issue
// #define CCTK_REAL8_VEC vector4double
struct CCTK_REAL8_VEC {
  vector4double v;
  CCTK_REAL8_VEC() {}
  CCTK_REAL8_VEC(CCTK_REAL8_VEC const& x): v(x.v) {}
  CCTK_REAL8_VEC(vector4double v_): v(v_) {}
  operator vector4double() const { return v; }
};

// Number of vector elements in a CCTK_REAL_VEC
#define CCTK_REAL8_VEC_SIZE 4

// Integer and boolean types corresponding to this real type
//#define CCTK_INTEGER8     CCTK_REAL8
#define CCTK_BOOLEAN8     CCTK_REAL8
//#define CCTK_INTEGER8_VEC CCTK_REAL8_VEC
#define CCTK_BOOLEAN8_VEC CCTK_REAL8_VEC



// Create vectors, extract vector elements

#define vec8_set1(a)      (vec_splats(a))
#define vec8_set(a,b,c,d) ((vector4double){a,b,c,d})

#define vec8_b2r(b) ((b)?+1.0:-1.0)
#define vec8b_set(a,b,c,d)                                              \
  ((vector4double){vec8_b2r(a),vec8_b2r(b),vec8_b2r(c),vec8_b2r(d)})

#define vec8_elt0(x)  (vec_extract(x,0))
#define vec8_elt1(x)  (vec_extract(x,1))
#define vec8_elt2(x)  (vec_extract(x,2))
#define vec8_elt3(x)  (vec_extract(x,3))
#define vec8_elt(x,d) (vec_extract(x,d))



// Load and store vectors

// Load a vector from memory (aligned and unaligned); this loads from
// a reference to a scalar
#define vec8_load(p)   (vec_lda(0,(CCTK_REAL8*)&(p)))
#define vec8_loadu(p_)                                                  \
  ({                                                                    \
    CCTK_REAL8 const& p__=(p_);                                         \
    CCTK_REAL8& p = *(CCTK_REAL8*)&p__;                                 \
    vector4double v1, v2, vp;                                           \
    /* code taken from IBM's compiler documentation */                  \
    v1 = vec_ld(0,&p);         /* load the left part of the vector */   \
    v2 = vec_ld(32,&p);        /* load the right part of the vector */  \
    vp = vec_lvsl(0,&p);       /* generate control value */             \
    vec_perm(v1,v2,vp);        /* generate the aligned vector */        \
  })

// Load a vector from memory that may or may not be aligned, as
// decided by the offset and the vector size
#if VECTORISE_ALWAYS_USE_UNALIGNED_LOADS
// Implementation: Always use unaligned load
#  define vec8_loadu_maybe(off,p)             vec8_loadu(p)
#  define vec8_loadu_maybe3(off1,off2,off3,p) vec8_loadu(p)
#else
#  define vec8_loadu_maybe(off,p_)              \
  ({                                            \
    CCTK_REAL8 const& p__=(p_);                 \
    CCTK_REAL8 const& p=p__;                    \
    (off) % CCTK_REAL8_VEC_SIZE == 0 ?          \
      vec8_load(p) :                            \
      vec8_loadu(p);                            \
  })
#  if VECTORISE_ALIGNED_ARRAYS
// Assume all array x sizes are multiples of the vector size
#    define vec8_loadu_maybe3(off1,off2,off3,p) vec8_loadu_maybe(off1,p)
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
#define vec8_store(p,x)     (vec_sta(x,0,&(p)))
#define vec8_storeu(p_,x_)                              \
  ({                                                    \
    CCTK_REAL8& p__=(p_);                               \
    CCTK_REAL8_VEC x__=(x_);                            \
    CCTK_REAL8& p=p__;                                  \
    CCTK_REAL8_VEC x=x__;                               \
    CCTK_REAL8_VEC v1, v2, v3, vp, m1, m2, m3;          \
    /* code taken from IBM's compiler documentation */  \
    /* generate insert masks */                         \
    vp = vec_lvsr(0,&p);                                \
    m1 = k8lfalse;                                      \
    m2 = k8ltrue;                                       \
    m3 = vec_perm(m1,m2,vp);                            \
    /* get existing data */                             \
    v1 = vec_ld(0,&p);                                  \
    v2 = vec_ld(32,&p);                                 \
    /* permute and insert */                            \
    v3 = vec_perm(x,x,vp);                              \
    v1 = vec_sel(v1,v3,m3);                             \
    v2 = vec_sel(v3,v2,m3);                             \
    /* store data back */                               \
    vec_st(0,&p,v1);                                    \
    vec_st(32,&p,v2);                                   \
  })
#define vec8_store_nta(p,x) (vec_sta(x,0,&(p))) // this doesn't avoid the cache

// Store a partial vector (aligned and non-temporal)
#define vec8_store_partial_prepare(i,imin_,imax_)                       \
  bool v8stp_all;                                                       \
  CCTK_REAL8_VEC v8stp_mask;                                            \
  ({                                                                    \
    ptrdiff_t const imin__=(imin_);                                     \
    ptrdiff_t const imax__=(imax_);                                     \
    ptrdiff_t const imin=imin__;                                        \
    ptrdiff_t const imax=imax__;                                        \
                                                                        \
    v8stp_all = i>=imin and i+CCTK_REAL8_VEC_SIZE-1<imax;		\
                                                                        \
    if (not CCTK_BUILTIN_EXPECT(v8stp_all, true)) {                     \
      CCTK_REAL8_VEC vp_lo, vp_hi, mask_lo, mask_hi;			\
      /* this is correct but slow */                                    \
      /*                                                                \
      mask_lo = vec8b_set(i+0>=imin, i+1>=imin, i+2>=imin, i+3>=imin);  \
      mask_hi = vec8b_set(i+0<imax, i+1<imax, i+2<imax, i+3<imax);      \
      */                                                                \
      /* Note: vec_lvsl(i,p) =  &p[i] / 8 % 4                           \
         Note: vec_lvsr(i,p) = -&p[i] / 8 % 4                           \
         /8: 8 bytes per double                                         \
         %4: 4 doubles per vector                                       \
      */                                                                \
      /* We assume p[i] is aligned */                                   \
      /* Ensure at least one vector element is inside the active region */ \
      assert(i-imin>=-(CCTK_REAL8_VEC_SIZE-1));                         \
      vp_lo = vec_lvsl(8 * (i-imin), (CCTK_REAL*)0);                    \
      mask_lo = (i-imin >= 0 ?						\
		 k8ltrue :						\
		 vec_perm(k8lfalse, k8ltrue, vp_lo));			\
      /* Ensure at least one vector element is inside the active region */ \
      assert(i<imax);                                                   \
      vp_hi = vec_lvsl(8 * (i-imax), (CCTK_REAL*)0);                    \
      mask_hi =	(i-imax < -(CCTK_REAL8_VEC_SIZE-1) ?                    \
		 k8ltrue :						\
		 vec_perm(k8ltrue, k8lfalse, vp_hi));			\
      v8stp_mask = k8land(mask_lo, mask_hi);                            \
    }									\
  })
#define vec8_store_nta_partial(p_,x_)                           \
  ({                                                            \
    CCTK_REAL8&    p__=(p_);                                    \
    CCTK_REAL8_VEC x__=(x_);                                    \
    CCTK_REAL8&    p=p__;                                       \
    CCTK_REAL8_VEC x=x__;                                       \
    if (CCTK_BUILTIN_EXPECT(v8stp_all, true)) {                 \
      vec8_store(p, x);                                         \
    } else {                                                    \
      vec8_store(p, k8ifthen(v8stp_mask, x, vec8_load(p)));     \
    }                                                           \
  })

// Store a lower or higher partial vector (aligned and non-temporal);
// the non-temporal hint is probably ignored
#define vec8_store_nta_partial_lo(p_,x_,n)                              \
  ({                                                                    \
    CCTK_REAL8&    p__=(p_);                                            \
    CCTK_REAL8_VEC x__=(x_);                                            \
    CCTK_REAL8&    p=p__;                                               \
    CCTK_REAL8_VEC x=x__;                                               \
    CCTK_REAL8_VEC vp, mask;                                            \
    /* Ensure at least one and but all vector elements are active */    \
    assert(n>0 and n<CCTK_REAL8_VEC_SIZE-1);                            \
    vp = vec_lvsl(-8 * n, (CCTK_REAL*)0);                               \
    mask = vec_perm(k8ltrue, k8lfalse, vp);                             \
    vec8_store(p, k8ifthen(mask, x, vec8_load(p)));                     \
  })
#define vec8_store_nta_partial_hi(p_,x_,n)                              \
  ({                                                                    \
    CCTK_REAL8&    p__=(p_);                                            \
    CCTK_REAL8_VEC x__=(x_);                                            \
    CCTK_REAL8&    p=p__;                                               \
    CCTK_REAL8_VEC x=x__;                                               \
    CCTK_REAL8_VEC vp, mask;                                            \
    /* Ensure at least one but not all vector elements are active */    \
    assert(n>0 and n<CCTK_REAL8_VEC_SIZE-1);                            \
    vp = vec_lvsl(8 * n, (CCTK_REAL*)0);                                \
    mask = vec_perm(k8lfalse, k8ltrue, vp);                             \
    vec8_store(p, k8ifthen(mask, x, vec8_load(p)));                     \
  })
#define vec8_store_nta_partial_mid(p_,x_,nlo,nhi)                       \
  ({                                                                    \
    CCTK_REAL8&    p__=(p_);                                            \
    CCTK_REAL8_VEC x__=(x_);                                            \
    CCTK_REAL8     p=p__;                                               \
    CCTK_REAL8_VEC x=x__;                                               \
    CCTK_REAL8_VEC vp_lo, mask_lo;                                      \
    /* Ensure at least one but not all vector elements are active */    \
    assert(nlo>0 and nlo<CCTK_REAL8_VEC_SIZE-1);                        \
    vp_lo = vec_lvsl(-8 * nlo, (CCTK_REAL*)0);                          \
    mask_lo = vec_perm(k8lfalse, k8ltrue, vp_lo);                       \
    CCTK_REAL8_VEC vp_hi, mask_hi;                                      \
    /* Ensure at least one but not all vector elements are active */    \
    assert(nhi>0 and nhi<CCTK_REAL8_VEC_SIZE-1);                        \
    vp_hi = vec_lvsl(8 * nhi, (CCTK_REAL*)0);                           \
    mask_hi = vec_perm(k8lfalse, k8ltrue, vp_hi);                       \
    CCTK_REAL8_VEC mask;                                                \
    mask = vec_and(mask_lo, mask_hi);                                   \
    vec8_store(p, k8ifthen(mask, x, vec8_load(p)));                     \
  })



// Functions and operators

// Operators
#define k8neg(x) (vec_neg(x))

#define k8add(x,y) (vec_add(x,y))
#define k8sub(x,y) (vec_sub(x,y))
#define k8mul(x,y) (vec_mul(x,y))
#define k8div(x,y) (vec_swdiv_nochk(x,y))

// Fused multiply-add, defined as [+-]x*y[+-]z
#define k8madd(x,y,z)  (vec_madd(x,y,z))
#define k8msub(x,y,z)  (vec_msub(x,y,z))
#define k8nmadd(x,y,z) (vec_nmadd(x,y,z))
#define k8nmsub(x,y,z) (vec_nmsub(x,y,z))

// Cheap functions
#define k8copysign(x,y) (vec_cpsgn(y,x))
#define k8fabs(x)       (vec_abs(x))
#define k8fmax(x_,y_)                           \
  ({                                            \
    CCTK_REAL8_VEC x__=(x_);                    \
    CCTK_REAL8_VEC y__=(y_);                    \
    CCTK_REAL8_VEC x=x__;                       \
    CCTK_REAL8_VEC y=y__;                       \
    k8ifthen(k8cmplt(x,y),y,x);                 \
  })
#define k8fmin(x_,y_)                           \
  ({                                            \
    CCTK_REAL8_VEC x__=(x_);                    \
    CCTK_REAL8_VEC y__=(y_);                    \
    CCTK_REAL8_VEC x=x__;                       \
    CCTK_REAL8_VEC y=y__;                       \
    k8ifthen(k8cmpgt(x,y),y,x);                 \
  })
#define k8fnabs(x) (vec_nabs(x))
#define k8sgn(x_)                               \
  ({                                            \
    CCTK_REAL8_VEC x__=(x_);                    \
    CCTK_REAL8_VEC x=x__;                       \
    CCTK_REAL8_VEC one, zero, iszero;           \
    one = k8ltrue;                              \
    zero = k8sub(one, one);                     \
    iszero = k8cmpeq(x, zero);                  \
    k8ifthen(iszero, zero, k8copysign(one, x)); \
  })
#define k8sqrt(x) (vec_swsqrt_nochk(x))

// Expensive functions
#define K8REPL(f,x_)                            \
  ({                                            \
    CCTK_REAL8_VEC x__=(x_);                    \
    CCTK_REAL8_VEC x=x__;                       \
    vec8_set(f(vec8_elt0(x)),                   \
             f(vec8_elt1(x)),                   \
             f(vec8_elt2(x)),                   \
             f(vec8_elt3(x)));                  \
  })
#define K8REPL2S(f,x_,a_)                       \
  ({                                            \
    CCTK_REAL8_VEC x__=(x_);                    \
    CCTK_REAL8     a__=(a_);                    \
    CCTK_REAL8_VEC x=x__;                       \
    CCTK_REAL8     a=a__;                       \
    vec8_set(f(vec8_elt0(x),a),                 \
             f(vec8_elt1(x),a),                 \
             f(vec8_elt2(x),a),                 \
             f(vec8_elt3(x),a));                \
  })
#define K8REPL2(f,x_,y_)                        \
  ({                                            \
    CCTK_REAL8_VEC x__=(x_);                    \
    CCTK_REAL8_VEC y__=(y_);                    \
    CCTK_REAL8_VEC x=x__;                       \
    CCTK_REAL8_VEC y=y__;                       \
    vec8_set(f(vec8_elt0(x),vec8_elt0(y)),      \
             f(vec8_elt1(x),vec8_elt1(y)),      \
             f(vec8_elt2(x),vec8_elt2(y)),      \
             f(vec8_elt3(x),vec8_elt3(y)));     \
  })

#define k8acos(x)    K8REPL(acos,x)
#define k8acosh(x)   K8REPL(acosh,x)
#define k8asin(x)    K8REPL(asin,x)
#define k8asinh(x)   K8REPL(asinh,x)
#define k8atan(x)    K8REPL(atan,x)
#define k8atan2(x,y) K8REPL2(atan2,x,y)
#define k8atanh(x)   K8REPL(atanh,x)
#define k8cos(x)     K8REPL(cos,x)
#define k8cosh(x)    K8REPL(cosh,x)
#define k8exp(x)     K8REPL(exp,x)
#define k8log(x)     K8REPL(log,x)
#define k8pow(x,a)   K8REPL2S(pow,x,a)
#define k8sin(x)     K8REPL(sin,x)
#define k8sinh(x)    K8REPL(sinh,x)
#define k8tan(x)     K8REPL(tan,x)
#define k8tanh(x)    K8REPL(tanh,x)

// canonical true is +1.0, canonical false is -1.0
// >=0 is true, -0 is true, nan is false
#define k8lfalse                                                \
  ({ CCTK_REAL8_VEC dummy; vec_logical(dummy,dummy,0x0); })
#define k8ltrue                                                 \
  ({ CCTK_REAL8_VEC dummy; vec_logical(dummy,dummy,0xf); })
#define k8lnot(x)       (vec_not(x))
#define k8land(x,y)     (vec_and(x,y))
#define k8lor(x,y)      (vec_or(x,y))
#define k8lxor(x,y)     (vec_xor(x,y))
#define k8ifthen(x,y,z) (vec_sel(z,y,x))

#define k8cmpeq(x,y) (vec_cmpeq(x,y))
#define k8cmpne(x,y) (k8lnot(vec_cmpeq(x,y)))
#define k8cmpgt(x,y) (vec_cmpgt(x,y))
#define k8cmpge(x,y) (k8lnot(vec_cmplt(x,y)))
#define k8cmplt(x,y) (vec_cmplt(x,y))
#define k8cmple(x,y) (vec_not(vec_cmpgt(x,y)))
