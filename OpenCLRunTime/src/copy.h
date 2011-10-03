#ifndef COPY_H
#define COPY_H

#include "defs.h"
#include <vector>

namespace OpenCLRunTime {
  
  struct var_t {
    int vi, tl;
  };
  
  void copy_to_device(cGH const *restrict const cctkGH,
                      vector<var_t> const& vars);
  void copy_to_host(cGH const *restrict const cctkGH,
                    vector<var_t> const& vars);
  void copy_presync(cGH const *restrict const cctkGH,
                    vector<var_t> const& vars);
  void copy_postsync(cGH const *restrict const cctkGH,
                     vector<var_t> const& vars);
  void copy_cycle(cGH const *restrict const cctkGH,
                  vector<var_t> const& vars);
  void copy_mol(cGH const *restrict const cctkGH,
                vector<var_t> const& vars);
  
} // namespace OpenCLRunTime

#endif  // #ifndef COPY_H
