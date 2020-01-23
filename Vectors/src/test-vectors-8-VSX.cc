// fudge CCTK_REAL_PRECISION to 4 or 8 depending on the vector type we want to
// test
#define _CCTK_TYPES_H_
#include "cctk_Config.h"
#undef CCTK_REAL_PRECISION_16
#undef CCTK_REAL_PRECISION_8
#undef CCTK_REAL_PRECISION_4
#define CCTK_REAL_PRECISION_8 1
#undef _CCTK_TYPES_H_
#include "cctk_Types.h"

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

bool Test_8_VSX(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

#if defined __ALTIVEC__ && defined _ARCH_PWR7 // Power VSX
  return Test(CCTK_PASS_CTOC, "8-VSX");
#else
  return true;
#endif
}

}
