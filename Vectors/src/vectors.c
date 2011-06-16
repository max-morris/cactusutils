
#include "cctk.h" 
#include "cctk_Arguments.h" 
#include "cctk_Parameters.h" 
#include "vectors.h"

int Vectors_Startup(void)
{
  CCTK_VInfo(CCTK_THORNSTRING, "Using vector size %d", CCTK_REAL_VEC_SIZE);
  return 0;
}
