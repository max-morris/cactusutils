#include <cctk.h>
#include <cctk_Arguments.h>
#include <cctk_Parameters.h>

#include "test-vectors.h"

#include "test.hcc"

namespace Vectors {
extern "C" void Vectors_Test(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  Test(CCTK_PASS_CTOC, "");

  if(test_all) {
    // the Test_N_XXX functions are no-ops if the engine is not avaiable
    Test_4_AVX(CCTK_PASS_CTOC);
    Test_4_SSE(CCTK_PASS_CTOC);
    Test_4_Altivec(CCTK_PASS_CTOC);
    Test_8_AVX512(CCTK_PASS_CTOC);
    Test_8_MIC(CCTK_PASS_CTOC);
    Test_8_AVX(CCTK_PASS_CTOC);
    Test_8_SSE2(CCTK_PASS_CTOC);
    Test_8_QPX(CCTK_PASS_CTOC);
    Test_8_VSX(CCTK_PASS_CTOC);
    Test_8_DoubleHummer(CCTK_PASS_CTOC);

// Default implementation, do not vectorise
    Test_4_default(CCTK_PASS_CTOC);
    Test_8_default(CCTK_PASS_CTOC);

  }
}

} // namespace Vectors

