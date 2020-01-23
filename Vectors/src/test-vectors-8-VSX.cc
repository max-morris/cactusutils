#include <cctk.h>
#include <cctk_Arguments.h>
#include <cctk_Parameters.h>

#include "test-vectors.h"

#undef __AVX__
#undef __knl__
#undef __MIC__
#undef __AVX512F__
#undef __AVX512ER__
#undef __SSE2__
#undef __SSE__
#undef __bgq__
#undef __VECTOR4DOUBLE__
//#undef __ALTIVEC__
//#undef _ARCH_PWR7
#undef _ARCH_450D

#include "test.hcc"

namespace Vectors {

void Test_8_VSX(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

#if defined __ALTIVEC__ && defined _ARCH_PWR7 // Power VSX
  Test(CCTK_PASS_CTOC, "8-VSX");
#endif
}

}
