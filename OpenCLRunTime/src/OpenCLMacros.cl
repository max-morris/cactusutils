// -*-C-*-

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

////////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_ATTRIBUTE_UNUSED
#define CCTK_ATTRIBUTE_UNUSED __attribute__((__unused__))
#else
#define CCTK_ATTRIBUTE_UNUSED
#endif

#ifdef HAVE_BUILTIN_EXPECT
#define CCTK_BUILTIN_EXPECT(a, b) __builtin_expect(a, b)
#else
#define CCTK_BUILTIN_EXPECT(a, b) (a)
#endif

#ifdef HAVE_PRAGMA_UNROLL
#define CCTK_UNROLL _Pragma("unroll")
#else
#define CCTK_UNROLL
#endif

// doubleV           vector of double
// convert_doubleV   convert to doubleV
// longV             vector of long (same size as double)
// indicesV          longV containing (0,1,2,...)
// vloadV            load unaligned vector
// vstoreV           store unaligned vector

#if VECTOR_SIZE_I == 1
#define doubleV double
#define convert_doubleV convert_double
#define longV long
#define indicesV ((longV)(0))
#define vloadV(i, p) ((p)[i])
#define vstoreV(x, i, p) ((p)[i] = (x))
#elif VECTOR_SIZE_I == 2
#define doubleV double2
#define convert_doubleV convert_double2
#define longV long2
#define indicesV ((longV)(0, 1))
#define vloadV vload2
#define vstoreV vstore2
#elif VECTOR_SIZE_I == 4
#define doubleV double4
#define convert_doubleV convert_double4
#define longV long4
#define indicesV ((longV)(0, 1, 2, 3))
#define vloadV vload4
#define vstoreV vstore4
#elif VECTOR_SIZE_I == 8
#define doubleV double8
#define convert_doubleV convert_double8
#define longV long8
#define indicesV ((longV)(0, 1, 2, 3, 4, 5, 6, 7))
#define vloadV vload8
#define vstoreV vstore8
#elif VECTOR_SIZE_I == 16
#define doubleV double16
#define convert_doubleV convert_double16
#define longV long16
#define indicesV ((longV)(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15))
#define vloadV vload16
#define vstoreV vstore16
#else
#error
#endif

#if VECTOR_SIZE_J != 1 || VECTOR_SIZE_K != 1
#error                                                                         \
    "Non-trivial vector sizes in the j and k directions are not yet supported"
#endif

#define CCTK_REAL double
#define CCTK_INT int
#define CCTK_LONG long

#define CCTK_REAL_VEC_SIZE VECTOR_SIZE_I
#define CCTK_REAL_VEC doubleV
#define CCTK_LONG_VEC longV
#define convert_real_vec convert_doubleV
#define vec_index convert_real_vec(indicesV)

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

#define vec_loada(p) (*(CCTK_REAL_VEC const __global *)&(p))
#define vec_loadu(p) vloadV(0, &(p))

#if VECTORISE_ALIGNED_ARRAYS
#define vec_load(p) vec_loada(p)
#define vec_loadu_maybe3(off1, off2, off3, p)                                  \
  ((off1) % CCTK_REAL_VEC_SIZE == 0 ? vec_loada(p) : vec_loadu(p))
#else
#define vec_load(p) vec_loadu(p)
#define vec_loadu_maybe3(off1, off2, off3, p) vec_loadu(p)
#endif

#define vec_storea(p, x) (*(CCTK_REAL_VEC __global *)&(p) = (x))
#define vec_storeu(p, x) vstoreV(x, 0, &(p))

#if VECTORISE_ALIGNED_ARRAYS
#define vec_store_nta(p, x) vec_storea(p, x)
#else
#define vec_store_nta(p, x) vec_storeu(p, x)
#endif

#define vec_store_partial_prepare(i, imin, imax)

#if CCTK_REAL_VEC_SIZE == 1

#define vec_store_nta_partial(p, x)                                            \
  do {                                                                         \
    if (CCTK_BUILTIN_EXPECT(lc_vec_any_I && lc_vec_any_J && lc_vec_any_K,      \
                            true)) {                                           \
      vec_store_nta(p, x);                                                     \
    }                                                                          \
  } while (0)

#elif CCTK_REAL_VEC_SIZE == 2

