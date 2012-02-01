#ifndef ACCELERATOR_HH
#define ACCELERATOR_HH

#include <cctk.h>
#include <vector>

namespace Accelerator {
  
  using namespace std;
  
  struct mem_t {
    bool host_valid, device_valid;
  };
  
  struct vars_t {
    vector<CCTK_INT> vis, tls;
    vars_t() { vis.reserve(CCTK_NumVars()); tls.reserve(CCTK_NumVars()); }
    void push_back(int vi, int tl) { vis.push_back(vi); tls.push_back(tl); }
    CCTK_INT const *vi_ptr() const { return &vis[0]; }
    CCTK_INT const *tl_ptr() const { return &tls[0]; }
    int nvars() const { return vis.size(); }
  };
  
  struct device_t {
    vector<vector<mem_t> > mems; // [vi][tl]
    device_t(): mems(CCTK_NumVars()) {}
  };
  
  extern device_t *device;
  
} // namespace Accelerator

#endif  // #ifndef ACCELERATOR_HH
