// Vectorise using IBM's Altivec VSX (Power)

// Use the type vector double directly, without introducing a wrapper class
// Use macros instead of inline functions



#include <altivec.h>



#define vec8_architecture "VSX"

// Vector type corresponding to CCTK_REAL
#define CCTK_REAL8_VEC vector double

// Number of vector elements in a CCTK_REAL_VEC
#define CCTK_REAL8_VEC_SIZE 2



// Create vectors, extract vector elements

#define vec8_set1(a)  (vec_splats(a))
#define vec8_set(a,b)                           \
  ({                                            \
    CCTK_REAL8_VEC x;                           \
    x[0]=(a);                                   \
    x[1]=(b);                                   \
    x;                                          \
  })

#define vec8_elt0(x) ((x)[0])
#define vec8_elt1(x) ((x)[1])
#define vec8_elt(x,d) ((x)[d])



// Load and store vectors

// Load a vector from memory (aligned and unaligned); this loads from
// a reference to a scalar
#define vec8_load(p)  (*(CCTK_REAL8_VEC const*)&(p))
#define vec8_loadu(p) (*(CCTK_REAL8_VEC const*)&(p))

// Load a vector from memory that may or may not be aligned, as
// decided by the offset and the vector size
#define vec8_loadu_maybe(off,p)             (vec8_loadu(p))
#define vec8_loadu_maybe3(off1,off2,off3,p) (vec8_loadu(p))

// Store a vector to memory (aligned and non-temporal); this stores to
// a reference to a scalar
#define vec8_store(p,x)  (*(CCTK_REAL8_VEC*)&(p)=(x))
#define vec8_storeu(p,x) (*(CCTK_REAL8_VEC*)&(p)=(x))
// stvxl instruction doesn't exist for double precision
#define vec8_store_nta(p,x) vec8_store(p,x)

// Store a lower or higher partial vector (aligned and non-temporal);
// the non-temporal hint is probably ignored
#define vec8_store_nta_partial_lo(p,x,n) ((&(p))[0]=(x)[0])
#define vec8_store_nta_partial_hi(p,x,n) ((&(p))[1]=(x)[1])
#define vec8_store_nta_partial_mid(p,x,nlo,nhi) (assert(0))



// Functions and operators

// Operators
#define k8pos(x) (+(x))
#define k8neg(x) (-(x))

#define k8add(x,y) ((x)+(y))
#define k8sub(x,y) ((x)-(y))
#define k8mul(x,y) ((x)*(y))
#define k8div(x,y) ((x)/(y))

// Fused multiply-add, defined as [+-]x*y[+-]z
#define k8madd(x,y,z)  (vec_madd(x,y,z))
#define k8msub(x,y,z)  (vec_msub(x,y,z))
#define k8nmadd(x,y,z) (vec_nmadd(x,y,z))
#define k8nmsub(x,y,z) (vec_nmsub(x,y,z))

// Cheap functions
#define k8fabs(x)   (vec_abs(x))
#define k8fmax(x,y) (vec_max(x,y))
#define k8fmin(x,y) (vec_min(x,y))
#define k8fnabs(x)  (vec_nabs(x))

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

#define k8ifpos(x,y,z)                                                  \
  (vec_sel((y), (z), vec_sra(vec_convert((x), &(vector long long*)0), 63)))
