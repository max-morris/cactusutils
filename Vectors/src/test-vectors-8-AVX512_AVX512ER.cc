#undef __AVX__
//#undef __knl__
#undef __MIC__
//#undef __AVX512F__
//#undef __AVX512ER__
#undef __SSE2__
#undef __SSE__
#undef __ALTIVEC__
#undef _ARCH_PWR7

#define VECTOR_REAL_PRECISION 8

// includes cctk.h of correct type
#include "test.hcc"

#include "test-vectors.h"

namespace Vectors {

bool Test_8_AVX512_AVX512ER(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

#if (defined __knl__ || defined __AVX512F__) && defined __AVX512ER__ && \
    !defined DISABLE_AVX512 // Intel AVX512
  return Test(CCTK_PASS_CTOC, vec_architecture);
#else
  return true;
#endif
}

}
