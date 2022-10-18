#include "accelerator.hh"

#include <carpet.hh>

#include <cctk.h>
#include <cctk_Arguments.h>
#include <cctk_Parameters.h>
#include <util_Table.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

using namespace std;



namespace Accelerator {
  
  device_t *device = NULL;
  
  
  
  //////////////////////////////////////////////////////////////////////////////
  
  

  // Given the |name| of a variable or group, add the corresponding
  // variable indices to |vars|
  void vars_from_rw_name(const char *name1, vector<CCTK_INT> &vars)
  {
    // Remove trailing "[...]" modifier, if any
    char *const name = strdup(name1);
    char *const p = strchr(name, '(');
    if (p) *p = '\0';
    int const gi = CCTK_GroupIndex(name);
    if (gi >= 0) {
      // A group
      int const v0 = CCTK_FirstVarIndexI(gi); assert(v0 >= 0);
      int const nv = CCTK_NumVarsInGroupI(gi); assert(nv >= 0);
      for (int v = v0; v < v0+nv; ++v) {
        vars.push_back(v);
      }
    } else {
      // Not a group - should be a variable
      const int v = CCTK_VarIndex(name);
      assert(v >= 0);
      vars.push_back(v);
    }
    free(name);
  }

  // Given a schedule |attribute| pointer, add the variable indices of
  // all read variables to |vars|
  void get_read_variables(cFunctionData const *restrict const attribute,
                          vector<CCTK_INT> &vars)
  {
    for (int n=0; n<attribute->n_ReadsClauses; ++n) {
      vars_from_rw_name(attribute->ReadsClauses[n], vars);
    }
  }

  // Given a schedule |attribute| pointer, add the variable indices of
  // all written variables to |vars|
  void get_written_variables(cFunctionData const *restrict const attribute,
                             vector<CCTK_INT> &vars)
  {
    for (int n=0; n<attribute->n_WritesClauses; ++n) {
      vars_from_rw_name(attribute->WritesClauses[n], vars);
    }
  }
  
  
  
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
    
    int const rl = GetRefinementLevel(cctkGH);
    assert(rl >= 0);
    
    vars_t vars;
    for (int vi=0; vi<CCTK_NumVars(); ++vi) {
      
      // Variables without storage or with only a single timelevels
      // are not cycled
      int const cactus_tl = CCTK_ActiveTimeLevelsVI(cctkGH, vi);
      if (cactus_tl <= 1) continue;
      
      // NOTE: We do not introduce new timelevels while cycling. If
      // the device needs a timelevel that has become valid through
      // cycling, then this timelevel will be copied from the host.
      // (This can happen only before the first iteration.) This
      // requires that the host has valid data before the first
      // iteration. (This condition is checked below.)
      
      int const num_tl = device->ntimelevels(vi, rl);
      for (int tl=num_tl-1; tl>0; --tl) {
        
        // Cycle those timelevels that are valid on the device
        if (device->mem(vi, rl, tl-1).device_valid) {
          vars.push_back(vi, rl, tl);
        }
        
        device->mem(vi, rl, tl).device_valid =
          device->mem(vi, rl, tl-1).device_valid;
        // Cycle host information here as well
        device->mem(vi, rl, tl).host_valid =
          device->mem(vi, rl, tl-1).host_valid;
      }
      if (num_tl>1) {
        // Current timelevel is now invalid
      	device->mem(vi, rl, 0).device_valid = false;
      	// Cycle host information here as well
      	device->mem(vi, rl, 0).host_valid = false;
      }
    }
    
