/* C99 provides standard functions to deal with floating point
 * exceptions. Alas, they fail to provide the features we want. If you
 * manage to figure out a portable way to enable individual traps,
 * tell us about it. For now, we only support glibc-based and x86
 * systems. [dk]
 */

#define _GNU_SOURCE

#include <cctk.h>

/* Fake a GNU extension if it does not exist */
#ifndef __GLIBC_PREREQ
#define __GLIBC_PREREQ(a, b) 0
#endif

/*
 * Prototypes of the routines that are defined below.
 */
int catch_nans(void);
int no_catch_nans(void);

#if __GLIBC__ >= 2 && __GLIBC_PREREQ(2, 2)

/*
 * Glibc 2.2 provides a simple and easy to use way to deal with
 * individual exceptions as a GNU extension. If we are on a GNU
 * system, this is the preferred way to proceed.
 */

#include <fenv.h>

int catch_nans(void) {
  if (-1 != feenableexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_INVALID)) {
    CCTK_INFO("NaNCatcher enabled");
    return 0;
  }

  CCTK_WARN(1, "NaNCatcher disabled -- failed to enable traps");
  return 0;
}

int no_catch_nans(void) {
  if (-1 != fedisableexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_INVALID)) {
    CCTK_INFO("NaNCatcher disabled");
    return 0;
  }

  CCTK_WARN(1, "NaNCatcher -- failed to disable traps");
  return 0;
}

#elif(defined __linux__ && (defined __i386__ || defined __x86_64__) &&         \
      (defined __GNUC__ || defined __INTEL_COMPILER))

/*
 * Here's an x86-only fallback for non-glibc systems that use the GNU
 * or Intel compilers.
 */

#include <fpu_control.h>

int catch_nans(void) {
  fpu_control_t cw;

  _FPU_GETCW(cw);
  /* create interrupts for invalid operations, zero divide, and overflow */
  cw &= ~(_FPU_MASK_IM | _FPU_MASK_ZM | _FPU_MASK_OM);
  _FPU_SETCW(cw);

  CCTK_INFO("NaNCatcher enabled");

  return 0;
}

int no_catch_nans(void) {
  fpu_control_t cw;

  _FPU_GETCW(cw);
  /* don't create interrupts for invalid operations, zero divide, and
     overflow */
  cw |= (_FPU_MASK_IM | _FPU_MASK_ZM | _FPU_MASK_OM);
  _FPU_SETCW(cw);

  CCTK_INFO("NaNCatcher disabled");

  return 0;
}

#elif defined(__ppc__) || defined(__ppc64__)

/*
 * PowerPC architecture.  Use code taken from the GNU libc, version
 * 2.3.5, subdirectory sysdeps/powerpc/fpu.  Long live the GPL.
 */

typedef double fenv_t;

typedef union {
  fenv_t fenv;
  unsigned int l[2];
} fenv_union_t;

/* Definitions of all the FPSCR bit numbers */
enum {
  FPSCR_FX = 0,  /* exception summary */
  FPSCR_FEX,     /* enabled exception summary */
  FPSCR_VX,      /* invalid operation summary */
  FPSCR_OX,      /* overflow */
  FPSCR_UX,      /* underflow */
  FPSCR_ZX,      /* zero divide */
  FPSCR_XX,      /* inexact */
  FPSCR_VXSNAN,  /* invalid operation for SNaN */
  FPSCR_VXISI,   /* invalid operation for Inf-Inf */
  FPSCR_VXIDI,   /* invalid operation for Inf/Inf */
  FPSCR_VXZDZ,   /* invalid operation for 0/0 */
  FPSCR_VXIMZ,   /* invalid operation for Inf*0 */
  FPSCR_VXVC,    /* invalid operation for invalid compare */
  FPSCR_FR,      /* fraction rounded [fraction was incremented by round] */
  FPSCR_FI,      /* fraction inexact */
  FPSCR_FPRF_C,  /* result class descriptor */
  FPSCR_FPRF_FL, /* result less than (usually, less than 0) */
  FPSCR_FPRF_FG, /* result greater than */
  FPSCR_FPRF_FE, /* result equal to */
  FPSCR_FPRF_FU, /* result unordered */
  FPSCR_20,      /* reserved */
  FPSCR_VXSOFT,  /* invalid operation set by software */
  FPSCR_VXSQRT,  /* invalid operation for square root */
  FPSCR_VXCVI,   /* invalid operation for invalid integer convert */
  FPSCR_VE,      /* invalid operation exception enable */
  FPSCR_OE,      /* overflow exception enable */
  FPSCR_UE,      /* underflow exception enable */
  FPSCR_ZE,      /* zero divide exception enable */
  FPSCR_XE,      /* inexact exception enable */
  FPSCR_NI       /* non-IEEE mode (typically, no denormalised numbers) */
  /* the remaining two least-significant bits keep the rounding mode */
};

/* Equivalent to fegetenv, but returns a fenv_t instead of taking a
   pointer.  */
#define fegetenv_register()                                                    \
  ({                                                                           \
    fenv_t env;                                                                \
    asm volatile("mffs %0" : "=f"(env));                                       \
    env;                                                                       \
  })

/* Equivalent to fesetenv, but takes a fenv_t instead of a pointer.  */
#define fesetenv_register(env)                                                 \
  ({                                                                           \
    double d = (env);                                                          \
    asm volatile("mtfsf 0xff,%0" : : "f"(d));                                  \
  })

int catch_nans(void) {
  fenv_union_t fe;

  fe.fenv = fegetenv_register();
  fe.l[1] |= (1 << (31 - FPSCR_ZE));
  fe.l[1] |= (1 << (31 - FPSCR_OE));
  fe.l[1] |= (1 << (31 - FPSCR_VE));
  fesetenv_register(fe.fenv);

  CCTK_INFO("NaNCatcher enabled");

  return 0;
}

int no_catch_nans(void) {
  fenv_union_t fe;

  fe.fenv = fegetenv_register();
  fe.l[1] &= ~(1 << (31 - FPSCR_ZE));
  fe.l[1] &= ~(1 << (31 - FPSCR_OE));
  fe.l[1] &= ~(1 << (31 - FPSCR_VE));
  fesetenv_register(fe.fenv);

  CCTK_INFO("NaNCatcher disabled");

  return 0;
}

#else

/*
 * Out of options now. Do nothing.
 */

int catch_nans(void) {
  CCTK_WARN(CCTK_WARN_ALERT,
            "NaNCatcher disabled -- no support for your compiler");

  return 0;
}

int no_catch_nans(void) {
  CCTK_WARN(CCTK_WARN_ALERT,
            "NaNCatcher disabled -- no support for your compiler");

  return 0;
}

#endif
