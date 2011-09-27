// -*-C-*-

// pown; note that Apple optimises pow(,2) but not pown(,2)
// __builtin_expect
// Boost, <boost/preprocessor/...>
// mad

#pragma OPENCL EXTENSION cl_khr_fp64    : enable
#pragma OPENCL EXTENSION cl_amd_printf  : enable
#pragma OPENCL EXTENSION cl_intel_printf: enable



////////////////////////////////////////////////////////////////////////////////



// doubleV           vector of double
// convert_doubleV   convert to doubleV
// longV             vector of long (same size as double)
// indicesV          longV containing (0,1,2,...)
// vloadV            load unaligned vector
// vstoreV           store unaligned vector

#if VECTOR_SIZE_I == 1
#  define doubleV            double
#  define convert_doubleV    convert_double
#  define longV              long
#  define indicesV           ((longV)(0))
#  define vloadV(i,p)        ((p)[i])
#  define vstoreV(x,i,p)     ((p)[i]=(x))
#elif VECTOR_SIZE_I == 2
#  define doubleV            double2
#  define convert_doubleV    convert_double2
#  define longV              long2
#  define indicesV           ((longV)(0,1))
#  define vloadV             vload2
#  define vstoreV            vstore2
#elif VECTOR_SIZE_I == 4
#  define doubleV            double4
#  define convert_doubleV    convert_double4
#  define longV              long4
#  define indicesV           ((longV)(0,1,2,3))
#  define vloadV             vload4
#  define vstoreV            vstore4
#elif VECTOR_SIZE_I == 8
#  define doubleV            double8
#  define convert_doubleV    convert_double8
#  define longV              long8
#  define indicesV           ((longV)(0,1,2,3,4,5,6,7))
#  define vloadV             vload8
#  define vstoreV            vstore8
#elif VECTOR_SIZE_I == 16
#  define doubleV            double16
#  define convert_doubleV    convert_double16
#  define longV              long16
#  define indicesV           ((longV)(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15))
#  define vloadV             vload16
#  define vstoreV            vstore16
#else
#  error
#endif

#if VECTOR_SIZE_J!=1 || VECTOR_SIZE_K!=1
#  error
#endif



#define CCTK_REAL double
#define CCTK_INT  int
#define CCTK_LONG long

#define CCTK_REAL_VEC_SIZE VECTOR_SIZE_I
#define CCTK_REAL_VEC      doubleV
#define CCTK_LONG_VEC      longV
#define convert_real_vec   convert_doubleV
#define vec_index          convert_real_vec(indicesV)



// vec_loada               load aligned vector
// vec_loadu               load unaligned vector
// vec_load                load regular vector
// vec_loadu_maybe3        load unaligned vector
// vec_storea              store aligned vector
// vec_storeu              store unaligned vector
// vec_store_nta           store regular vector
// vec_store_nta_partial   store regular vector partially

// VECTORISE_ALIGNED_ARRAYS assumes that all grid points [0,j,k] are
// aligned, and arrays are padded as necessary

#define vec_loada(p) (* (CCTK_REAL_VEC const global *) & (p))
#define vec_loadu(p) vloadV(0, & (p))

#if VECTORISE_ALIGNED_ARRAYS
#  define vec_load(p) vec_loada(p)
#  define vec_loadu_maybe3(off1,off2,off3, p)                           \
  ((off1) % CCTK_REAL_VEC_SIZE == 0 ? vec_loada(p) : vec_loadu(p))
#else
#  define vec_load(p) vec_loadu(p)
#  define vec_loadu_maybe3(off1,off2,off3, p) vec_loadu(p)
#endif

#define vec_storea(p, x) (* (CCTK_REAL_VEC global *) & (p) = (x))
#define vec_storeu(p, x) vstoreV(x, 0, & (p))

#if VECTORISE_ALIGNED_ARRAYS
#  define vec_store_nta(p, x) vec_storea(p, x)
#else
#  define vec_store_nta(p, x) vec_storeu(p, x)
#endif

#if CCTK_REAL_VEC_SIZE == 1

#  define vec_store_nta_partial(p, x)                                   \
  do {                                                                  \
    if (__builtin_expect(lc_vec_any_I && lc_vec_any_J && lc_vec_any_K, true)) \
    {                                                                   \
      vec_store_nta(p, x);                                              \
    }                                                                   \
  } while(0)

#elif CCTK_REAL_VEC_SIZE == 2

