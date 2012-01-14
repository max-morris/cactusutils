#include "copy.hh"
#include "device.hh"

#include <cctk_Parameters.h>
#include <util_Table.h>

#include <carpet.hh>

#include <cassert>

#ifdef CL_VERSION_1_1
#  define HAVE_BUFFER_RECT_OPS 1
#else
#  define HAVE_BUFFER_RECT_OPS 0
#endif



namespace OpenCLRunTime {
  
  
  
  void copy_to_device(cGH const *restrict const cctkGH,
                      vector<var_t> const& vars)
  {
    assert(Carpet::is_local_mode());
    
    switch (device->mem_model) {
      
    case mm_always_mapped:
      // do nothing
      break;
      
    case mm_copy: {
      
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
          
          if (device->mems.at(vi).at(tl).device_valid) continue;
          
          // TODO: Check this. For now, we just assume this is true,
          // because we don't assume that all provides/requires
          // information is complete and correct.
          // assert(device->mems.at(vi).at(tl).host_valid);
          device->mems.at(vi).at(tl).host_valid = true;
          
          void *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
          
          if (HAVE_BUFFER_RECT_OPS and device->same_padding) {
            checkErr(clEnqueueWriteBuffer(device->queue,
                                          device->mems.at(vi).at(tl).mem,
                                          CL_FALSE,
                                          0, np*sizeof(CCTK_REAL), ptr,
                                          0, NULL, NULL));
          } else {
#if HAVE_BUFFER_RECT_OPS
            checkErr(clEnqueueWriteBufferRect(device->queue,
                                              device->mems.at(vi).at(tl).mem,
                                              CL_FALSE,
                                              offset, offset, length,
                                              dJ, dK, dj, dk,
                                              ptr,
                                              0, NULL, NULL));
#endif
          }
          
          device->mems.at(vi).at(tl).device_valid = true;
          
        }
      } // for var
      
      break;
    }
      
    case mm_map: {
      for (size_t var=0; var<vars.size(); ++var) {
        int const vi=vars.at(var).vi;
        assert(vi>=0);
        int const tl=vars.at(var).tl;
        assert(tl>=0);
        if (int(device->mems.at(vi).size()) > tl) {
          
          if (device->mems.at(vi).at(tl).device_valid) continue;
          
          // TODO: Check this. For now, we just assume this is true,
          // because we don't assume that all provides/requires
          // information is complete and correct.
          // assert(device->mems.at(vi).at(tl).host_valid);
          
          void *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
          
          assert (device->same_padding);
          
          checkErr(clEnqueueUnmapMemObject(device->queue,
                                           device->mems.at(vi).at(tl).mem,
                                           ptr,
                                           0, NULL, NULL));
          
          device->mems.at(vi).at(tl).device_valid = true;
          device->mems.at(vi).at(tl).host_valid   = false;
          
        }
      } // for var
      break;
    }
      
