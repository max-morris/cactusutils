#ifndef COPY_H
#define COPY_H

// Copying data between host and device

#include "defs.hh"
#include <vector>

namespace OpenCLRunTime {

using namespace std;

struct vars_t {
  vector<CCTK_INT> vis, tls;
  vars_t() {
    vis.reserve(CCTK_NumVars());
    tls.reserve(CCTK_NumVars());
  }
  void push_back(int vi, int tl) {
    vis.push_back(vi);
    tls.push_back(tl);
  }
  CCTK_INT const *vi_ptr() const { return &vis[0]; }
  CCTK_INT const *tl_ptr() const { return &tls[0]; }
  int nvars() const { return vis.size(); }
};

} // namespace OpenCLRunTime

#endif // #ifndef COPY_H
