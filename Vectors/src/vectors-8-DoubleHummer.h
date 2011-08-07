// Vectorise using IBM's Blue Gene/P Double Hummer (Power)

// Use the type double _Complex directly, without introducing a wrapper class
// Use macros instead of inline functions



#include <builtins.h>



#define vec8_architecture "Double Hummer"

// Vector type corresponding to CCTK_REAL
#define CCTK_REAL8_VEC double _Complex

// Number of vector elements in a CCTK_REAL_VEC
#define CCTK_REAL8_VEC_SIZE 2



// Create vectors, extract vector elements

#define vec8_set1(a)  (__cmplx(a,a))
#define vec8_set(a,b) (__cmplx(a,b))

#define vec8_elt0(x) (__creal(x))
#define vec8_elt1(x) (__cimag(x))
#define vec8_elt(x_,d)                          \
  ({                                            \
    CCTK_REAL8_VEC const xx=(x_);               \
    CCTK_REAL8_VEC const x=xx;                  \
    CCTK_REAL8 a;                               \
    switch (d) {                                \
    case 0: a=vec8_elt0(x); break;              \
    case 1: a=vec8_elt1(x); break;              \
    }                                           \
    a;                                          \
  })



// Load and store vectors

// Load a vector from memory (aligned and unaligned); this loads from
// a reference to a scalar
#define vec8_load(p)  (__lfpd((CCTK_REAL8 *)&(p)))
#if ! VECTORISE_ALWAYS_USE_ALIGNED_LOADS
#  define vec8_load_off1(p_)                    \
  ({                                            \
    CCTK_REAL8 const& pp=(p_);                  \
    CCTK_REAL8 const& p=pp;                     \
    vec8_set((&p)[0],(&p)[1]);                  \
  })
#else
#if 0
#  define vec8_load_off1(p_)                                    \
  ({                                                            \
    CCTK_REAL8 const& pp=(p_);                                  \
    CCTK_REAL8 const& p=pp;                                     \
    CCTK_REAL8_VEC const lo = __lfxd((CCTK_REAL8 *)(&p-1));     \
    CCTK_REAL8_VEC const hi = __lfxd((CCTK_REAL8 *)(&p+1));     \
    __fpsel(vec8_set(-1.0,+1.0),lo,hi);                         \
  })
#endif
#  define vec8_load_off1(p_)                            \
  ({                                                    \
    CCTK_REAL8 const& pp=(p_);                          \
    CCTK_REAL8 const& p=pp;                             \
    CCTK_REAL8_VEC const lo = vec8_load((&p)[-1]);      \
    CCTK_REAL8_VEC const hi = vec8_load((&p)[+1]);      \
    __fxmr(__fpsel(vec8_set(+1.0,-1.0),lo,hi));         \
  })
#endif
#define vec8_loadu(p_)                          \
  ({                                            \
    CCTK_REAL8 const& pp=(p_);                  \
    CCTK_REAL8 const& p=pp;                     \
    int const off = (ptrdiff_t)&p & 0xf;        \
    off==0 ? vec8_load(p) : vec8_load_off1(p);  \
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
    CCTK_REAL8 const& pp=(p_);                  \
    CCTK_REAL8 const& p=pp;                     \
    (off) % CCTK_REAL8_VEC_SIZE == 0 ?          \
      vec8_load(p) :                            \
      vec8_load_off1(p);                        \
  })
#  if VECTORISE_ALIGNED_ARRAYS
// Assume all array x sizes are multiples of the vector size
#    define vec8_loadu_maybe3(off1,off2,off3,p) vec8_loadu_maybe(off1,p)
#  else
#    define vec8_loadu_maybe3(off1,off2,off3,p_)        \
  ({                                                    \
    CCTK_REAL8 const& pp=(p_);                          \
    CCTK_REAL8 const& p=pp;                             \
    ((off2) % CCTK_REAL8_VEC_SIZE != 0 ||               \
     (off3) % CCTK_REAL8_VEC_SIZE != 0) ?               \
      vec8_loadu(p) :                                   \
      vec8_loadu_maybe(off1,p);                         \
  })
