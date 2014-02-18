#ifndef ACCELERATOR_HH
#define ACCELERATOR_HH

#include <cctk.h>
#include <cassert>
#include <vector>



namespace Accelerator {
  
  using namespace std;
  
  
  
  class id_t {
  public:
    struct ls_t { int vi, rl, tl; };
  private:
    vector<vector<vector<int> > > ids;
    vector<ls_t> lss;
  public:
    bool id_exists(int vi, int rl, int tl) const
    {
      if (int(ids.size()) <= vi) return false;
      if (int(ids.at(vi).size()) <= rl)  return false;
      if (int(ids.at(vi).at(rl).size()) <= tl) return false;
      return true;
    }
    int get_existing_id(int vi, int rl, int tl) const
    {
      return ids.at(vi).at(rl).at(tl);
    }
    int get_id(int vi, int rl, int tl)
    {
      if (int(ids.size()) <= vi) {
        ids.resize(CCTK_NumVars());
      }
      if (int(ids.at(vi).size()) <= rl) {
        ids.at(vi).resize(GetRefinementLevels(NULL));
      }
      if (int(ids.at(vi).at(rl).size()) <= tl) {
        ids.at(vi).at(rl).reserve(tl+1);
      }
      while (int(ids.at(vi).at(rl).size()) <= tl) {
        ids.at(vi).at(rl).push_back(lss.size());
        ls_t const ls = { vi, rl, int(ids.at(vi).at(rl).size()) };
        lss.push_back(ls);
      }
      return get_existing_id(vi, rl, tl);
    }
    ls_t const& resolve_id(int id) const
    {
      return lss.at(id);
    }
  };
  
  
  
  class vars_t {
    vector<CCTK_INT> vis;
    vector<CCTK_INT> rls;
    vector<CCTK_INT> tls;
  public:
    void push_back(int vi, int rl, int tl)
    {
      vis.push_back(vi);
      rls.push_back(rl);
      tls.push_back(tl);
    }
    CCTK_INT const *vi_ptr() const { return &vis.front(); }
    CCTK_INT const *rl_ptr() const { return &rls.front(); }
    CCTK_INT const *tl_ptr() const { return &tls.front(); }
    int nvars() const { return vis.size(); }
  };
  
  
  
  struct mem_t {
    bool host_valid, device_valid;
  };
  
  class device_t {
    vector<vector<vector<mem_t> > > mems; // [vi][rl][tl]
    void ensure_reflevel(int vi, int rl)
    {
      if (rl >= int(mems.at(vi).size())) {
        // Allocate space for all refinement levels, so that we don't
        // have to reallocate too often
        int const rls = GetRefinementLevels(NULL);
        mems.at(vi).resize(rls);
      }
    }
  public:
    device_t(): mems(CCTK_NumVars()) {}
    int ntimelevels(int vi, int rl) const
    {
      if (rl < int(mems.at(vi).size())) {
        return mems.at(vi).at(rl).size();
      } else {
        return 0;
      }
    }
    void create_timelevel(int vi, int rl, int tl, mem_t const& m)
    {
      ensure_reflevel(vi, rl);
      // We can only create the next missing timelevel
      assert(int(mems.at(vi).at(rl).size()) == tl);
      mems.at(vi).at(rl).resize(tl+1, m);
    }
    mem_t& mem(int vi, int rl, int tl)
    {
      ensure_reflevel(vi, rl);
      return mems.at(vi).at(rl).at(tl);
    }
  };
  
  extern device_t *device;
  
} // namespace Accelerator

#endif  // #ifndef ACCELERATOR_HH
