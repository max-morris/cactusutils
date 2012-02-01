#include "accelerator.hh"

#include <cctk.h>
#include <cctk_Parameters.h>
#include <util_Table.h>

#include <carpet.hh>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace std;



// #define n_ReadsClauses  n_RequiresClauses
// #define n_WritesClauses n_ProvidesClauses
// #define ReadsClauses    RequiresClauses
// #define WritesClauses   ProvidesClauses




namespace Accelerator {
  
  device_t *device = NULL;
  
  
  
  //////////////////////////////////////////////////////////////////////////////
  
  
  
  extern "C"
  CCTK_INT
  AcceleratorThorn_Cycle(CCTK_POINTER_TO_CONST const cctkGH_)
  {
    cGH const *restrict const cctkGH = static_cast<cGH const*>(cctkGH_);
    DECLARE_CCTK_PARAMETERS;
    
    if (veryverbose) {
      CCTK_VInfo(CCTK_THORNSTRING, "Cycle");
    }
    
    vars_t vars;
    for (int vi=0; vi<CCTK_NumVars(); ++vi) {
      int const num_tl = device->mems.at(vi).size();
      // Don't cycle single-timelevel variables
      
      // TODO: There is a suble bug here that (luckily) doesn't have
      // any effect. For historic reasons, we do not track all
      // timelevels of all variables, we only track those timelevels
      // that we have "seen" before. However, time level cycling does
      // not make us track an older timelevel that may now be valid.
      // This means that we will assume this (old) timelevel to be
      // invalid on the device, and will copy it from the host. (This
      // can only occur at the first iteration.) Luckily, we copy all
      // data back to the host at the end of every time step (for
      // output), so that the host will have valid data.
      
      // For example, if we initialise the current time level on the
      // device, it will be valid there. Timelevel cycling at the
      // beginning of the first iteration should then "know" that the
      // past timelevel is now also correct, but it won't (because of
      // this bug). MoL will copy the past timelevel to the current
      // timelevel, transferring data from the host to the device
      // because the past timelevel on the device is not marked as
      // valid. This works only if the past timelevel on the host is
      // valid, i.e. if the current timelevel on the host was valid
      // before timelevel cycling. This happens to be the case,
      // because we copied it back to the host after analysis.
      
      // The "correct" solution is to track all timelevels of all
      // variables at all times, e.g. in the driver, instead of in
      // this thorn here.
      
      if (num_tl > 1) {
        for (int tl=num_tl-1; tl>0; --tl) {
          vars.push_back(vi, tl);
          
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
    
    BEGIN_LOCAL_MAP_LOOP(cctkGH, CCTK_GF) {
      BEGIN_LOCAL_COMPONENT_LOOP(cctkGH, CCTK_GF) {
        
        Device_CopyCycle(cctkGH, vars.vi_ptr(), vars.tl_ptr(), vars.nvars());
        
      } END_LOCAL_COMPONENT_LOOP;
    } END_LOCAL_MAP_LOOP;
    
    return 0;
  }
  
  
  
  extern "C"
  CCTK_INT
  AcceleratorThorn_CopyFromPast(CCTK_POINTER_TO_CONST const cctkGH_,
                                CCTK_INT const vis[],
                                CCTK_INT const nvars)
  {
    cGH const *restrict const cctkGH = static_cast<cGH const*>(cctkGH_);
    DECLARE_CCTK_PARAMETERS;
    
    if (veryverbose) {
      CCTK_VInfo(CCTK_THORNSTRING, "Cycle");
    }
    
    vars_t vars;
    for (int var=0; var<nvars; ++var) {
      int const vi = vis[var];
      int const tl = 0;
      
      // TODO: Check this. For now, we just assume this is true,
      // because we don't assume that all provides/requires
      // information is complete and correct. Also, we don't track
      // validity over time level cycling quite correctly yet.
      // assert(device->mems.at(vi).at(tl+1).device_valid);
      device->mems.at(vi).at(tl+1).device_valid = true;
      
      vars.push_back(vi, tl);
      
      device->mems.at(vi).at(tl).device_valid =
        device->mems.at(vi).at(tl+1).device_valid;
    }
    
    return 0;
  }
  
  
  
  extern "C"
  CCTK_INT
  AcceleratorThorn_PreCallFunction(CCTK_POINTER_TO_CONST const cctkGH_,
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
      
      cout << "   Reads:";
      for (int n=0; n<attribute->n_ReadsClauses; ++n) {
        cout << " " << attribute->ReadsClauses[n];
      }
      cout << "\n";
      
      cout << "   Writes:";
      for (int n=0; n<attribute->n_WritesClauses; ++n) {
        cout << " " << attribute->WritesClauses[n];
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
    bool mem_t::*dst_valid, mem_t::*src_valid;
    CCTK_INT (*copy) (CCTK_POINTER_TO_CONST cctkGH,
                      CCTK_INT const *vars,
                      CCTK_INT const *tls,
                      CCTK_INT nvars,
                      CCTK_INT *moved);
    if (is_opencl) {
      dst_valid = &mem_t::device_valid;
      src_valid = &mem_t::host_valid;
      copy = Device_CopyToDevice;
    } else {
      dst_valid = &mem_t::host_valid;
      src_valid = &mem_t::device_valid;
      copy = Device_CopyToHost;
    }
    
    vars_t vars;
    for (int n=0; n<attribute->n_ReadsClauses; ++n) {
      int const gi = CCTK_GroupIndex(attribute->ReadsClauses[n]);
      assert(gi>=0);
      int const nv = CCTK_NumVarsInGroupI(gi);
      assert(nv>=0);
      if (nv > 0) {
        int const v0 = CCTK_FirstVarIndexI(gi);
        assert(v0>=0);
        for (int vi=v0; vi<v0+nv; ++vi) {
          int const tl=0;       // only copy current timelevel
          
          if (int(device->mems.at(vi).size()) <= tl) {
            device->mems.at(vi).resize(tl+1);
            // We see this variable for the first time -- we assume it
            // is valid on the host
            device->mems.at(vi).at(tl).host_valid = true;
            device->mems.at(vi).at(tl).device_valid = false;
          }
          
          if (not (device->mems.at(vi).at(tl).*dst_valid)) {
            // TODO: Check this. For now, we just assume this is true,
            // because we don't assume that all provides/requires
            // information is complete and correct.
            // assert(device->mems.at(vi).at(tl).*src_valid);
            device->mems.at(vi).at(tl).*src_valid = true;
            
            vars.push_back(vi, tl);
            device->mems.at(vi).at(tl).*dst_valid = true;
          }
        }
      }
    }
    
    if (veryverbose) {
      CCTK_VInfo(CCTK_THORNSTRING, "Copying in");
      cout << "[" << attribute->where << "] "
           << attribute->thorn << "::" << attribute->routine << "\n";
      cout << "   Copy in:";
      for (int n=0; n<vars.nvars(); ++n) {
        char *const fullname = CCTK_FullName(vars.vi_ptr()[n]);
        cout << " " << fullname << "/" << vars.tl_ptr()[n];
        free(fullname);
      }
      cout << "\n";
    }
    
    CCTK_INT moved;
    copy(cctkGH, vars.vi_ptr(), vars.tl_ptr(), vars.nvars(), &moved);
    
    // If the data were moved (instead of copied), mark them as
    // invalid on the source
    if (moved) {
      for (int var=0; var<vars.nvars(); ++var) {
        int const vi = vars.vi_ptr()[var];
        int const tl = vars.tl_ptr()[var];
        device->mems.at(vi).at(tl).*src_valid = false;
      }
    }
    
    return 0;
  }

  
  
  extern "C"
  CCTK_INT
  AcceleratorThorn_PostCallFunction(CCTK_POINTER_TO_CONST const cctkGH_,
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
      
      cout << "   Reads:";
      for (int n=0; n<attribute->n_ReadsClauses; ++n) {
        cout << " " << attribute->ReadsClauses[n];
      }
      cout << "\n";
      
      cout << "   Writes:";
      for (int n=0; n<attribute->n_WritesClauses; ++n) {
        cout << " " << attribute->WritesClauses[n];
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
    
    for (int n=0; n<attribute->n_WritesClauses; ++n) {
      int const gi = CCTK_GroupIndex(attribute->WritesClauses[n]);
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
      
      vars_t vars;
      for (int n=0; n<attribute->n_WritesClauses; ++n) {
        int const gi = CCTK_GroupIndex(attribute->WritesClauses[n]);
        assert(gi>=0);
        int const nv = CCTK_NumVarsInGroupI(gi);
        assert(nv>=0);
        if (nv > 0) {
          int const v0 = CCTK_FirstVarIndexI(gi);
          assert(v0>=0);
          for (int vi=v0; vi<v0+nv; ++vi) {
            int const tl=0;       // only copy current timelevel
            if (int(device->mems.at(vi).size()) > tl) {
              vars.push_back(vi, tl);
            }
          }
        }
      }
      
      if (veryverbose) {
        CCTK_VInfo(CCTK_THORNSTRING, "Copying out");
        cout << "[" << attribute->where << "] "
             << attribute->thorn << "::" << attribute->routine << "\n";
        cout << "   Copy in:";
        for (int n=0; n<vars.nvars(); ++n) {
          char *const fullname = CCTK_FullName(vars.vi_ptr()[n]);
          cout << " " << fullname << "/" << vars.tl_ptr()[n];
          free(fullname);
        }
        cout << "\n";
      }
      
      CCTK_INT moved;
      Device_CopyToHost
        (cctkGH, vars.vi_ptr(), vars.tl_ptr(), vars.nvars(), &moved);
      
      // If the data were moved (instead of copied), mark them as
      // invalid on the device
      if (moved) {
        for (int var=0; var<vars.nvars(); ++var) {
          int const vi = vars.vi_ptr()[var];
          int const tl = vars.tl_ptr()[var];
          device->mems.at(vi).at(tl).device_valid = false;
        }
      }
      
    }
    
    return 0;
  }
  
  
  
  extern "C"
  CCTK_INT
  AcceleratorThorn_PreSync(CCTK_POINTER_TO_CONST const cctkGH_,
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
    
    vars_t vars;
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
          vars.push_back(vi, tl);
        }
      }
    }
    
    BEGIN_LOCAL_MAP_LOOP(cctkGH, CCTK_GF) {
      BEGIN_LOCAL_COMPONENT_LOOP(cctkGH, CCTK_GF) {
        
        Device_CopyPreSync(cctkGH, vars.vi_ptr(), vars.tl_ptr(), vars.nvars());
        
      } END_LOCAL_COMPONENT_LOOP;
    } END_LOCAL_MAP_LOOP;
    
    return 0;
  }
  
  
  
  extern "C"
  CCTK_INT
  AcceleratorThorn_PostSync(CCTK_POINTER_TO_CONST const cctkGH_,
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
    
    vars_t vars;
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
          vars.push_back(vi, tl);
        }
      }
    }
    