    default:
      assert(0);
    }
  }
  
  
  
  void copy_to_host(cGH const *restrict const cctkGH,
                    vector<var_t> const& vars)
  {
    assert(Carpet::is_local_mode());
    
    int const np =
      cctkGH->cctk_lsh[0] * cctkGH->cctk_lsh[1] * cctkGH->cctk_lsh[2];
    
    cl_int errcode;
    
    switch (device->mem_model) {
      
    case mm_always_mapped:
      // do nothing
      break;
      
    case mm_copy: {
      
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
          
          if (device->mems.at(vi).at(tl).host_valid) continue;
          
          // TODO: Check this. For now, we just assume this is true,
          // because we don't assume that all provides/requires
          // information is complete and correct.
          // assert(device->mems.at(vi).at(tl).device_valid);
          device->mems.at(vi).at(tl).device_valid = true;
          
          void *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
          
          if (HAVE_BUFFER_RECT_OPS and device->same_padding) {
            checkErr(clEnqueueReadBuffer(device->queue,
                                         device->mems.at(vi).at(tl).mem,
                                         CL_FALSE,
                                         0, np*sizeof(CCTK_REAL), ptr,
                                         0, NULL, NULL));
          } else {
#if HAVE_BUFFER_RECT_OPS
            checkErr(clEnqueueReadBufferRect(device->queue,
                                             device->mems.at(vi).at(tl).mem,
                                             CL_FALSE,
                                             offset, offset, length,
                                             dJ, dK, dj, dk,
                                             ptr,
                                             0, NULL, NULL));
#endif
          }
          
          device->mems.at(vi).at(tl).host_valid = true;
          
        }
      } // for var
      
      // Finish, because we output
      checkErr(clFinish(device->queue));
      
      break;
    }
      
    case mm_map: {
      for (size_t var=0; var<vars.size(); ++var) {
        int const vi=vars.at(var).vi;
        assert(vi>=0);
        int const tl=vars.at(var).tl;
        assert(tl>=0);
        if (int(device->mems.at(vi).size()) > tl) {
          
          if (device->mems.at(vi).at(tl).device_valid) continue;
          
          // TODO: Check this. For now, we just assume this is true,
          // because we don't assume that all provides/requires
          // information is complete and correct.
          // assert(device->mems.at(vi).at(tl).host_valid);
          
          assert (device->same_padding);
          
          void *ptr;
          checkErr((ptr = clEnqueueMapBuffer(device->queue,
                                             device->mems.at(vi).at(tl).mem,
                                             CL_FALSE,
                                             CL_MAP_READ | CL_MAP_WRITE,
                                             0, np*sizeof(CCTK_REAL),
                                             0, NULL, NULL, &errcode),
                    errcode));
          
          assert(ptr == CCTK_VarDataPtrI(cctkGH, tl, vi));
          
          device->mems.at(vi).at(tl).device_valid = true;
          device->mems.at(vi).at(tl).host_valid   = false;
          
        }
      } // for var
      break;
    }
      
    default:
      assert(0);
    }
  }
  
  
  
  void copy_presync(cGH const *restrict const cctkGH,
                    vector<var_t> const& vars)
  {
    assert(Carpet::is_local_mode());
    
    if (device->mem_model == mm_always_mapped) return;
    assert (device->mem_model != mm_map);
    
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
      
      void *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
      
#if HAVE_BUFFER_RECT_OPS
      
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
            
            checkErr(clEnqueueReadBufferRect(device->queue,
                                             device->mems.at(vi).at(tl).mem,
                                             CL_FALSE,
                                             offset, offset, length,
                                             dJ, dK, dj, dk,
                                             ptr,
                                             0, NULL, NULL));
            
          } // if bbox
        }   // for face
      }     // for dir
      
#else
      
      bool have_sync_bnd = false;
      for (int dir=0; dir<dim; ++dir) {
        for (int face=0; face<2; ++face) {
          if (not cctkGH->cctk_bbox[2*dir+face]) {
            have_sync_bnd = true;
          }
        }
      }
      
      if (have_sync_bnd) {
        int const np =
          cctkGH->cctk_lsh[0] * cctkGH->cctk_lsh[1] * cctkGH->cctk_lsh[2];
        
        checkErr(clEnqueueReadBuffer(device->queue,
                                     device->mems.at(vi).at(tl).mem,
                                     CL_FALSE,
                                     0, np*sizeof(CCTK_REAL), ptr,
                                     0, NULL, NULL));
      }
      
#endif
      
    } // for var
    
    // Finish, because we sync
    checkErr(clFinish(device->queue));
  }
  
  
  
  void copy_postsync(cGH const *restrict const cctkGH,
                     vector<var_t> const& vars)
  {
    assert(Carpet::is_local_mode());
    
    if (device->mem_model == mm_always_mapped) return;
    assert (device->mem_model != mm_map);
    
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
      
      void const *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
      
#if HAVE_BUFFER_RECT_OPS
      
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
            
            checkErr(clEnqueueWriteBufferRect(device->queue,
                                              device->mems.at(vi).at(tl).mem,
                                              CL_FALSE,
                                              offset, offset, length,
                                              dJ, dK, dj, dk,
                                              ptr,
                                              0, NULL, NULL));

          } // if bbox
        }   // if face
      }     // if dir
      
