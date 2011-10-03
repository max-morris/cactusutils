#include "copy.h"
#include "device.h"

#include <cctk_Parameters.h>

#include <carpet.hh>

#include <cassert>



namespace OpenCLRunTime {
  
  
  
  void copy_to_device(cGH const *restrict const cctkGH,
                      vector<var_t> const& vars)
  {
    assert(Carpet::is_local_mode());
    
    if (device->mem_model == mm_always_mapped) return;
    
    int const np =
      cctkGH->cctk_lsh[0] * cctkGH->cctk_lsh[1] * cctkGH->cctk_lsh[2];
    
    int const di = sizeof(CCTK_REAL);
    int const dj = di * cctkGH->cctk_lsh[0];
    int const dk = dj * cctkGH->cctk_lsh[1];
    int const dI = sizeof(CCTK_REAL);
    int const dJ = dI * device->grid.lsh[0];
    int const dK = dJ * device->grid.lsh[1];
    
    size_t offset[dim];
    size_t length[dim];
    for (int d=0; d<dim; ++d) {
      offset[d] = 0;
      length[d] = cctkGH->CCTK_LSSH(0,d);
    }
    offset[0] *= di;
    length[0] *= di;
    
    for (size_t var=0; var<vars.size(); ++var) {
      int const vi=vars.at(var).vi;
      assert(vi>=0);
      int const tl=vars.at(var).tl;
      assert(tl>=0);
      if (int(device->mems.at(vi).size()) > tl) {
        
        void *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
        
        if (device->same_padding) {
          checkErr(clEnqueueWriteBuffer(device->queue,
                                        device->mems.at(vi).at(tl),
                                        CL_FALSE,
                                        0, np*sizeof(CCTK_REAL), ptr,
                                        0, NULL, NULL));
        } else {
          checkErr(clEnqueueWriteBufferRect(device->queue,
                                            device->mems.at(vi).at(tl),
                                            CL_FALSE,
                                            offset, offset, length,
                                            dJ, dK, dj, dk,
                                            ptr,
                                            0, NULL, NULL));
        }
        
      }
    } // for var
  }
  
  
  
  void copy_to_host(cGH const *restrict const cctkGH,
                    vector<var_t> const& vars)
  {
    assert(Carpet::is_local_mode());
    
    if (device->mem_model == mm_always_mapped) return;
    
    int const np =
      cctkGH->cctk_lsh[0] * cctkGH->cctk_lsh[1] * cctkGH->cctk_lsh[2];
    
    int const di = sizeof(CCTK_REAL);
    int const dj = di * cctkGH->cctk_lsh[0];
    int const dk = dj * cctkGH->cctk_lsh[1];
    int const dI = sizeof(CCTK_REAL);
    int const dJ = dI * device->grid.lsh[0];
    int const dK = dJ * device->grid.lsh[1];
    
    size_t offset[dim];
    size_t length[dim];
    for (int d=0; d<dim; ++d) {
      offset[d] = 0;
      length[d] = cctkGH->CCTK_LSSH(0,d);
    }
    offset[0] *= di;
    length[0] *= di;
    
    for (size_t var=0; var<vars.size(); ++var) {
      int const vi=vars.at(var).vi;
      assert(vi>=0);
      int const tl=vars.at(var).tl;
      assert(tl>=0);
      if (int(device->mems.at(vi).size()) > tl) {
        
        void *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
        
        if (device->same_padding) {
          checkErr(clEnqueueReadBuffer(device->queue,
                                       device->mems.at(vi).at(tl),
                                       CL_FALSE,
                                       0, np*sizeof(CCTK_REAL), ptr,
                                       0, NULL, NULL));
        } else {
          checkErr(clEnqueueReadBufferRect(device->queue,
                                           device->mems.at(vi).at(tl),
                                           CL_FALSE,
                                           offset, offset, length,
                                           dJ, dK, dj, dk,
                                           ptr,
                                           0, NULL, NULL));
        }
        
      }
    } // for var
  }
  
  
  