#  define vec_store_nta_partial(p, x)                                   \
  do {                                                                  \
    if (__builtin_expect(lc_vec_any_I && lc_vec_any_J && lc_vec_any_K, true)) \
    {                                                                   \
      if (__builtin_expect(lc_vec_all_I, true)) {                       \
        vec_store_nta(p, x);                                            \
      } else {                                                          \
        if (lc_vec_lo_I) {                                              \
          (&(p))[0] = (x).s[0];                                         \
        } else {                                                        \
          (&(p))[1] = (x).s[1];                                         \
        }                                                               \
      }                                                                 \
    }                                                                   \
  } while (0)

#else

#  define vec_store_nta_partial(p, x)                                   \
  do {                                                                  \
    if (__builtin_expect(lc_vec_any_I && lc_vec_any_J && lc_vec_any_K, true)) \
    {                                                                   \
      if (__builtin_expect(lc_vec_all_I, true)) {                       \
        vec_store_nta(p, x);                                            \
      } else {                                                          \
        /* select(a,b,c) = MSB(c) ? b : a */                            \
        vec_store_nta(p, select(vec_load(p), x, lc_vec_mask_I));        \
      }                                                                 \
    }                                                                   \
  } while(0)

#endif



#define kpos(x) (+(x))
#define kneg(x) (-(x))

#define kadd(x,y) ((x)+(y))
#define ksub(x,y) ((x)-(y))
#define kmul(x,y) ((x)*(y))
#define kdiv(x,y) ((x)/(y))

#define kmadd(x,y,z)  mad(x,y,z)   // faster than fma(x,y,z)
#define kmsub(x,y,z)  mad(x,y,-(z))
#define knmadd(x,y,z) (-mad(x,y,z))
#define knmsub(x,y,z) (-mad(x,y,(-z)))

#define kfabs(x)   fabs(x)
#define kfmax(x,y) fmax(x,y)
#define kfmin(x,y) fmin(x,y)
#define kfnabs(x)  (-fabs(x))
#define ksqrt(x)   sqrt(x)

#define kcos(x)   cos(x)
#define kexp(x)   exp(x)
#define klog(x)   log(x)
#define kpow(x,n) pown(x,n)
#define ksin(x)   sin(x)
#define ktan(x)   tan(x)

// Choice   [sign(x)>0 ? y : z]
#define kifpos(x,y,z) select(y,z,x)



#ifdef __APPLE__

// Apple's pow implementation is much better than their pown
#  undef pown
#  define pown pow

inline CCTK_REAL myfabs (CCTK_REAL x);
inline CCTK_REAL myfabs (CCTK_REAL x)
{
  return x>=0 ? x : -x;
}

