#include "copy.hh"
#include "device.hh"

#include <cctk_Parameters.h>
#include <util_Table.h>

#include <carpet.hh>

#include <cassert>

namespace Accelerator {
  
  void copy_to_device(cGH const *restrict const cctkGH,
                      vector<var_t> const& vars)
  {
    for (size_t var=0; var<vars.size(); ++var) {
      Accelerator_CopyToDevice(cctkGH, vars[var].vi, vars[var].tl);
    }
  }
  
  void copy_to_host(cGH const *restrict const cctkGH,
                    vector<var_t> const& vars)
  {
    for (size_t var=0; var<vars.size(); ++var) {
      Accelerator_CopyToHost(cctkGH, vars[var].vi, vars[var].tl);
    }
  }
  
  //////////////////////////////////////////////////////////////////////////////
  
  
  
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
//    if (not device) return 0;
    // ICH: Maybe we need an aliased function to determine if the
    // device has been initialised?
    
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
//    if (not device) return 0;
    
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
      
      vector<var_t> vars;
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
     
} // namespace Accelerator