  void copy_presync(cGH const *restrict const cctkGH,
                    vector<var_t> const& vars)
  {
    assert(Carpet::is_local_mode());
    
    if (device->mem_model == mm_always_mapped) return;
    
    int const dI = sizeof(CCTK_REAL);
    int const dJ = dI * device->grid.lsh[0];
    int const dK = dJ * device->grid.lsh[1];
    int const di = sizeof(CCTK_REAL);
    int const dj = di * cctkGH->cctk_lsh[0];
    int const dk = dj * cctkGH->cctk_lsh[1];
    
    for (size_t var=0; var<vars.size(); ++var) {
      int const vi=vars.at(var).vi;
      assert(vi>=0);
      int const tl=vars.at(var).tl;
      assert(tl>=0);
      assert (tl < int(device->mems.at(vi).size()));
      
      for (int dir=0; dir<dim; ++dir) {
        for (int face=0; face<2; ++face) {
          if (not cctkGH->cctk_bbox[2*dir+face]) {
            
            int imin[dim];
            int imax[dim];
            for (int d=0; d<dim; ++d) {
              // Whole domain
              imin[d] = 0;
              imax[d] = cctkGH->CCTK_LSSH(0,d);
              // Skip ghost points
              if (not cctkGH->cctk_bbox[2*d+0]) {
                imin[d] += cctkGH->cctk_nghostzones[d];
              }
              if (not cctkGH->cctk_bbox[2*d+1]) {
                imax[d] -= cctkGH->cctk_nghostzones[d];
              }
              // Skip points that will be copied via later directions
              if (d > dir) {
                if (not cctkGH->cctk_bbox[2*d+0]) {
                  imin[d] += cctkGH->cctk_nghostzones[d];
                }
                if (not cctkGH->cctk_bbox[2*d+1]) {
                  imax[d] -= cctkGH->cctk_nghostzones[d];
                }
              }
            }
            if (face==0) {
              imin[dir] = cctkGH->cctk_nghostzones[dir];
              imax[dir] = imin[dir] + cctkGH->cctk_nghostzones[dir];
            } else {
              imax[dir] =
                cctkGH->CCTK_LSSH(0,dir) - cctkGH->cctk_nghostzones[dir];
              imin[dir] = imax[dir] - cctkGH->cctk_nghostzones[dir];
            }
            for (int d=0; d<dim; ++d) {
              assert(imin[d] >= 0);
              assert(imin[d] <= imax[d]);
              assert(imax[d] <= cctkGH->CCTK_LSSH(0,d));
            }
            
            size_t offset[dim];
            size_t length[dim];
            for (int d=0; d<dim; ++d) {
              offset[d] = imin[d];
              length[d] = imax[d] - imin[d];
            }
            offset[0] *= di;
            length[0] *= di;
            
            void *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
            
            checkErr(clEnqueueReadBufferRect(device->queue,
                                             device->mems.at(vi).at(tl),
                                             CL_FALSE,
                                             offset, offset, length,
                                             dJ, dK, dj, dk,
                                             ptr,
                                             0, NULL, NULL));
            
          } // if bbox
        }   // for face
      }     // for dir
      
    } // for var
  }
  
  
  