#define vec_store_nta_partial(p, x)                                            \
  do {                                                                         \
    if (CCTK_BUILTIN_EXPECT(lc_vec_any_I && lc_vec_any_J && lc_vec_any_K,      \
                            true)) {                                           \
      if (CCTK_BUILTIN_EXPECT(lc_vec_all_I, true)) {                           \
        vec_store_nta(p, x);                                                   \
      } else {                                                                 \
        if (lc_vec_lo_I) {                                                     \
          (&(p))[0] = (x).s0;                                                  \
        } else {                                                               \
          (&(p))[1] = (x).s1;                                                  \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  } while (0)

#else

#define vec_store_nta_partial(p, x)                                            \
  do {                                                                         \
    if (CCTK_BUILTIN_EXPECT(lc_vec_any_I && lc_vec_any_J && lc_vec_any_K,      \
                            true)) {                                           \
      if (CCTK_BUILTIN_EXPECT(lc_vec_all_I, true)) {                           \
        vec_store_nta(p, x);                                                   \
      } else {                                                                 \
        /* select(a,b,c) = MSB(c) ? b : a */                                   \
        vec_store_nta(p, select(vec_load(p), x, lc_vec_mask_I));               \
      }                                                                        \
    }                                                                          \
  } while (0)

#endif

#define kneg(x) (-(x))

#define kadd(x, y) ((x) + (y))
#define ksub(x, y) ((x) - (y))
#define kmul(x, y) ((x) * (y))
#define kdiv(x, y) ((x) / (y))

#define kmadd(x, y, z) mad(x, y, z) // faster than fma(x,y,z)
#define kmsub(x, y, z) mad(x, y, -(z))
#define knmadd(x, y, z) (-mad(x, y, z))
#define knmsub(x, y, z) (-mad(x, y, -(z)))

#define kcopysign(x, y) copysign(x, y)
#define kfabs(x) fabs(x)
#define kfmax(x, y) fmax(x, y)
#define kfmin(x, y) fmin(x, y)
#define kfnabs(x) (-fabs(x))
#define ksqrt(x) sqrt(x)

#define kacos(x) acos(x)
#define kacosh(x) acosh(x)
#define kasin(x) asin(x)
#define kasinh(x) asinh(x)
#define katan(x) atan(x)
#define katan2(x, y) atan2(x, y)
#define katanh(x) atanh(x)
#define kcos(x) cos(x)
#define kcosh(x) cosh(x)
#define kexp(x) exp(x)
#define klog(x) log(x)
#define kpow(x, a) pow(x, (CCTK_REAL)(a))
#define ksin(x) sin(x)
#define ksinh(x) sinh(x)
#define ktan(x) tan(x)
#define ktanh(x) tanh(x)

// Choice   [sign(x)>0 ? y : z]
// #define kifpos(x,y,z) select(y,z,x)
// #define kifneg(x,y,z) select(z,y,x)

// Choice   [x ? y : z]
#define kifthen(x, y, z)                                                       \
  select((CCTK_REAL_VEC)(z), (CCTK_REAL_VEC)(y), (CCTK_LONG_VEC)(x))

////////////////////////////////////////////////////////////////////////////////

#define dim 3

typedef struct {
  // Doubles first, then ints, to ensure proper alignment
  // Coordinates:
  double cctk_origin_space[dim];
  double cctk_delta_space[dim];
  double cctk_time;
  double cctk_delta_time;
  // Grid structure properties:
  int cctk_iteration;
  int cctk_gsh[dim];
  int cctk_lbnd[dim];
  int cctk_lsh[dim];
  int cctk_ash[dim];
  // Loop settings:
  int imin[dim]; // active region
  int imax[dim];
#if 0
  int lmin[dim];                 // loop region
  int lmax[dim];
#endif
} cGH;

ptrdiff_t round_down(ptrdiff_t const a, ptrdiff_t const b);
ptrdiff_t round_down(ptrdiff_t const a, ptrdiff_t const b) { return a / b * b; }

// Cactus compatibility definitions

#define DECLARE_CCTK_ARGUMENTS                                                 \
  CCTK_REAL __constant const *restrict const cctk_origin_space =               \
      cctkGH->cctk_origin_space;                                               \
  CCTK_REAL __constant const *restrict const cctk_delta_space =                \
      cctkGH->cctk_delta_space;                                                \
  CCTK_REAL const cctk_time = cctkGH->cctk_time;                               \
  CCTK_REAL const cctk_delta_time = cctkGH->cctk_delta_time;                   \
  ptrdiff_t const cctk_iteration = cctkGH->cctk_iteration;                     \
  ptrdiff_t const cctk_gsh[] = {cctkGH->cctk_gsh[0], cctkGH->cctk_gsh[1],      \
                                cctkGH->cctk_gsh[2]};                          \
  ptrdiff_t const cctk_lbnd[] = {cctkGH->cctk_lbnd[0], cctkGH->cctk_lbnd[1],   \
                                 cctkGH->cctk_lbnd[2]};                        \
  ptrdiff_t const cctk_lsh[] = {cctkGH->cctk_lsh[0], cctkGH->cctk_lsh[1],      \
                                cctkGH->cctk_lsh[2]};                          \
  ptrdiff_t const cctk_ash[] = {cctkGH->cctk_ash[0], cctkGH->cctk_ash[1],      \
                                cctkGH->cctk_ash[2]};                          \
  ptrdiff_t const imin[] = {cctkGH->imin[0], cctkGH->imin[1],                  \
                            cctkGH->imin[2]};                                  \
  ptrdiff_t const imax[] = {cctkGH->imax[0], cctkGH->imax[1],                  \
                            cctkGH->imax[2]};                                  \
  bool const stress_energy_state1 = 0;

#define CCTK_GFINDEX3D(cctkGH, i, j, k)                                        \
  ((i) + cctk_ash[0] * ((j) + cctk_ash[1] * (k)))

#define CCTK_ORIGIN_SPACE(d) (cctkGH->cctk_origin_space[d])
#define CCTK_DELTA_SPACE(d) (cctkGH->cctk_delta_space[d])
#define CCTK_DELTA_TIME (cctkGH->cctk_delta_time)

// Kranc compatibility definitions

#define Pi M_PI
#define IfThen(c, x, y) ((c) ? (x) : (y))
#define ToReal(x) ((CCTK_REAL_VEC)(CCTK_REAL)(x))

CCTK_REAL ScalarINV(CCTK_REAL const x);
CCTK_REAL ScalarINV(CCTK_REAL const x) { return ((CCTK_REAL)1.0) / x; }
CCTK_REAL ScalarSQR(CCTK_REAL const x);
CCTK_REAL ScalarSQR(CCTK_REAL const x) { return x * x; }
CCTK_REAL_VEC INV(CCTK_REAL_VEC const x);
CCTK_REAL_VEC INV(CCTK_REAL_VEC const x) { return ToReal(1.0) / x; }
// CCTK_REAL_VEC Sign(CCTK_REAL_VEC const x);
// CCTK_REAL_VEC Sign(CCTK_REAL_VEC const x)
// {
//   return x==ToReal(0) ? ToReal(0) : copysign(ToReal(1), x);
// }
CCTK_REAL_VEC SQR(CCTK_REAL_VEC const x);
CCTK_REAL_VEC SQR(CCTK_REAL_VEC const x) { return x * x; }
CCTK_REAL_VEC ksgn(CCTK_REAL_VEC x);
CCTK_REAL_VEC ksgn(CCTK_REAL_VEC x) {
  return kifthen(x == ToReal(0.0), ToReal(0.0), kcopysign(ToReal(1.0), x));
}
#if 0
CCTK_LONG_VEC kisgn(CCTK_REAL_VEC x);
CCTK_LONG_VEC kisgn(CCTK_REAL_VEC x)
{
  return select(select((CCTK_LONG_VEC)+1,
                       (CCTK_LONG_VEC)-1, (CCTK_LONG_VEC)(x<ToReal(0.0))),
                (CCTK_LONG_VEC)0, (CCTK_LONG_VEC)(x==ToReal(0.0)));
}
#else
// Kranc uses a scalar variable to hold the return value of kisgn;
// therefore we just provide a dummy implementation, assuming it is
// unused
#define kisgn(x) 0
#endif

#define KRANC_GFOFFSET3D(u, i, j, k)                                           \
  vec_loadu_maybe3(i, j, k, (u)[di * (i) + dj * (j) + dk * (k)])

#define eTtt ((CCTK_REAL __global const *)0)
#define eTtx ((CCTK_REAL __global const *)0)
#define eTty ((CCTK_REAL __global const *)0)
#define eTtz ((CCTK_REAL __global const *)0)
#define eTxx ((CCTK_REAL __global const *)0)
#define eTxy ((CCTK_REAL __global const *)0)
#define eTxz ((CCTK_REAL __global const *)0)
#define eTyy ((CCTK_REAL __global const *)0)
#define eTyz ((CCTK_REAL __global const *)0)
#define eTzz ((CCTK_REAL __global const *)0)
#define jacobian_derivative_group ""
#define jacobian_group ""
#define jacobian_identity_map 0
#define stress_energy_state (&stress_energy_state1)
#define CCTK_IsFunctionAliased(x) 0
#define CCTK_WARN(lev, msg) ((void)0)
#define GenericFD_GroupDataPointers(a, b, c, d) ((void)0)
#define MultiPatch_GetMap(x) 0
#define strlen(x) 0

////////////////////////////////////////////////////////////////////////////////

// Prevent compiler crashes when get_local_id() is not working
size_t my_get_local_id(uint dir);
size_t my_get_local_id(uint dir) {
  switch (dir) {
  case 0:
#if GROUP_SIZE_I == 1
    return 0;
#else
    return get_local_id(0);
#endif
  case 1:
#if GROUP_SIZE_J == 1
    return 0;
#else
    return get_local_id(1);
#endif
  case 2:
#if GROUP_SIZE_K == 1
    return 0;
#else
    return get_local_id(2);
#endif
  }
  return 0;
}

#define LC_SET_GROUP_VARS(D)                                                   \
  ptrdiff_t const ind##D CCTK_ATTRIBUTE_UNUSED =                               \
      (lc_off##D +                                                             \
       VECTOR_SIZE_##D * UNROLL_SIZE_##D *                                     \
           (lc_grp##D +                                                        \
            GROUP_SIZE_##D * (lc_til##D + TILE_SIZE_##D * lc_grd##D)));        \
  bool const lc_grp_done_##D CCTK_ATTRIBUTE_UNUSED = ind##D >= lc_##D##max;

#define vecVI indicesV
#define vecVJ ((CCTK_LONG_VEC)0)
#define vecVK ((CCTK_LONG_VEC)0)

#define LC_SET_VECTOR_VARS(IND, D)                                             \
  ptrdiff_t const IND CCTK_ATTRIBUTE_UNUSED =                                  \
      (lc_off##D +                                                             \
       VECTOR_SIZE_##D *                                                       \
           (lc_unr##D +                                                        \
            UNROLL_SIZE_##D *                                                  \
                (lc_grp##D +                                                   \
                 GROUP_SIZE_##D * (lc_til##D + TILE_SIZE_##D * lc_grd##D))));  \
  bool const lc_vec_trivial_##D CCTK_ATTRIBUTE_UNUSED =                        \
      VECTOR_SIZE_##D * UNROLL_SIZE_##D == 1;                                  \
  bool const lc_vec_any_##D                                                    \
      CCTK_ATTRIBUTE_UNUSED = /*TODO because unroll size is 1*/                \
      1 /*TODO lc_vec_trivial_##D ||                                      \
        (IND+VECTOR_SIZE_##D-1 >= lc_##D##min && IND < lc_##D##max)*/;   \
  bool const lc_vec_lo_##D CCTK_ATTRIBUTE_UNUSED =                             \
      lc_vec_trivial_##D || IND >= lc_##D##min;                                \
  bool const lc_vec_hi_##D CCTK_ATTRIBUTE_UNUSED =                             \
      lc_vec_trivial_##D || IND + VECTOR_SIZE_##D - 1 < lc_##D##max;           \
  bool const lc_vec_all_##D CCTK_ATTRIBUTE_UNUSED =                            \
      lc_vec_trivial_##D || (lc_vec_lo_##D && lc_vec_hi_##D);                  \
  CCTK_LONG_VEC const lc_vec_mask_##D CCTK_ATTRIBUTE_UNUSED =                  \
      lc_vec_trivial_##D                                                       \
          ? (CCTK_LONG_VEC) true                                               \
          : ((CCTK_LONG_VEC)IND + vecV##D >= (CCTK_LONG_VEC)lc_##D##min) &     \
                ((CCTK_LONG_VEC)IND + vecV##D < (CCTK_LONG_VEC)lc_##D##max);

#define LC_LOOP3VEC(name, i, j, k, imin, jmin, kmin, imax, jmax, kmax, ilsh,        \
                    jlsh, klsh, vecsize)                                            \
  do {                                                                              \
    typedef int lc_loop3_##name;                                                    \
                                                                                    \
    ptrdiff_t const lc_Imin = (imin);                                               \
    ptrdiff_t const lc_Jmin = (jmin);                                               \
    ptrdiff_t const lc_Kmin = (kmin);                                               \
    ptrdiff_t const lc_Imax = (imax);                                               \
    ptrdiff_t const lc_Jmax = (jmax);                                               \
    ptrdiff_t const lc_Kmax = (kmax);                                               \
    ptrdiff_t const lc_offI =                                                       \
        round_down(lc_Imin, VECTOR_SIZE_I * UNROLL_SIZE_I); /* offset */            \
    ptrdiff_t const lc_offJ =                                                       \
        round_down(lc_Jmin, VECTOR_SIZE_J * UNROLL_SIZE_J);                         \
    ptrdiff_t const lc_offK =                                                       \
        round_down(lc_Kmin, VECTOR_SIZE_K * UNROLL_SIZE_K);                         \
    ptrdiff_t const lc_grpI = my_get_local_id(0); /* index in group */              \
    ptrdiff_t const lc_grpJ = my_get_local_id(1);                                   \
    ptrdiff_t const lc_grpK = my_get_local_id(2);                                   \
    ptrdiff_t const lc_grdI = get_group_id(0); /* index in grid */                  \
    ptrdiff_t const lc_grdJ = get_group_id(1);                                      \
    ptrdiff_t const lc_grdK = get_group_id(2);                                      \
                                                                                    \
    ptrdiff_t const lc_imin = lc_Imin;                                              \
    ptrdiff_t const lc_imax = lc_Imax;                                              \
                                                                                    \
    for (ptrdiff_t lc_tilK = 0; lc_tilK < TILE_SIZE_K; ++lc_tilK) {                 \
      LC_SET_GROUP_VARS(K);                                                         \
      if (CCTK_BUILTIN_EXPECT(lc_grp_done_K, 0))                                    \
        break;                                                                      \
      for (ptrdiff_t lc_tilJ = 0; lc_tilJ < TILE_SIZE_J; ++lc_tilJ) {               \
        LC_SET_GROUP_VARS(J);                                                       \
        if (CCTK_BUILTIN_EXPECT(lc_grp_done_J, 0))                                  \
          break;                                                                    \
        for (ptrdiff_t lc_tilI = 0; lc_tilI < TILE_SIZE_I; ++lc_tilI) {             \
          LC_SET_GROUP_VARS(I);                                                     \
          if (CCTK_BUILTIN_EXPECT(lc_grp_done_I, 0))                                \
            break;                                                                  \
                                                                                    \
          ptrdiff_t const lc_unrK = 0;                                              \
          /*TODO CCTK_UNROLL                                                \
        for (ptrdiff_t lc_unrK = 0; lc_unrK < UNROLL_SIZE_K; ++lc_unrK)*/ {     \
            LC_SET_VECTOR_VARS(k, K);                                               \
            ptrdiff_t const lc_unrJ = 0;                                            \
            /*TODO CCTK_UNROLL                                                \
        for (ptrdiff_t lc_unrJ = 0; lc_unrJ < UNROLL_SIZE_J; ++lc_unrJ)*/ {   \
              LC_SET_VECTOR_VARS(j, J);                                             \
              ptrdiff_t const lc_unrI = 0;                                          \
              /*TODO CCTK_UNROLL                                                \
        for (ptrdiff_t lc_unrI = 0; lc_unrI < UNROLL_SIZE_I; ++lc_unrI)*/ { \
                LC_SET_VECTOR_VARS(i, I);                                           \
                                                                                    \
                {
#define LC_ENDLOOP3VEC(name)                                                   \
  }                                                                            \
  }                                                                            \
  }                                                                            \
  }                                                                            \
  }                                                                            \
  }                                                                            \
  }                                                                            \
  typedef lc_loop3_##name lc_ensure_proper_nesting;                            \
  }                                                                            \
  while (0)

#define LC_LOOP3(name, i, j, k, imin, jmin, kmin, imax, jmax, kmax, ilsh,      \
                 jlsh, klsh)                                                   \
  LC_LOOP3VEC(name, i, j, k, imin, jmin, kmin, imax, jmax, kmax, ilsh, jlsh,   \
              klsh, 1)
#define LC_ENDLOOP3(name) LC_ENDLOOP3VEC(name)
