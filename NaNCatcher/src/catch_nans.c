/* $Header$ */

#include "cctk.h"

#if defined(__linux__) && defined(__i386__) && (defined(__GNUC__) || defined(__INTEL_COMPILER))
/* #if defined(__linux__) && defined(__i386__) && (defined(__GNUC__)) */

#include <fpu_control.h>

int catch_nans (void)
{
  fpu_control_t cw;
  
  _FPU_GETCW(cw);
  /* create interrupts for invalid operations, zero divide, and overflow */
  cw &= ~(_FPU_MASK_IM | _FPU_MASK_ZM | _FPU_MASK_OM);
  _FPU_SETCW(cw);
  
  CCTK_INFO ("NaNCatcher enabled");
  
  return 0;
}

#else

int catch_nans (void)
{
  CCTK_WARN (1, "NaNCatcher disabled -- no support for your compiler");
  
  return 0;
}

#endif
