/* $Header$ */

#include "cctk.h"

/* XXX C99 provides standard functions to deal with floating point exceptions.
 * Alas, they fail to provide the features we want. If you manage to figure out
 * a portable way to enable individual traps , tell us about it. For now, we
 * only support glibc-based and x86 systems. [dk]
 */

# define _GNU_SOURCE
#include <features.h>

/*
 * Glibc 2.2 provides a simple and easy to use way to deal with individual
 * exceptions as a GNU extension. If we are on a GNU system, this is the
 * preferred way to proceed.
 */

#if __GLIBC__ >= 2 && __GLIBC_PREREQ(2,2)

# include <fenv.h>

int catch_nans (void)
{
	if (-1 != feenableexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_INVALID)) {
		CCTK_INFO("NaNCatcher enabled");
		return 0;
	}

	CCTK_WARN (1, "NaNCatcher disabled -- failed to enable traps");
	return 0;
}

int no_catch_nans (void)
{
	if (-1 != fedisableexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_INVALID)) {
		CCTK_INFO ("NaNCatcher disabled");
		return 0;
	}

	CCTK_WARN (1, "NaNCatcher -- failed to disable traps");
	return 0;
}

/*
 * Here's an x86-only fallback for non-glibc systems that use the GNU or
 * Intel compilers.
 */

#elif defined(__linux__) && defined(__i386__) && (defined(__GNUC__) || defined(__INTEL_COMPILER))

# include <fpu_control.h>

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

int no_catch_nans (void)
{
  fpu_control_t cw;
  
  _FPU_GETCW(cw);
  /* don't create interrupts for invalid operations, zero divide, and
     overflow */
  cw |= (_FPU_MASK_IM | _FPU_MASK_ZM | _FPU_MASK_OM);
  _FPU_SETCW(cw);
  
  CCTK_INFO ("NaNCatcher disabled");
  
  return 0;
}

/*
 * Out of options now. Do nothing.
 */

#else

int catch_nans (void)
{
  CCTK_WARN (1, "NaNCatcher disabled -- no support for your compiler");
  
  return 0;
}

int no_catch_nans (void)
{
  CCTK_WARN (1, "NaNCatcher disabled -- no support for your compiler");
  
  return 0;
}

#endif
