// Fallback vectorisation implementation: Do not vectorise

// We use macros here, so that we are not surprised by compilers which
// don't like to inline functions. This should also make debug builds
// (which may not inline) more efficient.



#include <assert.h>
#include <math.h>



#define vec8_architecture "scalar (no vectorisation, 64-bit precision)"

// Use CCTK_REAL8
#define CCTK_REAL8_VEC CCTK_REAL8

// Number of vector elements in a vector
#define CCTK_REAL8_VEC_SIZE 1



// Create a vector replicating a scalar
#define vec8_set1(a) (a)
// Create a vector from N scalars
#define vec8_set(a) (a)

// Access vectors elements
#define vec8_elt0(x) (x)
#define vec8_elt(x,d) (x)



// Load an aligned vector from memory
#define vec8_load(p) (p)
// Load an unaligned vector from memory
#define vec8_loadu(p) (p)

// Load a vector from memory that may or may not be aligned, as
// decided by the offset and the vector size. These functions are
// useful e.g. for loading neightbouring grid points while evaluating
// finite differencing stencils.
#define vec8_loadu_maybe(off,p) (p)
#define vec8_loadu_maybe3(off1,off2,off3,p) (p)

// Aligned store
#define vec8_store(p,x) ((p)=(x))
// Unaligned store
#define vec8_store_nta(p,x) ((p)=(x))

#define vec8_store_partial_prepare(i,imin,imax) ((void)0)
#define vec8_store_nta_partial(p,x) (vec8_store_nta(p,x))
// Store the n lower elements of a vector to memory
#define vec8_store_nta_partial_lo(p,x,n) (assert(0))
// Store the n higher elements of a vector into memory. This stores
// the vector elements into memory locations as if element 0 were
// stored at p.
#define vec8_store_nta_partial_hi(p,x,n) (assert(0))
#define vec8_store_nta_partial_mid(p,x,nlo,nhi) (assert(0))



// Operators
#define k8neg(x) (-(x))

#define k8add(x,y) ((x)+(y))
#define k8sub(x,y) ((x)-(y))
#define k8mul(x,y) ((x)*(y))
#define k8div(x,y) ((x)/(y))

// Fused multiply-add, defined as [+-]x*y[+-]z
#define k8madd(x,y,z)  (+(x)*(y)+(z))
#define k8msub(x,y,z)  (+(x)*(y)-(z))
#define k8nmadd(x,y,z) (-(x)*(y)-(z))
#define k8nmsub(x,y,z) (-(x)*(y)+(z))

// Functions
#define k8cos(x)    (cos(x))
#define k8exp(x)    (exp(x))
#define k8fabs(x)   (fabs(x))
#define k8fmax(x,y) (fmax(x,y))
#define k8fmin(x,y) (fmin(x,y))
#define k8fnabs(x)  (-fabs(x))
#define k8log(x)    (log(x))
#define k8pow(x,a)  (pow(x,a))
#define k8sin(x)    (sin(x))
#define k8sqrt(x)   (sqrt(x))
#define k8tan(x)    (tan(x))

#ifdef __cplusplus
#  define k8sgn(x) ({ using namespace std; signbit(x); })
#else
#  define k8sgn(x) (signbit(x))
#endif

#define k8ifmsb(x,y,z) (k8sgn(x)?(y):(z))
