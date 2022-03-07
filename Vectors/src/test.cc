#include <cctk.h>
#include <cctk_Arguments.h>
#include <cctk_Parameters.h>

#ifndef DECLARE_CCTK_ARGUMENTS_CHECKED
#define DECLARE_CCTK_ARGUMENTS_CHECKED(fun) DECLARE_CCTK_ARGUMENTS
#endif

#include "test-vectors.h"

#define VECTOR_REAL_PRECISION CCTK_REAL_PRECISION
#include "test.hcc"

namespace Vectors {
extern "C" void Vectors_Test(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS_CHECKED(Vectors_Test);
  DECLARE_CCTK_PARAMETERS;

  bool passed = true;
  
  passed = passed && Test(CCTK_PASS_CTOC, "");

  if(test_all) {
#if VECTORISE
    // the Test_N_XXX functions are no-ops if the engine is not avaiable
    passed = passed && Test_4_AVX(CCTK_PASS_CTOC);
    // there's technically AVX+FMA4 and AVX+AVX2+FMA4 but I am only going to
    // test for FMA4 once since they do not form a tensor product in the source
    // file
    passed = passed && Test_4_AVX_AVX2(CCTK_PASS_CTOC);
    passed = passed && Test_4_AVX_FMA4(CCTK_PASS_CTOC);
    passed = passed && Test_4_SSE(CCTK_PASS_CTOC);
    // there's technically SSE+FMA4 and SSE+SSE41 etc but I am only going to
    // test for them once since they do not form a tensor product in the source
    // file
    passed = passed && Test_4_SSE_SSE41(CCTK_PASS_CTOC);
    passed = passed && Test_4_SSE_SSE4A(CCTK_PASS_CTOC);
    passed = passed && Test_4_SSE_FMA4(CCTK_PASS_CTOC);
    passed = passed && Test_4_Altivec(CCTK_PASS_CTOC);
    passed = passed && Test_8_AVX512(CCTK_PASS_CTOC);
    passed = passed && Test_8_AVX512_AVX512ER(CCTK_PASS_CTOC);
    passed = passed && Test_8_MIC(CCTK_PASS_CTOC);
    passed = passed && Test_8_AVX(CCTK_PASS_CTOC);
    // there's technically AVX+FMA4 and AVX+AVX2+FMA4 but I am only going to
    // test for FMA4 once since they do not form a tensor product in the source
    // file
    passed = passed && Test_8_AVX_AVX2(CCTK_PASS_CTOC);
    passed = passed && Test_8_AVX_FMA4(CCTK_PASS_CTOC);
    passed = passed && Test_8_SSE2(CCTK_PASS_CTOC);
    // there's technically SSE2+FMA4 and SSE2+SSE41 etc but I am only going to
    // test for them once since they do not form a tensor product in the source
    // file
    passed = passed && Test_8_SSE2_SSE41(CCTK_PASS_CTOC);
    passed = passed && Test_8_SSE2_SSE4A(CCTK_PASS_CTOC);
    passed = passed && Test_8_SSE2_FMA4(CCTK_PASS_CTOC);
    passed = passed && Test_8_VSX(CCTK_PASS_CTOC);

    // Default implementation, do not vectorise
    passed = passed && Test_4_default(CCTK_PASS_CTOC);
    passed = passed && Test_8_default(CCTK_PASS_CTOC);
#endif
  }

  *all_passed = CCTK_INT(passed);
}

} // namespace Vectors

