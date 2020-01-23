#ifndef _VECTORS_TESTS_H_
#define _VECTORS_TESTS_H_
#include <cctk.h>

namespace Vectors {
void Test_4_AVX(CCTK_ARGUMENTS);
void Test_4_Altivec(CCTK_ARGUMENTS);
void Test_4_SSE(CCTK_ARGUMENTS);
void Test_4_default(CCTK_ARGUMENTS);
void Test_8_AVX(CCTK_ARGUMENTS);
void Test_8_AVX512(CCTK_ARGUMENTS);
void Test_8_DoubleHummer(CCTK_ARGUMENTS);
void Test_8_MIC(CCTK_ARGUMENTS);
void Test_8_QPX(CCTK_ARGUMENTS);
void Test_8_SSE2(CCTK_ARGUMENTS);
void Test_8_VSX(CCTK_ARGUMENTS);
void Test_8_default(CCTK_ARGUMENTS);
}

#endif // _VECTORS_TESTS_H_