    if (CCTK_IsFunctionAliased("MultiPatch_GetSystemSpecification")) {
      CCTK_INT maps;
      MultiPatch_GetSystemSpecification(&maps);
      assert(maps==1);
    }
    BEGIN_LOCAL_MAP_LOOP(cctkGH, CCTK_GF) {
      int const nlcs = GetLocalComponents(cctkGH);
      assert(nlcs == 1);
      BEGIN_LOCAL_COMPONENT_LOOP(cctkGH, CCTK_GF) {
        
        Device_CopyCycle(cctkGH,
                         vars.vi_ptr(), vars.rl_ptr(), vars.tl_ptr(),
                         vars.nvars());
        
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
      CCTK_VInfo(CCTK_THORNSTRING, "CopyFromPast");
    }
    
    int const rl = GetRefinementLevel(cctkGH);
    assert(rl >= 0);
    
    vars_t vars, unknowns;
    for (int var=0; var<nvars; ++var) {
      int const vi = vis[var];
      int const tl = 0;
      
      assert(device->ntimelevels(vi, rl) >= tl);
      if (device->ntimelevels(vi, rl) == tl) {
        // We assume the host does not have valid data (otherwise, why
        // would the code copy to the current timelevel?)
        mem_t const newmem = {false, false};
        device->create_timelevel(vi, rl, tl, newmem);
        unknowns.push_back(vi, rl, tl);
      }
      
      if (device->ntimelevels(vi, rl) == tl+1) {
        // We assume the host has valid data (otherwise, why would the
        // code copy from the past timelevel?)
        mem_t const newmem = {true, false};
        device->create_timelevel(vi, rl, tl+1, newmem);
        unknowns.push_back(vi, rl, tl+1);
      }
      
      // Here we have a choice: Either we assume that the host copies
      // as well if it has valid data (then we need to update the
      // host's information), or we explicitly copy the host's data to
      // the device.
      
#if 0
      // We don't know which of these we want to do, so we hope for
      // the best (and check for it!):
      assert(device->mem(vi, rl, tl+1).device_valid);
#endif
      
      if (device->mem(vi, rl, tl+1).device_valid) {
        vars.push_back(vi, rl, tl);
        device->mem(vi, rl, tl).device_valid =
          device->mem(vi, rl, tl+1).device_valid;
      }      
    }
    
    Device_CreateVariables
      (cctkGH, unknowns.vi_ptr(), unknowns.rl_ptr(), unknowns.tl_ptr(),
       unknowns.nvars());
    Device_CopyFromPast
      (cctkGH, vars.vi_ptr(), vars.rl_ptr(), vars.tl_ptr(), vars.nvars());
    
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
    // the device setup routine is called via CallFunction.) This
    // happens only early during startup, long before Cactus variables
    // exist.
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
    
    // Is this a device routine?
    CCTK_INT is_device;
    int const ierr = Util_TableGetInt(attribute->tags, &is_device, "Device");
    if (ierr == UTIL_ERROR_TABLE_NO_SUCH_KEY) {
      is_device = 0;            // default
    } else if (ierr <= 0) {
      CCTK_WARN (CCTK_WARN_ABORT, "Error with schedule tag \"Device\"");
    }
    if (CCTK_IsFunctionAliased("Device_GetDevice") &&
        Device_GetDevice(cctkGH) == -1)
    {
      is_device = 0;
    }
    
    // Copy all required variables to the device or to the host,
    // depending on the language (device or not)
    bool mem_t::*dst_valid, mem_t::*src_valid;
    CCTK_INT (*copy) (CCTK_POINTER_TO_CONST cctkGH,
                      CCTK_INT const *vars,
                      CCTK_INT const *rls,
                      CCTK_INT const *tls,
                      CCTK_INT nvars,
                      CCTK_INT *moved);
    if (is_device) {
      dst_valid = &mem_t::device_valid;
      src_valid = &mem_t::host_valid;
      copy = Device_CopyToDevice;
    } else {
      dst_valid = &mem_t::host_valid;
      src_valid = &mem_t::device_valid;
      copy = Device_CopyToHost;
    }
    
    int const rl = GetRefinementLevel(cctkGH);
    assert(rl >= 0);
    
    vars_t vars, unknowns;

    vector<CCTK_INT> vars_read;
    get_read_variables(attribute,vars_read);

    for (vector<CCTK_INT>::iterator iter = vars_read.begin();
         iter != vars_read.end(); ++iter) {
      int const vi = *iter;
      assert(vi >= 0);
      int const gi = CCTK_GroupIndexFromVarI(vi);
      assert(gi >= 0);

      { // Maintain indentation
        int const cactus_tl = CCTK_ActiveTimeLevelsGI(cctkGH, gi);
        int const num_tl = only_reads_current_timelevel ? 1 : cactus_tl;
        assert(num_tl <= cactus_tl);
        
        { // Maintain indentation
          for (int tl=0; tl<num_tl; ++tl) {
            
            assert(tl <= device->ntimelevels(vi, rl));
            if (device->ntimelevels(vi, rl) == tl) {
              // We see this variable for the first time here, which
              // means that the function which generated it does not
              // declare the variable in WRITES (otherwise we would
              // have seen it in PostCall). This must be a host
              // function, as all device functions presumably have
              // valid WRITES lists. Therefore we assume the variable
              // is valid on the host.
              mem_t const newmem = {true, false};
              device->create_timelevel(vi, rl, tl, newmem);
              unknowns.push_back(vi, rl, tl);
            }
            
            if (not (device->mem(vi, rl, tl).*dst_valid)) {
              if (not (device->mem(vi, rl, tl).*src_valid)) {
                CCTK_VWarn(CCTK_WARN_ABORT, __LINE__, __FILE__, CCTK_THORNSTRING,
                           "Variable %s:%d is neither valid on the host nor on the device before function %s::%s",
                           CCTK_FullName(vi), tl,attribute->thorn,attribute->routine);
              }
              vars.push_back(vi, rl, tl);
              // This will be true after the copy operation below
              device->mem(vi, rl, tl).*dst_valid = true;
            }
            
          }
        }
      }
    }
    
    vector<CCTK_INT> vars_written;
    get_written_variables(attribute,vars_written);

    for (vector<CCTK_INT>::iterator iter = vars_written.begin();
         iter != vars_written.end(); ++iter) {
      int const vi = *iter;
      assert(vi >= 0);
      int const gi = CCTK_GroupIndexFromVarI(vi);
      assert(gi >= 0);

      { // Maintain indentation
        int const cactus_tl = CCTK_ActiveTimeLevelsGI(cctkGH, gi);
        int const num_tl = only_writes_current_timelevel ? 1 : cactus_tl;
        assert(num_tl <= cactus_tl);
        
        { // Maintain indentation
          for (int tl=0; tl<num_tl; ++tl) {
            
            assert(tl <= device->ntimelevels(vi, rl));
            if (device->ntimelevels(vi, rl) == tl) {
              // We see this variable for the first time here, and it
              // is not read by this function (otherwise it would have
              // been generated in the loop above). We can therefore
              // safely assume that this variable is undefined.
              mem_t const newmem = {false, false};
              device->create_timelevel(vi, rl, tl, newmem);
              unknowns.push_back(vi, rl, tl);
            }
            
            // This variable was written by the function which will
            // execute, therefore it will be valid there and invalid
            // elsewhere.
            device->mem(vi, rl, tl).*dst_valid = true;
            device->mem(vi, rl, tl).*src_valid = false;
            
          }
        }
      }
    }
    
    Device_CreateVariables
      (cctkGH, unknowns.vi_ptr(), unknowns.rl_ptr(), unknowns.tl_ptr(),
       unknowns.nvars());
    
    if (veryverbose) {
      CCTK_VInfo(CCTK_THORNSTRING, "Copying in");
      cout << "[" << attribute->where << "] "
           << attribute->thorn << "::" << attribute->routine << "\n";
      cout << "   Copy in:";
      for (int n=0; n<vars.nvars(); ++n) {
        char *const fullname = CCTK_FullName(vars.vi_ptr()[n]);
        cout << " " << fullname
             << "[rl=" << vars.rl_ptr()[n] << ",tl=" << vars.tl_ptr()[n] << "]";
        free(fullname);
      }
      cout << "\n";
    }
    
    CCTK_INT moved;
    copy(cctkGH, vars.vi_ptr(), vars.rl_ptr(), vars.tl_ptr(), vars.nvars(),
         &moved);
    
    // If the data were moved (instead of copied), mark them as
    // invalid on the source
    if (moved) {
      for (int var=0; var<vars.nvars(); ++var) {
        int const vi = vars.vi_ptr()[var];
        int const rl = vars.rl_ptr()[var];
        int const tl = vars.tl_ptr()[var];
        device->mem(vi, rl, tl).*src_valid = false;
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
    // the device setup routine is called via CallFunction.) This
    // happens only early during startup, long before Cactus variables
    // exist.
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
    
    // Is this a device routine?
    CCTK_INT is_device;
    int const ierr = Util_TableGetInt(attribute->tags, &is_device, "Device");
    if (ierr == UTIL_ERROR_TABLE_NO_SUCH_KEY) {
      is_device = 0;            // default
    } else if (ierr <= 0) {
      CCTK_WARN (CCTK_WARN_ABORT, "Error with schedule tag \"Device\"");
    }
    
    // If we are in the analysis bin, and if this is a device
    // routine, then copy back all provided variables since they may
    // be output. Otherwise, do nothing.
    // TODO: Add a hook to the flesh to do this only when an I/O
    // method has been called, and then copy only those variables
    // necessary.
    if (Carpet::in_analysis_bin and
        is_device and copy_back_all_written_variables_in_analysis)
    {
      int const rl = GetRefinementLevel(cctkGH);
      assert(rl >= 0);
      
      vars_t vars;
      vector<CCTK_INT> vars_written;
      get_written_variables(attribute,vars_written);

      for (vector<CCTK_INT>::iterator iter = vars_written.begin();
           iter != vars_written.end(); ++iter) {
        int const vi = *iter;
        assert(vi >= 0);
        int const gi = CCTK_GroupIndexFromVarI(vi);
        assert(gi >= 0);

        { // Maintain indentation
          int const cactus_tl = CCTK_ActiveTimeLevelsGI(cctkGH, gi);
          int const num_tl = copy_back_all_timelevels ? 1 : cactus_tl;
          assert(num_tl <= cactus_tl);
          
          { // Maintain indentation
            for (int tl=0; tl<num_tl; ++tl) {
              if (device->ntimelevels(vi, rl) > tl) {
                vars.push_back(vi, rl, tl);
              }
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
          cout << " " << fullname
               << "[rl=" << vars.rl_ptr()[n] << ",tl=" << vars.tl_ptr()[n]
               << "]";
          free(fullname);
        }
        cout << "\n";
      }
      
      CCTK_INT moved;
      Device_CopyToHost
        (cctkGH, vars.vi_ptr(), vars.rl_ptr(), vars.tl_ptr(), vars.nvars(),
         &moved);
      
      // If the data were moved (instead of copied), mark them as
      // invalid on the device
      if (moved) {
        for (int var=0; var<vars.nvars(); ++var) {
          int const vi = vars.vi_ptr()[var];
          int const tl = vars.tl_ptr()[var];
          int const rl = vars.rl_ptr()[var];
          device->mem(vi, rl, tl).device_valid = false;
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
    
    int const rl = GetRefinementLevel(cctkGH);
    assert(rl >= 0);
    
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
          if (!device->mem(vi, rl, tl).host_valid) {
            assert(device->mem(vi, rl, tl).device_valid);
            vars.push_back(vi, rl, tl);
          }
        }
      }
    }
    
    if (CCTK_IsFunctionAliased("MultiPatch_GetSystemSpecification")) {
      CCTK_INT maps;
      MultiPatch_GetSystemSpecification(&maps);
      assert(maps==1);
    }
    BEGIN_LOCAL_MAP_LOOP(cctkGH, CCTK_GF) {
      int const nlcs = GetLocalComponents(cctkGH);
      assert(nlcs == 1);
      BEGIN_LOCAL_COMPONENT_LOOP(cctkGH, CCTK_GF) {
        
        Device_CopyPreSync
          (cctkGH, vars.vi_ptr(), vars.rl_ptr(), vars.tl_ptr(), vars.nvars());
        
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
    
    int const rl = GetRefinementLevel(cctkGH);
    assert(rl >= 0);
    
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
          if (device->mem(vi, rl, tl).device_valid) {
            vars.push_back(vi, rl, tl);
          }
        }
      }
    }
    
    if (CCTK_IsFunctionAliased("MultiPatch_GetSystemSpecification")) {
      CCTK_INT maps;
      MultiPatch_GetSystemSpecification(&maps);
      assert(maps==1);
    }
    BEGIN_LOCAL_MAP_LOOP(cctkGH, CCTK_GF) {
      int const nlcs = GetLocalComponents(cctkGH);
      assert(nlcs == 1);
      BEGIN_LOCAL_COMPONENT_LOOP(cctkGH, CCTK_GF) {
        
        Device_CopyPostSync
          (cctkGH, vars.vi_ptr(), vars.rl_ptr(), vars.tl_ptr(), vars.nvars());
        
      } END_LOCAL_COMPONENT_LOOP;
    } END_LOCAL_MAP_LOOP;
    
    return 0;
  }
  
  
  
  extern "C"
  CCTK_INT
  AcceleratorThorn_NotifyDataModified(CCTK_POINTER_TO_CONST const cctkGH_,
                                      CCTK_INT const variables[],
                                      CCTK_INT const rls[],
                                      CCTK_INT const tls[],
                                      CCTK_INT const nvariables,
                                      CCTK_INT const on_device)
  {
    assert(cctkGH_);
    cGH const *restrict const cctkGH CCTK_ATTRIBUTE_UNUSED =
      static_cast<cGH const*>(cctkGH_);
    DECLARE_CCTK_PARAMETERS;
    
    if (veryverbose) {
      CCTK_VInfo(CCTK_THORNSTRING, "NotifyDataModified");
    }
    
    bool mem_t::*valid, mem_t::*invalid;
    if (on_device) {
      valid   = &mem_t::device_valid;
      invalid = &mem_t::host_valid;
    } else {
      valid   = &mem_t::host_valid;
      invalid = &mem_t::device_valid;
    }
    
    for (int var=0; var<nvariables; ++var) {
      int const vi = variables[var];
      int const rl = rls[var];
      int const tl = tls[var];
      device->mem(vi, rl, tl).*valid   = true;
      device->mem(vi, rl, tl).*invalid = false;
    }
    
    return 0;
  }
  

  
  extern "C"
  CCTK_INT
  AcceleratorThorn_RequireInvalidData(CCTK_POINTER_TO_CONST const cctkGH_,
                                      CCTK_INT const variables[],
                                      CCTK_INT const rls[],
                                      CCTK_INT const tls[],
                                      CCTK_INT const nvariables,
                                      CCTK_INT const on_device)
  {
    assert(cctkGH_);
    cGH const *restrict const cctkGH CCTK_ATTRIBUTE_UNUSED =
      static_cast<cGH const*>(cctkGH_);
    DECLARE_CCTK_PARAMETERS;
    
    if (veryverbose) {
      CCTK_VInfo(CCTK_THORNSTRING, "RequireInvalidData");
    }
    
    // Ensure the variable exists on the device or on the host,
    // depending on the language (device or not)
    
    vars_t unknowns;
    for (int var=0; var<nvariables; ++var) {
      int const vi = variables[var];
      int const rl = rls[var];
      int const tl = tls[var];
      
      assert(tl <= device->ntimelevels(vi, rl));
      if (device->ntimelevels(vi, rl) == tl) {
        // We see this variable for the first time here, and the
        // caller does not expect valid data. We therefore assume the
        // variable is invalid everywhere.
        mem_t const newmem = {false, false};
        device->create_timelevel(vi, rl, tl, newmem);
        unknowns.push_back(vi, rl, tl);
      }
    }
    
    Device_CreateVariables
      (cctkGH, unknowns.vi_ptr(), unknowns.rl_ptr(), unknowns.tl_ptr(),
       unknowns.nvars());
    
    return 0;
  }
  
  
  
  extern "C"
  CCTK_INT
  AcceleratorThorn_RequireValidData(CCTK_POINTER_TO_CONST const cctkGH_,
                                    CCTK_INT const variables[],
                                    CCTK_INT const rls[],
                                    CCTK_INT const tls[],
                                    CCTK_INT const nvariables,
                                    CCTK_INT const on_device)
  {
    assert(cctkGH_);
    cGH const *restrict const cctkGH CCTK_ATTRIBUTE_UNUSED =
      static_cast<cGH const*>(cctkGH_);
    DECLARE_CCTK_PARAMETERS;
    
    if (veryverbose) {
      CCTK_VInfo(CCTK_THORNSTRING, "RequireValidData");
    }
    
    // Copy all required variables to the device or to the host,
    // depending on the language (device or not)
    bool mem_t::*dst_valid, mem_t::*src_valid;
    CCTK_INT (*copy) (CCTK_POINTER_TO_CONST cctkGH,
                      CCTK_INT const *vars,
                      CCTK_INT const *rls,
                      CCTK_INT const *tls,
                      CCTK_INT nvars,
                      CCTK_INT *moved);
    if (on_device) {
      dst_valid = &mem_t::device_valid;
      src_valid = &mem_t::host_valid;
      copy = Device_CopyToDevice;
    } else {
      dst_valid = &mem_t::host_valid;
      src_valid = &mem_t::device_valid;
      copy = Device_CopyToHost;
    }
    
    vars_t vars, unknowns;
    for (int var=0; var<nvariables; ++var) {
      int const vi = variables[var];
      int const rl = rls[var];
      int const tl = tls[var];
      
      int const num_tl = CCTK_ActiveTimeLevelsVI(cctkGH, vi);
      assert(tl < num_tl);
      
      assert(tl <= device->ntimelevels(vi, rl));
      if (device->ntimelevels(vi, rl) == tl) {
        // We see this variable for the first time here, which means
        // that the function which generated it does not declare the
        // variable in WRITES (otherwise we would have seen it in
        // PostCall). This must be a host function, as all device
        // functions presumably have valid WRITES lists. Therefore we
        // assume the variable is valid on the host.
        mem_t const newmem = {true, false};
        device->create_timelevel(vi, rl, tl, newmem);
        unknowns.push_back(vi, rl, tl);
      }
      
      if (not (device->mem(vi, rl, tl).*dst_valid)) {
        assert(device->mem(vi, rl, tl).*src_valid);
        vars.push_back(vi, rl, tl);
      }
    }
    
    Device_CreateVariables
      (cctkGH, unknowns.vi_ptr(), unknowns.rl_ptr(), unknowns.tl_ptr(),
       unknowns.nvars());
    CCTK_INT moved;
    copy(cctkGH, vars.vi_ptr(), vars.rl_ptr(), vars.tl_ptr(), vars.nvars(),
         &moved);
    
    // If the data were moved (instead of copied), mark them as
    // invalid on the source
    if (moved) {
      for (int var=0; var<vars.nvars(); ++var) {
        int const vi = vars.vi_ptr()[var];
        int const rl = vars.rl_ptr()[var];
        int const tl = vars.tl_ptr()[var];
        device->mem(vi, rl, tl).*src_valid = false;
      }
    }
    
    return 0;
  }
  
  
  
  //////////////////////////////////////////////////////////////////////////////
  
  
  
  struct mark_var_t {
    cGH const *cctkGH;
    vars_t vars;
  };
  
  static
  void mark_var(int const vi, char const *const optstring, void *const args_)
  {
    mark_var_t& args = *(mark_var_t*)args_;
    cGH const *const cctkGH = args.cctkGH;
    DECLARE_CCTK_ARGUMENTS;
    DECLARE_CCTK_PARAMETERS;
    
    int const cactus_tl = CCTK_ActiveTimeLevelsVI(cctkGH, vi);
    int const num_tl = copy_back_all_timelevels ? 1 : cactus_tl;
    assert(num_tl <= cactus_tl);
    
    int const rl = GetRefinementLevel(cctkGH);
    assert(rl >= 0);
    
    vars_t& vars = args.vars;
    for (int tl=0; tl<num_tl; ++tl) {
      if (tl < device->ntimelevels(vi, rl)) {
        if (not device->mem(vi, rl, tl).host_valid and
            device->mem(vi, rl, tl).device_valid)
        {
          vars.push_back(vi, rl, tl);
          device->mem(vi, rl, tl).host_valid = true;
        }
      }
    }
  }
  
  extern "C"
  void Accelerator_CopyBack(CCTK_ARGUMENTS)
  {
    DECLARE_CCTK_ARGUMENTS;
    DECLARE_CCTK_PARAMETERS;
    
    if (veryverbose) {
      CCTK_VInfo(CCTK_THORNSTRING, "CopyBack");
    }
    
    if (copy_back_every == 0) return;
    if (cctk_iteration % copy_back_every != 0) return;
    
    mark_var_t args;
    args.cctkGH = cctkGH;
    CCTK_TraverseString(copy_back_vars, mark_var, &args, CCTK_GROUP_OR_VAR);
    vars_t& vars = args.vars;
    
    CCTK_INT moved;
    Device_CopyToHost
      (cctkGH, vars.vi_ptr(), vars.rl_ptr(), vars.tl_ptr(), vars.nvars(),
       &moved);
    
    // If the data were moved (instead of copied), mark them as
    // invalid on the device
    if (moved) {
      for (int var=0; var<vars.nvars(); ++var) {
        int const vi = vars.vi_ptr()[var];
        int const rl = vars.rl_ptr()[var];
        int const tl = vars.tl_ptr()[var];
        device->mem(vi, rl, tl).device_valid = false;
      }
    }
    
  }
     
} // namespace Accelerator
