/* $Header$ */

#include "cctk.h"

#if defined(__linux__) && defined(__i386__) && (defined(__GNUC__) || defined(__INTEL_COMPILER))

#include <fpu_control.h>

void catch_nans (void)
{
  fpu_control_t cw;
  
  _FPU_GETCW(cw);
  /* create interrupts for invalid operations, zero divide, and overflow */
  cw &= ~(_FPU_MASK_IM | _FPU_MASK_ZM | _FPU_MASK_OM);
  _FPU_SETCW(cw);
  
  CCTK_INFO ("NaNCatcher enabled");
}

void no_catch_nans (void)
{
  fpu_control_t cw;
  
  _FPU_GETCW(cw);
  /* don't create interrupts for invalid operations, zero divide, and
     overflow */
  cw |= (_FPU_MASK_IM | _FPU_MASK_ZM | _FPU_MASK_OM);
  _FPU_SETCW(cw);
  
  CCTK_INFO ("NaNCatcher disabled");
}

#else

void catch_nans (void)
{
  CCTK_WARN (1, "NaNCatcher disabled -- no support for your compiler");
}

void no_catch_nans (void)
{
  CCTK_WARN (1, "NaNCatcher disabled -- no support for your compiler");
}

#endif
