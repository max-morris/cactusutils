// Fallback vectorisation implementation: Do not vectorise

// We use macros here, so that we are not surprised by compilers which
// don't like to inline functions. This should also make debug builds
// (which may not inline) more efficient.



#include <assert.h>
#include <math.h>



#define vec4_architecture "scalar (no vectorisation, 32-bit precision)"

// Use CCTK_REAL4
#define CCTK_REAL4_VEC CCTK_REAL4

// Number of vector elements in a vector
#define CCTK_REAL4_VEC_SIZE 1



// Create a vector replicating a scalar
#define vec4_set1(a) (a)
// Create a vector from N scalars
#define vec4_set(a) (a)

// Access vectors elements
#define vec4_elt0(x) (x)
#define vec4_elt(x,d) (x)



// Load an aligned vector from memory
#define vec4_load(p) (p)
// Load an unaligned vector from memory
#define vec4_loadu(p) (p)

// Load a vector from memory that may or may not be aligned, as
// decided by the offset and the vector size. These functions are
// useful e.g. for loading neightbouring grid points while evaluating
// finite differencing stencils.
#define vec4_loadu_maybe(off,p) (p)
#define vec4_loadu_maybe3(off1,off2,off3,p) (p)

// Aligned store
#define vec4_store(p,x) ((p)=(x))
// Unaligned store
#define vec4_store_nta(p,x) ((p)=(x))

// Store the n lower elements of a vector to memory
#define vec4_store_nta_partial_lo(p,x,n) (assert(0))
// Store the n higher elements of a vector into memory. This stores
// the vector elements into memory locations as if element 0 were
// stored at p.
#define vec4_store_nta_partial_hi(p,x,n) (assert(0))
#define vec4_store_nta_partial_mid(p,x,nlo,nhi) (assert(0))



// Operators
#define k4pos(x) (+(x))
#define k4neg(x) (-(x))

#define k4add(x,y) ((x)+(y))
#define k4sub(x,y) ((x)-(y))
#define k4mul(x,y) ((x)*(y))
#define k4div(x,y) ((x)/(y))

// Fused multiply-add, defined as [+-]x*y[+-]z
#define k4madd(x,y,z)  (+(x)*(y)+(z))
#define k4msub(x,y,z)  (+(x)*(y)-(z))
#define k4nmadd(x,y,z) (-(x)*(y)-(z))
#define k4nmsub(x,y,z) (-(x)*(y)+(z))

// Functions
#define k4exp(x)    (expf(x))
#define k4fabs(x)   (fabsf(x))
#define k4fmax(x,y) (fmaxf(x,y))
#define k4fmin(x,y) (fminf(x,y))
#define k4fnabs(x)  (-fabsf(x))
#define k4log(x)    (logf(x))
#define k4pow(x,a)  (powf(x,a))
#define k4sqrt(x)   (sqrtf(x))

#ifdef __cplusplus
#  define SGN(x) std::signbit(x)
#else
#  define SGN(x) signbit(x)
#endif

#define k4ifpos(x,y,z) (SGN(x)?(z):(y))
