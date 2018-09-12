#include "cctk.h"
#include "cctk_Parameters.h"
#include "cctk_Arguments.h"

void TerminationTrigger_TriggerTermination(CCTK_ARGUMENTS)
{
  DECLARE_CCTK_PARAMETERS;
  DECLARE_CCTK_ARGUMENTS;

  *triggered = 1;

  CCTK_TerminateNext(cctkGH);
}

void TerminationTrigger_ResetTrigger(CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;

  *triggered = 0;
}
