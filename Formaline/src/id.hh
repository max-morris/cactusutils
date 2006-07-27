#ifndef FORMALINE_ID_HH
#define FORMALINE_ID_HH

#include "cctk.h"

namespace Formaline
{
  
  // Get a unique build id
  char const *
  get_build_id (cGH const * const cctkGH);
  
  // Get a unique job id
  char const *
  get_job_id (cGH const * const cctkGH);
  
} // namespace Formaline

#endif // #ifndef FORMALINE_ID_HH