    BEGIN_LOCAL_MAP_LOOP(cctkGH, CCTK_GF) {
      BEGIN_LOCAL_COMPONENT_LOOP(cctkGH, CCTK_GF) {
        
        Device_CopyPostSync(cctkGH, vars.vi_ptr(), vars.tl_ptr(), vars.nvars());
        
      } END_LOCAL_COMPONENT_LOOP;
    } END_LOCAL_MAP_LOOP;
    
    return 0;
  }
  
  
  
  extern "C"
  CCTK_INT
  AcceleratorThorn_CopyToDevice(CCTK_POINTER_TO_CONST const cctkGH_,
                                CCTK_INT const vis[],
                                CCTK_INT const tls[],
                                CCTK_INT const nvars)
  {
    cGH const *restrict const cctkGH = static_cast<cGH const*>(cctkGH_);
    DECLARE_CCTK_PARAMETERS;
    
    if (veryverbose) {
      stringstream buf;
      for (int var=0; var<nvars; ++var) {
        int const vi = vis[var];
        int const tl = tls[var];
        char *const fullname = CCTK_FullName(vi);
        buf << " " << fullname << "[" << tl << "]";
        free(fullname);
      }
      CCTK_VInfo(CCTK_THORNSTRING, "CopyToDevice%s", buf.str().c_str());
    }
    
    for (int var=0; var<nvars; ++var) {
      int const vi = vis[var];
      int const tl = tls[var];
      
      if (int(device->mems.at(vi).size()) <= tl) {
        device->mems.at(vi).resize(tl+1);
        // We see this variable for the first time -- we assume it is
        // valid on the host
        device->mems.at(vi).at(tl).host_valid = true;
        device->mems.at(vi).at(tl).device_valid = false;
      }
      
      if (not device->mems.at(vi).at(tl).device_valid) {
        // TODO: Check this. For now, we just assume this is true,
        // because we don't assume that all provides/requires
        // information is complete and correct.
        device->mems.at(vi).at(tl).host_valid = true;
        
        device->mems.at(vi).at(tl).device_valid = true;
      }
    }
    
    CCTK_INT moved;
    Device_CopyToDevice(cctkGH, vis, tls, nvars, &moved);
    
    // If the data were moved (instead of copied), mark them as
    // invalid on the host
    if (moved) {
      for (int var=0; var<nvars; ++var) {
        int const vi = vis[var];
        int const tl = tls[var];
        device->mems.at(vi).at(tl).host_valid = false;
      }
    }
    
    return 0;
  }
  
  
  
  //////////////////////////////////////////////////////////////////////////////
  
  
  
  extern "C"
  void Accelerator_CopyBack(CCTK_ARGUMENTS)
  {
    DECLARE_CCTK_ARGUMENTS;
    DECLARE_CCTK_PARAMETERS;
    
    if (veryverbose) {
      CCTK_VInfo(CCTK_THORNSTRING, "CopyBack");
    }
    
    vars_t vars;
    for (int vi=0; vi<int(device->mems.size()); ++vi) {
      int const tl=0;           // only copy current timelevel
      if (int(device->mems.at(vi).size()) > tl) {
        if (not device->mems.at(vi).at(tl).host_valid) {
          vars.push_back(vi, tl);
          device->mems.at(vi).at(tl).host_valid = true;
        }
      }
    }
    
    CCTK_INT moved;
    Device_CopyToHost
      (cctkGH, vars.vi_ptr(), vars.tl_ptr(), vars.nvars(), &moved);
    
    // If the data were moved (instead of copied), mark them as
    // invalid on the device
    if (moved) {
      for (int var=0; var<vars.nvars(); ++var) {
        int const vi = vars.vi_ptr()[var];
        int const tl = vars.tl_ptr()[var];
        device->mems.at(vi).at(tl).device_valid = false;
      }
    }
    
  }
     
} // namespace Accelerator