#else
      
      bool have_sync_bnd = false;
      for (int dir=0; dir<dim; ++dir) {
        for (int face=0; face<2; ++face) {
          if (not cctkGH->cctk_bbox[2*dir+face]) {
            have_sync_bnd = true;
          }
        }
      }
      
      if (have_sync_bnd) {
        int const np =
          cctkGH->cctk_lsh[0] * cctkGH->cctk_lsh[1] * cctkGH->cctk_lsh[2];
        
        checkErr(clEnqueueWriteBuffer(device->queue,
                                      device->mems.at(vi).at(tl).mem,
                                      CL_FALSE,
                                      0, np*sizeof(CCTK_REAL), ptr,
                                      0, NULL, NULL));
      }
      
#endif            
            
    } // for var
  }
  
  
  
  void copy_cycle(cGH const *restrict const cctkGH)
  {
    assert(Carpet::is_local_mode());
    
    if (device->mem_model == mm_always_mapped) return;
    assert(device->mem_model != mm_map);
    
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
                                       device->mems.at(vi).at(tl-1).mem,
                                       device->mems.at(vi).at(tl).mem,
                                       0, 0, NP*sizeof(CCTK_REAL),
                                       have_event ? 1 : 0,
                                       have_event ? &event : NULL,
                                       &new_event));
          event = new_event;
          have_event = true;
          
          device->mems.at(vi).at(tl).device_valid =
            device->mems.at(vi).at(tl-1).device_valid;
          // Cycle host information here as well
          device->mems.at(vi).at(tl).host_valid =
            device->mems.at(vi).at(tl-1).host_valid;
        }
        device->mems.at(vi).at(0).device_valid = false;
        // Cycle host information here as well
        device->mems.at(vi).at(0).host_valid = false;
      }
    }
  }
  
  
  
  void copy_from_past(cGH const *restrict const cctkGH,
                      vector<var_t> const& vars)
  {
    assert(Carpet::is_local_mode());
    
    if (device->mem_model == mm_always_mapped) return;
    assert(device->mem_model != mm_map);
    
    int const NP =
      device->grid.lsh[0] * device->grid.lsh[1] * device->grid.lsh[2];
    
    for (size_t var=0; var<vars.size(); ++var) {
      int const vi=vars.at(var).vi;
      assert(vi>=0);
      int const tl=vars.at(var).tl;
      assert(tl>=0);
      assert (tl+1 < int(device->mems.at(vi).size()));
        
      // TODO: Check this. For now, we just assume this is true,
      // because we don't assume that all provides/requires
      // information is complete and correct.
      // assert(device->mems.at(vi).at(tl+1).device_valid);
      device->mems.at(vi).at(tl+1).device_valid = true;
      
      checkErr(clEnqueueCopyBuffer(device->queue,
                                   device->mems.at(vi).at(tl+1).mem,
                                   device->mems.at(vi).at(tl).mem,
                                   0, 0, NP*sizeof(CCTK_REAL),
                                   0, NULL, NULL));
      
      device->mems.at(vi).at(tl).device_valid =
        device->mems.at(vi).at(tl+1).device_valid;
      
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
  CCTK_INT OpenCLRunTime_CopyFromPast(CCTK_POINTER_TO_CONST const cctkGH_,
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
    
    copy_from_past(cctkGH, vars);
    
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
    DECLARE_CCTK_PARAMETERS;
    
    // Don't do anything before the device has been set up. (Note that
    // the device setup routine is called via CallFunction.)
    if (not device) return 0;
    
    // Can only handle grid functions if called in local mode
    if (not Carpet::is_local_mode()) return 0;
    
    if (veryverbose) {
      CCTK_VInfo(CCTK_THORNSTRING, "PreCallFunction");
      
      cout << "[" << attribute->where << "] "
           << attribute->thorn << "::" << attribute->routine << "\n";
      
      cout << "   SyncGroups:";
      for (int n=0; n<attribute->n_SyncGroups; ++n) {
        char *const groupname = CCTK_GroupName(attribute->SyncGroups[n]);
        cout << " " << groupname;
        free(groupname);
      }
      cout << "\n";
      
      cout << "   Triggers:";
      for (int n=0; n<attribute->n_TriggerGroups; ++n) {
        cout << " " << attribute->TriggerGroups[n];
      }
      cout << "\n";
      
      cout << "   Requires:";
      for (int n=0; n<attribute->n_RequiresClauses; ++n) {
        cout << " " << attribute->RequiresClauses[n];
      }
      cout << "\n";
      
      cout << "   Provides:";
      for (int n=0; n<attribute->n_ProvidesClauses; ++n) {
        cout << " " << attribute->ProvidesClauses[n];
      }
      cout << "\n";
      
      cout << "   Tags: ";
      Util_TablePrintPretty(stdout, attribute->tags);
      cout << "\n";
    }
    
    // Is this an OpenCL routine?
    CCTK_INT is_opencl;
    int const ierr = Util_TableGetInt(attribute->tags, &is_opencl, "OpenCL");
    if (ierr == UTIL_ERROR_TABLE_NO_SUCH_KEY) {
      is_opencl = 0;            // default
    } else if (ierr <= 0) {
      CCTK_WARN (CCTK_WARN_ABORT, "Error with schedule tag \"OpenCL\"");
    }
    
    // Copy all required variables to the device or to the host,
    // depending on the language (OpenCL or not)
    bool mem_t:: *valid;
    void (*copy) (cGH const *restrict const cctkGH, vector<var_t> const& vars);
    if (is_opencl) {
      valid = &mem_t::device_valid;
      copy = copy_to_device;
    } else {
      valid = &mem_t::host_valid;
      copy = copy_to_host;
    }
    
    vector<var_t> vars;
    for (int n=0; n<attribute->n_RequiresClauses; ++n) {
      int const gi = CCTK_GroupIndex(attribute->RequiresClauses[n]);
      assert(gi>=0);
      int const nv = CCTK_NumVarsInGroupI(gi);
      assert(nv>=0);
      if (nv > 0) {
        int const v0 = CCTK_FirstVarIndexI(gi);
        assert(v0>=0);
        for (int vi=v0; vi<v0+nv; ++vi) {
          int const tl=0;       // only copy current timelevel
          if (int(device->mems.at(vi).size()) > tl) {
            if (not (device->mems.at(vi).at(tl).*valid)) {
              var_t const var = {vi, tl};
              vars.push_back(var);
            }
          }
        }
      }
    }
    
    if (veryverbose) {
      CCTK_VInfo(CCTK_THORNSTRING, "Copying in");
      cout << "[" << attribute->where << "] "
           << attribute->thorn << "::" << attribute->routine << "\n";
      cout << "   Copy in:";
      for (size_t n=0; n<vars.size(); ++n) {
        char *const fullname = CCTK_FullName(vars.at(n).vi);
        cout << " " << fullname << "/" << vars.at(n).tl;
        free(fullname);
      }
      cout << "\n";
    }
    
    copy(cctkGH, vars);
    
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
    
    // Can only handle grid functions if called in local mode
    if (not Carpet::is_local_mode()) return 0;
    
    if (veryverbose) {
      CCTK_VInfo(CCTK_THORNSTRING, "PostCallFunction");
      
      cout << "[" << attribute->where << "] "
           << attribute->thorn << "::" << attribute->routine << "\n";
      
      cout << "   SyncGroups:";
      for (int n=0; n<attribute->n_SyncGroups; ++n) {
        char *const groupname = CCTK_GroupName(attribute->SyncGroups[n]);
        cout << " " << groupname;
        free(groupname);
      }
      cout << "\n";
      
      cout << "   Triggers:";
      for (int n=0; n<attribute->n_TriggerGroups; ++n) {
        cout << " " << attribute->TriggerGroups[n];
      }
      cout << "\n";
      
      cout << "   Requires:";
      for (int n=0; n<attribute->n_RequiresClauses; ++n) {
        cout << " " << attribute->RequiresClauses[n];
      }
      cout << "\n";
      
      cout << "   Provides:";
      for (int n=0; n<attribute->n_ProvidesClauses; ++n) {
        cout << " " << attribute->ProvidesClauses[n];
      }
      cout << "\n";
      
      cout << "   Tags: ";
      Util_TablePrintPretty(stdout, attribute->tags);
      cout << "\n";
    }
    
    // Is this an OpenCL routine?
    CCTK_INT is_opencl;
    int const ierr = Util_TableGetInt(attribute->tags, &is_opencl, "OpenCL");
    if (ierr == UTIL_ERROR_TABLE_NO_SUCH_KEY) {
      is_opencl = 0;            // default
    } else if (ierr <= 0) {
      CCTK_WARN (CCTK_WARN_ABORT, "Error with schedule tag \"OpenCL\"");
    }
    
    // Mark all provided variables as valid, and mark them as invalid
    // on the other end
    bool mem_t:: *valid;
    bool mem_t:: *invalid;
    if (is_opencl) {
      valid = &mem_t::device_valid;
      invalid = &mem_t::host_valid;
    } else {
      valid = &mem_t::host_valid;
      invalid = &mem_t::device_valid;
    }
    
    for (int n=0; n<attribute->n_ProvidesClauses; ++n) {
      int const gi = CCTK_GroupIndex(attribute->ProvidesClauses[n]);
      assert(gi>=0);
      int const nv = CCTK_NumVarsInGroupI(gi);
      assert(nv>=0);
      if (nv > 0) {
        int const v0 = CCTK_FirstVarIndexI(gi);
        assert(v0>=0);
        for (int vi=v0; vi<v0+nv; ++vi) {
          int const tl=0;       // only mark current timelevel
          if (int(device->mems.at(vi).size()) > tl) {
            device->mems.at(vi).at(tl).*valid = true;
            device->mems.at(vi).at(tl).*invalid = false;
          }
        }
      }
    }
    
    // If we are in the analysis bin, and if this is an OpenCL
    // routine, then copy back all provided variables since they may
    // be output. Otherwise, do nothing.
    // TODO: Add a hook to the flesh to do this only when an I/O
    // method has been called, and then copy only those variables
    // necessary.
    if (Carpet::in_analysis_bin and is_opencl) {
      
      vector<var_t> vars;
      for (int n=0; n<attribute->n_ProvidesClauses; ++n) {
        int const gi = CCTK_GroupIndex(attribute->ProvidesClauses[n]);
        assert(gi>=0);
        int const nv = CCTK_NumVarsInGroupI(gi);
        assert(nv>=0);
        if (nv > 0) {
          int const v0 = CCTK_FirstVarIndexI(gi);
          assert(v0>=0);
          for (int vi=v0; vi<v0+nv; ++vi) {
            int const tl=0;       // only copy current timelevel
            if (int(device->mems.at(vi).size()) > tl) {
              var_t const var = {vi, tl};
              vars.push_back(var);
            }
          }
        }
      }
      
      if (veryverbose) {
        CCTK_VInfo(CCTK_THORNSTRING, "Copying out");
        cout << "[" << attribute->where << "] "
             << attribute->thorn << "::" << attribute->routine << "\n";
        cout << "   Copy in:";
        for (size_t n=0; n<vars.size(); ++n) {
          char *const fullname = CCTK_FullName(vars.at(n).vi);
          cout << " " << fullname << "/" << vars.at(n).tl;
          free(fullname);
        }
        cout << "\n";
      }
      
      copy_to_host(cctkGH, vars);
      
    }
    
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