inline CCTK_REAL mycos1 (CCTK_REAL x);
inline CCTK_REAL mycos1 (CCTK_REAL x)
{
  // 0<=x<=pi/2
  CCTK_REAL const c1 = +1.0;
  CCTK_REAL const c2 = -1.0/2.0;
  CCTK_REAL const c3 = +1.0/24.0;
  CCTK_REAL const c4 = -1.0/720.0;
  CCTK_REAL const c5 = +1.0/40320.0;
  CCTK_REAL const c6 = -1.0/3628800.0;
  CCTK_REAL const c7 = +1.0/479001600.0;
  CCTK_REAL const c8 = -1.0/87178291200.0;
  CCTK_REAL const x2 = pown(x,2);
  return (c1 + x2 *
          (c2 + x2 *
           (c3 + x2 *
            (c4 + x2 *
             (c5 + x2 *
              (c6 + x2 *
               (c7 + x2 * c8))))));
}

inline CCTK_REAL mycos (CCTK_REAL x);
inline CCTK_REAL mycos (CCTK_REAL x)
{
  x = myfabs(x);
  x = fmod(x,2*M_PI);
  if (x>M_PI) x=M_PI-x;
  bool const isneg = x>M_PI/2;
  if (isneg) x=M_PI/2-x;
  CCTK_REAL y = mycos1(x);
  if (isneg) y=-y;
  return y;
}

#  undef cos
#  define cos mycos



#  undef kcos
// Apple's OpenCL compiler segfaults when calling cos on a vector, so
// we serialise this operation explicitly
inline CCTK_REAL_VEC kcos (CCTK_REAL_VEC const x);
#  if CCTK_REAL_VEC_SIZE==1
inline CCTK_REAL_VEC kcos (CCTK_REAL_VEC const x)
{
  return cos(x);
}
#  elif CCTK_REAL_VEC_SIZE==2
inline CCTK_REAL_VEC kcos (CCTK_REAL_VEC const x)
{
  return (CCTK_REAL_VEC)(cos(x.s[0]), cos(x.s[1]));
}
#  else
#    error
#  endif

#endif



////////////////////////////////////////////////////////////////////////////////



#define dim 3

#if SIZEOF_PTRDIFF_T == 4
typedef unsigned int cl_size_t;
typedef int          cl_ptrdiff_t;
#elif SIZEOF_PTRDIFF_T == 8
typedef unsigned long cl_size_t;
typedef long          cl_ptrdiff_t;
#else
#  error
#endif



typedef struct {
  // Doubles first, then ints, to ensure proper alignment
  // Coordinates:
  double cctk_origin_space[dim];
  double cctk_delta_space[dim];
  double cctk_time;
  double cctk_delta_time;
  // Grid structure properties:
  cl_ptrdiff_t cctk_gsh[dim];
  cl_ptrdiff_t cctk_lbnd[dim];
  cl_ptrdiff_t cctk_lssh[dim];
  cl_ptrdiff_t cctk_lsh[dim];
  // Loop settings:
  cl_ptrdiff_t lmin[dim];       // loop region
  cl_ptrdiff_t lmax[dim];
  cl_ptrdiff_t imin[dim];       // active region
  cl_ptrdiff_t imax[dim];
} cGH;



// Cactus compatibility definitions

#define CCTK_ATTRIBUTE_UNUSED __attribute__((__unused__))
#define CCTK_BUILTIN_EXPECT   __builtin_expect

#define DECLARE_CCTK_ARGUMENTS                                          \
  cl_ptrdiff_t constant *restrict const cctk_lbnd = cctkGH->cctk_lbnd;  \
  cl_ptrdiff_t constant *restrict const cctk_lsh  = cctkGH->cctk_lsh;   \
  cl_ptrdiff_t constant *restrict const imin      = cctkGH->imin;       \
  cl_ptrdiff_t constant *restrict const imax      = cctkGH->imax;       \
  CCTK_REAL const cctk_time = cctkGH->cctk_time;
 
#define CCTK_GFINDEX3D(cctkGH,i,j,k)                                    \
  ((i) + cctkGH->cctk_lsh[0] * ((j) + cctkGH->cctk_lsh[1] * (k)))
 
#define CCTK_ORIGIN_SPACE(d) (cctkGH->cctk_origin_space[d])
#define CCTK_DELTA_SPACE(d)  (cctkGH->cctk_delta_space[d])
#define CCTK_DELTA_TIME      (cctkGH->cctk_delta_time)



// Kranc compatibility definitions

#define Pi        M_PI
#define ToReal(x) ((CCTK_REAL_VEC)(x))
#define INV(x)    (1.0/(x))
#define SQR(x)    (pown((x),2))

#define KRANC_GFOFFSET3D(u,i,j,k)                       \
  vec_loadu_maybe3(i,j,k,(u)[di*(i)+dj*(j)+dk*(k)])



////////////////////////////////////////////////////////////////////////////////



#define LC_SET_GROUP_VARS(D)                                            \
  ptrdiff_t const ind##D __attribute__((__unused__)) =                  \
    (lc_off##D + VECTOR_SIZE_##D * UNROLL_SIZE_##D *                    \
     (lc_grp##D + GROUP_SIZE_##D *                                      \
      (lc_til##D + TILE_SIZE_##D * lc_grd##D)));                        \
  bool const lc_grp_any_##D __attribute__((__unused__)) =               \
    ind##D + VECTOR_SIZE_##D * UNROLL_SIZE_##D - 1 >= lc_##D##min &&    \
    ind##D < lc_##D##max;

#define vecVI indicesV
#define vecVJ ((CCTK_LONG_VEC)0)
#define vecVK ((CCTK_LONG_VEC)0)

#define LC_SET_VECTOR_VARS(IND,D)                                       \
  ptrdiff_t const IND __attribute__((__unused__)) =                     \
    (lc_off##D + VECTOR_SIZE_##D *                                      \
     (lc_unr##D + UNROLL_SIZE_##D *                                     \
      (lc_grp##D + GROUP_SIZE_##D *                                     \
       (lc_til##D + TILE_SIZE_##D * lc_grd##D))));                      \
  bool const lc_vec_trivial_##D __attribute__((__unused__)) =           \
    VECTOR_SIZE_##D * UNROLL_SIZE_##D == 1;                             \
  bool const lc_vec_any_##D __attribute__((__unused__)) =               \
    lc_vec_trivial_##D ||                                               \
    (IND+VECTOR_SIZE_##D-1 >= lc_##D##min && IND < lc_##D##max);        \
  bool const lc_vec_lo_##D __attribute__((__unused__)) =                \
    lc_vec_trivial_##D ||                                               \
    IND >= lc_##D##min;                                                 \
  bool const lc_vec_hi_##D __attribute__((__unused__)) =                \
    lc_vec_trivial_##D ||                                               \
    IND+VECTOR_SIZE_##D-1 < lc_##D##max;                                \
  bool const lc_vec_all_##D __attribute__((__unused__)) =               \
    lc_vec_trivial_##D ||                                               \
    lc_vec_lo_##D && lc_vec_hi_##D;                                     \
  CCTK_LONG_VEC const lc_vec_mask_##D __attribute__((__unused__)) =     \
    lc_vec_trivial_##D ?                                                \
    (CCTK_LONG_VEC)true :                                               \
    (CCTK_LONG_VEC)IND+vecV##D >= (CCTK_LONG_VEC)lc_##D##min &&         \
    (CCTK_LONG_VEC)IND+vecV##D <  (CCTK_LONG_VEC)lc_##D##max;

#define LC_LOOP3VEC(name,                                               \
                    i,j,k,                                              \
                    imin,jmin,kmin,                                     \
                    imax,jmax,kmax,                                     \
                    ilsh,jlsh,klsh,                                     \
                    vecsize)                                            \
  do {                                                                  \
    typedef int lc_loop3_##name;                                        \
                                                                        \
    ptrdiff_t const lc_Imin = (imin);                                   \
    ptrdiff_t const lc_Jmin = (jmin);                                   \
    ptrdiff_t const lc_Kmin = (kmin);                                   \
    ptrdiff_t const lc_Imax = (imax);                                   \
    ptrdiff_t const lc_Jmax = (jmax);                                   \
    ptrdiff_t const lc_Kmax = (kmax);                                   \
    ptrdiff_t const lc_offI = cctkGH->lmin[0]; /* offset */             \
    ptrdiff_t const lc_offJ = cctkGH->lmin[1];                          \
    ptrdiff_t const lc_offK = cctkGH->lmin[2];                          \
    ptrdiff_t const lc_grpI = get_local_id(0); /* index in group */     \
    ptrdiff_t const lc_grpJ = get_local_id(1);                          \
    ptrdiff_t const lc_grpK = get_local_id(2);                          \
    ptrdiff_t const lc_grdI = get_group_id(0); /* index in grid */      \
    ptrdiff_t const lc_grdJ = get_group_id(1);                          \
    ptrdiff_t const lc_grdK = get_group_id(2);                          \
                                                                        \
    ptrdiff_t const lc_imin = lc_Imin;                                  \
    ptrdiff_t const lc_imax = lc_Imax;                                  \
                                                                        \
    for (ptrdiff_t lc_tilK = 0; lc_tilK < TILE_SIZE_K; ++lc_tilK) {     \
    for (ptrdiff_t lc_tilJ = 0; lc_tilJ < TILE_SIZE_J; ++lc_tilJ) {     \
    for (ptrdiff_t lc_tilI = 0; lc_tilI < TILE_SIZE_I; ++lc_tilI) {     \
                                                                        \
      LC_SET_GROUP_VARS(I);                                             \
      LC_SET_GROUP_VARS(J);                                             \
      LC_SET_GROUP_VARS(K);                                             \
      if (lc_grp_any_K) {                                               \
      if (lc_grp_any_J) {                                               \
      if (lc_grp_any_I) {                                               \
                                                                        \
        _Pragma("unroll")                                               \
          for (ptrdiff_t lc_unrK = 0; lc_unrK < UNROLL_SIZE_K; ++lc_unrK) { \
        _Pragma("unroll")                                               \
          for (ptrdiff_t lc_unrJ = 0; lc_unrJ < UNROLL_SIZE_J; ++lc_unrJ) { \
        _Pragma("unroll")                                               \
          for (ptrdiff_t lc_unrI = 0; lc_unrI < UNROLL_SIZE_I; ++lc_unrI) { \
                                                                        \
          LC_SET_VECTOR_VARS(i,I);                                      \
          LC_SET_VECTOR_VARS(j,J);                                      \
          LC_SET_VECTOR_VARS(k,K);                                      \
                                                                        \
          {
          
#define LC_ENDLOOP3VEC(name)                            \
          }                                             \
        }                                               \
        }                                               \
        }                                               \
      }                                                 \
      }                                                 \
      }                                                 \
    }                                                   \
    }                                                   \
    }                                                   \
    typedef lc_loop3_##name lc_ensure_proper_nesting;   \
  } while(0)