#  endif
#endif

// Store a vector to memory (aligned and non-temporal); this stores to
// a reference to a scalar
#define vec8_store(p,x)     (__stfpd(&(p),x))
#define vec8_storeu(p,x)    (__stfpd(&(p),x)) // this may not work
#define vec8_store_nta(p,x) (__stfpd(&(p),x)) // this doesn't avoid the cache

// Store a lower or higher partial vector (aligned and non-temporal);
// the non-temporal hint is probably ignored
#define vec8_store_nta_partial_lo(p,x,n) ((&(p))[0]=vec8_elt0(x))
#define vec8_store_nta_partial_hi(p,x,n) ((&(p))[1]=vec8_elt1(x))
#define vec8_store_nta_partial_mid(p,x,nlo,nhi) assert(0)



// Functions and operators

// Operators
#define k8pos(x) (x)
#define k8neg(x) (__fpneg(x))

#define k8add(x,y) (__fpadd(x,y))
#define k8sub(x,y) (__fpsub(x,y))
#define k8mul(x,y) (__fpmul(x,y))
// Estimate for reciprocal
#define k8inv_init(x) (__fpre(x))
// One Newton iteration for reciprocal
#define k8inv_iter(x_,r_)                               \
  ({                                                    \
    CCTK_REAL8_VEC const xx=(x_);                       \
    CCTK_REAL8_VEC const x=xx;                          \
    CCTK_REAL8_VEC const rr=(r_);                       \
    CCTK_REAL8_VEC const r=rr;                          \
    /* r + r * (vec8_set1(1.0) - x*r); */               \
    k8madd(r, k8nmsub(x, r, vec8_set1(1.0)), r);        \
  })
// Reciprocal: First estimate, then apply two Newton iterations
#define k8inv(x_)                               \
  ({                                            \
    CCTK_REAL8_VEC const xx=(x_);               \
    CCTK_REAL8_VEC const x=xx;                  \
    CCTK_REAL8_VEC const r0 = k8inv_init(x);    \
    CCTK_REAL8_VEC const r1 = k8inv_iter(x,r0); \
    CCTK_REAL8_VEC const r2 = k8inv_iter(x,r1); \
    r2;                                         \
  })
#define k8div(x,y) (__fpmul(x,k8inv(y)))

// Fused multiply-add, defined as [+-]x*y[+-]z
#define k8madd(x,y,z)  (__fpmadd(z,x,y))
#define k8msub(x,y,z)  (__fpmsub(z,x,y))
#define k8nmadd(x,y,z) (__fpnmadd(z,x,y))
#define k8nmsub(x,y,z) (__fpnmsub(z,x,y))

// Cheap functions
#define k8fabs(x) (__fpabs(x))
#define k8fmax(x_,y_)                           \
  ({                                            \
    CCTK_REAL8_VEC const xx=(x_);               \
    CCTK_REAL8_VEC const x=xx;                  \
    CCTK_REAL8_VEC const yy=(y_);               \
    CCTK_REAL8_VEC const y=yy;                  \
    __fpsel(k8sub(y,x),x,y);                    \
  })
#define k8fmin(x_,y_)                           \
  ({                                            \
    CCTK_REAL8_VEC const xx=(x_);               \
    CCTK_REAL8_VEC const x=xx;                  \
    CCTK_REAL8_VEC const yy=(y_);               \
    CCTK_REAL8_VEC const y=yy;                  \
    __fpsel(k8sub(x,y),x,y);                    \
  })
#define k8fnabs(x) (__fpnabs(x))

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
#define k8sqrt(x)  K8REPL(sqrt,x)

#define k8ifpos(x,y,z) fpsel(y,z,x)