  void copy_postsync(cGH const *restrict const cctkGH,
                     vector<var_t> const& vars)
  {
    assert(Carpet::is_local_mode());
    
    if (device->mem_model == mm_always_mapped) return;
    
    int const dI = sizeof(CCTK_REAL);
    int const dJ = dI * device->grid.lsh[0];
    int const dK = dJ * device->grid.lsh[1];
    int const di = sizeof(CCTK_REAL);
    int const dj = di * cctkGH->cctk_lsh[0];
    int const dk = dj * cctkGH->cctk_lsh[1];
    
    for (size_t var=0; var<vars.size(); ++var) {
      int const vi=vars.at(var).vi;
      assert(vi>=0);
      int const tl=vars.at(var).tl;
      assert(tl>=0);
      assert (tl < int(device->mems.at(vi).size()));
      
      for (int dir=0; dir<dim; ++dir) {
        for (int face=0; face<2; ++face) {
          if (not cctkGH->cctk_bbox[2*dir+face]) {
            
            int imin[dim];
            int imax[dim];
            for (int d=0; d<dim; ++d) {
              // Whole domain
              imin[d] = 0;
              imax[d] = cctkGH->CCTK_LSSH(0,d);
              // Skip points that will be copied via later directions
              if (d > dir) {
                if (not cctkGH->cctk_bbox[2*d+0]) {
                  imin[d] += cctkGH->cctk_nghostzones[d];
                }
                if (not cctkGH->cctk_bbox[2*d+1]) {
                  imax[d] -= cctkGH->cctk_nghostzones[d];
                }
              }
            }
            if (face==0) {
              imin[dir] = 0;
              imax[dir] = imin[dir] + cctkGH->cctk_nghostzones[dir];
            } else {
              imax[dir] = cctkGH->CCTK_LSSH(0,dir);
              imin[dir] = imax[dir] - cctkGH->cctk_nghostzones[dir];
            }
            for (int d=0; d<dim; ++d) {
              assert(imin[d] >= 0);
              assert(imin[d] <= imax[d]);
              assert(imax[d] <= cctkGH->CCTK_LSSH(0,d));
            }
            
            size_t offset[dim];
            size_t length[dim];
            for (int d=0; d<dim; ++d) {
              offset[d] = imin[d];
              length[d] = imax[d] - imin[d];
            }
            offset[0] *= di;
            length[0] *= di;
            
            void const *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
            
            checkErr(clEnqueueWriteBufferRect(device->queue,
                                              device->mems.at(vi).at(tl),
                                              CL_FALSE,
                                              offset, offset, length,
                                              dJ, dK, dj, dk,
                                              ptr,
                                              0, NULL, NULL));
            
          } // if bbox
        }   // if face
      }     // if dir
      
    } // for var
  }
  
  
  
  void copy_cycle(cGH const *restrict const cctkGH)
  {
    assert(Carpet::is_local_mode());
    
    if (device->mem_model == mm_always_mapped) return;
    
    int const NP =
      device->grid.lsh[0] * device->grid.lsh[1] * device->grid.lsh[2];
    
    for (int vi=0; vi<CCTK_NumVars(); ++vi) {
      int const num_tl = device->mems.at(vi).size();
      if (num_tl > 1) {
        cl_event event;
        bool have_event = false;
        for (int tl=num_tl-1; tl>0; --tl) {
          cl_event new_event;
          checkErr(clEnqueueCopyBuffer(device->queue,
                                       device->mems.at(vi).at(tl-1),
                                       device->mems.at(vi).at(tl),
                                       0, 0, NP*sizeof(CCTK_REAL),
                                       have_event ? 1 : 0,
                                       have_event ? &event : NULL,
                                       &new_event));
          event = new_event;
          have_event = true;
        }
      }
    }
  }
  
  
  
  void copy_mol(cGH const *restrict const cctkGH,
                vector<var_t> const& vars)
  {
    assert(Carpet::is_local_mode());
    
    if (device->mem_model == mm_always_mapped) return;
    
    int const NP =
      device->grid.lsh[0] * device->grid.lsh[1] * device->grid.lsh[2];
    
    for (size_t var=0; var<vars.size(); ++var) {
      int const vi=vars.at(var).vi;
      assert(vi>=0);
      int const tl=vars.at(var).tl;
      assert(tl>=0);
      assert (tl+1 < int(device->mems.at(vi).size()));
      
      checkErr(clEnqueueCopyBuffer(device->queue,
                                   device->mems.at(vi).at(tl+1),
                                   device->mems.at(vi).at(tl),
                                   0, 0, NP*sizeof(CCTK_REAL),
                                   0, NULL, NULL));
      
    } // for var
  }
  
  
  
  //////////////////////////////////////////////////////////////////////////////
  
  
  
