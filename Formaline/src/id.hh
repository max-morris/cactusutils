#ifndef FORMALINE_ID_HH
#define FORMALINE_ID_HH

#include "cctk.h"

namespace Formaline {

// Get the configuration id
char const *get_config_id(cGH const *const cctkGH);

// Get the unique build id
char const *get_build_id(cGH const *const cctkGH);

// Get a unique simulation id
char const *get_simulation_id(cGH const *const cctkGH);

// Get a unique run id
char const *get_run_id(cGH const *const cctkGH);

} // namespace Formaline

#endif // #ifndef FORMALINE_ID_HH
