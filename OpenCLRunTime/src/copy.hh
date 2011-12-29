#ifndef COPY_H
#define COPY_H

// Copying data between host and device

#include "defs.hh"
#include <vector>

namespace OpenCLRunTime {
  
  // This identifies a variable (by index) with a certain time level
  struct var_t {
    int vi, tl;
  };
  
  // Copy data from the host to the device
  void copy_to_device(cGH const *restrict const cctkGH,
                      vector<var_t> const& vars);
  // Copy data from the device back to the host
  void copy_to_host(cGH const *restrict const cctkGH,
                    vector<var_t> const& vars);
  
  // Copy those data from the device back to the host that will be
  // needed for synchronization (i.e. inter-process synchronization;
  // AMR is not yet supported)
  void copy_presync(cGH const *restrict const cctkGH,
                    vector<var_t> const& vars);
  // Copy ghost zones from the host to the device
  void copy_postsync(cGH const *restrict const cctkGH,
                     vector<var_t> const& vars);
  
  // Cycle timelevels by copying
  void copy_cycle(cGH const *restrict const cctkGH,
                  vector<var_t> const& vars);
  // Copy from past to current timelevel (used by MoL)
  void copy_from_past(cGH const *restrict const cctkGH,
                      vector<var_t> const& vars);
  
} // namespace OpenCLRunTime

#endif  // #ifndef COPY_H