  extern "C"
  CCTK_INT OpenCLRunTime_Cycle(CCTK_POINTER_TO_CONST const cctkGH_)
  {
    cGH const *restrict const cctkGH = static_cast<cGH const*>(cctkGH_);
    DECLARE_CCTK_PARAMETERS;
    
    if (veryverbose) {
      CCTK_VInfo(CCTK_THORNSTRING, "Cycle");
    }
    
    if (device->mem_model == mm_always_mapped) return 0;
    
    BEGIN_LOCAL_MAP_LOOP(cctkGH, CCTK_GF) {
      BEGIN_LOCAL_COMPONENT_LOOP(cctkGH, CCTK_GF) {
        
        copy_cycle(cctkGH);
        
      } END_LOCAL_COMPONENT_LOOP;
    } END_LOCAL_MAP_LOOP;
    
    return 0;
  }
  
  
  
  extern "C"
  CCTK_INT OpenCLRunTime_CopyMoL(CCTK_POINTER_TO_CONST const cctkGH_,
                                 CCTK_INT const varindices[],
                                 CCTK_INT const nvars)
  {
    cGH const *restrict const cctkGH = static_cast<cGH const*>(cctkGH_);
    DECLARE_CCTK_PARAMETERS;
    
    if (veryverbose) {
      CCTK_VInfo(CCTK_THORNSTRING, "CopyMoL");
    }
    
    assert(nvars>=0);
    vector<var_t> vars(nvars);
    for (int var=0; var<nvars; ++var) {
      vars.at(var).vi = varindices[var];
      vars.at(var).tl = 0;
    }
    
    copy_mol(cctkGH, vars);
    
    return 0;
  }
  
  
  
  extern "C"
  CCTK_INT
  OpenCLRunTime_PreCallFunction(CCTK_POINTER_TO_CONST const cctkGH_,
                                CCTK_POINTER_TO_CONST const attribute_)
  {
    assert(cctkGH_);
    cGH const *restrict const cctkGH CCTK_ATTRIBUTE_UNUSED =
      static_cast<cGH const*>(cctkGH_);
    assert(attribute_);
    cFunctionData const *restrict const attribute CCTK_ATTRIBUTE_UNUSED =
      static_cast<cFunctionData const*>(attribute_);
    
    return 0;
  }

  
  
  extern "C"
  CCTK_INT
  OpenCLRunTime_PostCallFunction(CCTK_POINTER_TO_CONST const cctkGH_,
                                 CCTK_POINTER_TO_CONST const attribute_)
  {
    assert(cctkGH_);
    cGH const *restrict const cctkGH = static_cast<cGH const*>(cctkGH_);
    assert(attribute_);
    cFunctionData const *restrict const attribute CCTK_ATTRIBUTE_UNUSED =
      static_cast<cFunctionData const*>(attribute_);
    DECLARE_CCTK_PARAMETERS;
    
    // Don't do anything before the device has been set up. (Note that
    // the device setup routine is called via CallFunction.)
    if (not device) return 0;
    
    // Only copy back if called in local mode
    if (not Carpet::is_local_mode()) return 0;
    
    if (veryverbose) {
      CCTK_VInfo(CCTK_THORNSTRING, "PostCallFunction");
    }
    
    vector<var_t> vars;
    for (int vi=0; vi<int(device->mems.size()); ++vi) {
      int const tl=0;           // only copy current timelevel
      if (int(device->mems.at(vi).size()) > tl) {
        var_t const var = {vi, tl};
        vars.push_back(var);
      }
    }
    
    copy_to_host(cctkGH, vars);
    
    return 0;
  }
  
  
  
