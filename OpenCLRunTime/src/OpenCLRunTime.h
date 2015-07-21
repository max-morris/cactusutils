#ifndef OPENCLRUNTIME_H
#define OPENCLRUNTIME_H

#include <cctk.h>

#ifdef __cplusplus

namespace OpenCLRunTime {

extern "C" {

#endif

// Opaque object describing an OpenCL kernel
struct OpenCLKernel;

// Call a kernel function that is given as source code string. This
// is efficient, i.e. the compiled kernel is cached. *pkernel must
// be a NULL pointer in the first call, and is then allocated by
// this routine.
void OpenCLRunTime_CallKernel(cGH const *const cctkGH,
                              // thorn name
                              char const *const thorn,
                              // kernel name
                              char const *const name,
                              // kernel source
                              char const *const sources[],
                              // list of accessed grid functions
                              char const *const groups[],
                              int const varindices[], int const timelevels[],
                              char const *const aliases[], int const nvars,
                              // looping region
                              int const imin[], int const imax[],
                              // data structure describing kernel
                              struct OpenCLKernel **const pkernel);

#ifdef __cplusplus

} // extern "C"

} // namespace OpenCLRunTime

using namespace OpenCLRunTime;

#endif

#endif // #ifndef OPENCLRUNTIME_H