  extern "C"
  CCTK_INT OpenCLRunTime_PreSync(CCTK_POINTER_TO_CONST const cctkGH_,
                                 CCTK_INT const groups[],
                                 CCTK_INT const ngroups)
  {
    cGH const *restrict const cctkGH = static_cast<cGH const*>(cctkGH_);
    DECLARE_CCTK_PARAMETERS;
    
    if (veryverbose) {
      stringstream buf;
      for (int group=0; group<ngroups; ++group) {
        int const gi = groups[group];
        char *const groupname = CCTK_GroupName(gi);
        buf << " " << groupname;
        free(groupname);
      }
      CCTK_VInfo(CCTK_THORNSTRING, "PreSync%s", buf.str().c_str());
    }
    
    vector<var_t> vars;
    assert(ngroups>=0);
    for (int group=0; group<ngroups; ++group) {
      int const gi = groups[group];
      assert(gi>=0);
      int const nv = CCTK_NumVarsInGroupI(gi);
      assert(nv>=0);
      if (nv > 0) {
        int const v0 = CCTK_FirstVarIndexI(gi);
        assert(v0>=0);
        for (int vi=v0; vi<v0+nv; ++vi) {
          int const tl=0;       // only copy current timelevel
          var_t const var = {vi,tl};
          vars.push_back(var);
        }
      }
    }
    
    BEGIN_LOCAL_MAP_LOOP(cctkGH, CCTK_GF) {
      BEGIN_LOCAL_COMPONENT_LOOP(cctkGH, CCTK_GF) {
        
        if (sync_copy_whole_buffer) {
          copy_to_host(cctkGH, vars);
        } else {
          copy_presync(cctkGH, vars);
        }
        
      } END_LOCAL_COMPONENT_LOOP;
    } END_LOCAL_MAP_LOOP;
    
    return 0;
  }
  
  
  
  extern "C"
  CCTK_INT OpenCLRunTime_PostSync(CCTK_POINTER_TO_CONST const cctkGH_,
                                  CCTK_INT const groups[],
                                  CCTK_INT const ngroups)
  {
    cGH const *restrict const cctkGH = static_cast<cGH const*>(cctkGH_);
    DECLARE_CCTK_PARAMETERS;
    
    if (veryverbose) {
      stringstream buf;
      for (int group=0; group<ngroups; ++group) {
        int const gi = groups[group];
        char *const groupname = CCTK_GroupName(gi);
        buf << " " << groupname;
        free(groupname);
      }
      CCTK_VInfo(CCTK_THORNSTRING, "PostSync%s", buf.str().c_str());
    }
    
    vector<var_t> vars;
    assert(ngroups>=0);
    for (int group=0; group<ngroups; ++group) {
      int const gi = groups[group];
      assert(gi>=0);
      int const nv = CCTK_NumVarsInGroupI(gi);
      assert(nv>=0);
      if (nv > 0) {
        int const v0 = CCTK_FirstVarIndexI(gi);
        assert(v0>=0);
        for (int vi=v0; vi<v0+nv; ++vi) {
          int const tl=0;       // only copy current timelevel
          var_t const var = {vi,tl};
          vars.push_back(var);
        }
      }
    }
    
    BEGIN_LOCAL_MAP_LOOP(cctkGH, CCTK_GF) {
      BEGIN_LOCAL_COMPONENT_LOOP(cctkGH, CCTK_GF) {
        
        if (sync_copy_whole_buffer) {
          copy_to_device(cctkGH, vars);
        } else {
          copy_postsync(cctkGH, vars);
        }
        
      } END_LOCAL_COMPONENT_LOOP;
    } END_LOCAL_MAP_LOOP;
    
    return 0;
  }
  
  
  
  extern "C"
  void OpenCLRunTime_CopyBack(CCTK_ARGUMENTS)
  {
    DECLARE_CCTK_ARGUMENTS;
    DECLARE_CCTK_PARAMETERS;
    
    if (veryverbose) {
      CCTK_VInfo(CCTK_THORNSTRING, "CopyBack");
    }
    
    vector<var_t> vars;
    for (int vi=0; vi<int(device->mems.size()); ++vi) {
      int const tl=0;           // only copy current timelevel
      if (int(device->mems.at(vi).size()) > tl) {
        var_t const var = {vi, tl};
        vars.push_back(var);
      }
    }
    
    copy_to_host(cctkGH, vars);
  }
  
  
  
} // namespace OpenCLRunTime
