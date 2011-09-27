#include <cctk.h>
#include <cctk_Arguments.h>
#include <cctk_Parameters.h>

#include <carpet.hh>
#include <vectors.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef __APPLE__
#  include <OpenCL/opencl.h>
#else
#  include <CL/opencl.h>
#endif	

#include "OpenCLRunTime.h"

using namespace std;

#ifdef CCTK_CXX_RESTRICT
#  define restrict CCTK_CXX_RESTRICT
#endif



namespace OpenCLRunTime {
  
  
  
  char const *error_string(int const error_code)
  {
    switch (error_code) {
    case CL_SUCCESS                                  : return "CL_SUCCESS";
    case CL_DEVICE_NOT_FOUND                         : return "CL_DEVICE_NOT_FOUND";
    case CL_DEVICE_NOT_AVAILABLE                     : return "CL_DEVICE_NOT_AVAILABLE";
    case CL_COMPILER_NOT_AVAILABLE                   : return "CL_COMPILER_NOT_AVAILABLE";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE            : return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case CL_OUT_OF_RESOURCES                         : return "CL_OUT_OF_RESOURCES";
    case CL_OUT_OF_HOST_MEMORY                       : return "CL_OUT_OF_HOST_MEMORY";
    case CL_PROFILING_INFO_NOT_AVAILABLE             : return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case CL_MEM_COPY_OVERLAP                         : return "CL_MEM_COPY_OVERLAP";
    case CL_IMAGE_FORMAT_MISMATCH                    : return "CL_IMAGE_FORMAT_MISMATCH";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED               : return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case CL_BUILD_PROGRAM_FAILURE                    : return "CL_BUILD_PROGRAM_FAILURE";
    case CL_MAP_FAILURE                              : return "CL_MAP_FAILURE";
    case CL_MISALIGNED_SUB_BUFFER_OFFSET             : return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
    case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
    case CL_INVALID_VALUE                            : return "CL_INVALID_VALUE";
    case CL_INVALID_DEVICE_TYPE                      : return "CL_INVALID_DEVICE_TYPE";
    case CL_INVALID_PLATFORM                         : return "CL_INVALID_PLATFORM";
    case CL_INVALID_DEVICE                           : return "CL_INVALID_DEVICE";
    case CL_INVALID_CONTEXT                          : return "CL_INVALID_CONTEXT";
    case CL_INVALID_QUEUE_PROPERTIES                 : return "CL_INVALID_QUEUE_PROPERTIES";
    case CL_INVALID_COMMAND_QUEUE                    : return "CL_INVALID_COMMAND_QUEUE";
    case CL_INVALID_HOST_PTR                         : return "CL_INVALID_HOST_PTR";
    case CL_INVALID_MEM_OBJECT                       : return "CL_INVALID_MEM_OBJECT";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR          : return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case CL_INVALID_IMAGE_SIZE                       : return "CL_INVALID_IMAGE_SIZE";
    case CL_INVALID_SAMPLER                          : return "CL_INVALID_SAMPLER";
    case CL_INVALID_BINARY                           : return "CL_INVALID_BINARY";
    case CL_INVALID_BUILD_OPTIONS                    : return "CL_INVALID_BUILD_OPTIONS";
    case CL_INVALID_PROGRAM                          : return "CL_INVALID_PROGRAM";
    case CL_INVALID_PROGRAM_EXECUTABLE               : return "CL_INVALID_PROGRAM_EXECUTABLE";
    case CL_INVALID_KERNEL_NAME                      : return "CL_INVALID_KERNEL_NAME";
    case CL_INVALID_KERNEL_DEFINITION                : return "CL_INVALID_KERNEL_DEFINITION";
    case CL_INVALID_KERNEL                           : return "CL_INVALID_KERNEL";
    case CL_INVALID_ARG_INDEX                        : return "CL_INVALID_ARG_INDEX";
    case CL_INVALID_ARG_VALUE                        : return "CL_INVALID_ARG_VALUE";
    case CL_INVALID_ARG_SIZE                         : return "CL_INVALID_ARG_SIZE";
    case CL_INVALID_KERNEL_ARGS                      : return "CL_INVALID_KERNEL_ARGS";
    case CL_INVALID_WORK_DIMENSION                   : return "CL_INVALID_WORK_DIMENSION";
    case CL_INVALID_WORK_GROUP_SIZE                  : return "CL_INVALID_WORK_GROUP_SIZE";
    case CL_INVALID_WORK_ITEM_SIZE                   : return "CL_INVALID_WORK_ITEM_SIZE";
    case CL_INVALID_GLOBAL_OFFSET                    : return "CL_INVALID_GLOBAL_OFFSET";
    case CL_INVALID_EVENT_WAIT_LIST                  : return "CL_INVALID_EVENT_WAIT_LIST";
    case CL_INVALID_EVENT                            : return "CL_INVALID_EVENT";
    case CL_INVALID_OPERATION                        : return "CL_INVALID_OPERATION";
    case CL_INVALID_GL_OBJECT                        : return "CL_INVALID_GL_OBJECT";
    case CL_INVALID_BUFFER_SIZE                      : return "CL_INVALID_BUFFER_SIZE";
    case CL_INVALID_MIP_LEVEL                        : return "CL_INVALID_MIP_LEVEL";
    case CL_INVALID_GLOBAL_WORK_SIZE                 : return "CL_INVALID_GLOBAL_WORK_SIZE";
    case CL_INVALID_PROPERTY                         : return "CL_INVALID_PROPERTY";
    }
    return "unknown error";
  }
  
  
  
#define checkErr(cmd) checkErr1(cmd, #cmd, __FILE__, __LINE__)
  void checkErr1 (cl_int const errcode, char const* const cmd,
                  char const* const file, int const line);
  void checkErr1 (cl_int const errcode, char const* const cmd,
                  char const* const file, int const line)
  {
    if (errcode == CL_SUCCESS) return;
    CCTK_VWarn (CCTK_WARN_ABORT, line, file, CCTK_THORNSTRING,
                "%s\nError %d: %s",
                cmd, (int)errcode, error_string(errcode));
  }
  
#define checkWarn(cmd) checkWarn1(cmd, #cmd, __FILE__, __LINE__)
  void checkWarn1 (cl_int const errcode, char const* const cmd,
                   char const* const file, int const line);
  void checkWarn1 (cl_int const errcode, char const* const cmd,
                   char const* const file, int const line)
  {
    if (errcode == CL_SUCCESS) return;
    CCTK_VWarn (CCTK_WARN_ALERT, __LINE__, __FILE__, CCTK_THORNSTRING,
                "%s\nError %d: %s",
                cmd, (int)errcode, error_string(errcode));
  }
  
  
  
  // Divide while rounding up
  size_t divup (size_t const a, size_t const b);
  size_t divup (size_t const a, size_t const b)
  {
    return (a+b-1)/b;
  }
  
  
  
  //////////////////////////////////////////////////////////////////////////////
  
  
  
  // Number of dimensions
  int const dim =  3;
  
  
  
  // NOTE: Ensure that sizeof(cl_ptrdiff_t) <= CL_DEVICE_ADDRESS_BITS
#if 0
  // 32 bits
  typedef cl_uint cl_size_t;
  typedef cl_int  cl_ptrdiff_t;
#else
  // 64 bits
  typedef cl_ulong cl_size_t;
  typedef cl_long  cl_ptrdiff_t;
#endif
  
  
  
  // Equivalent of cGH for the kernel
  struct grid_t {
    // Doubles first, then ints, to ensure proper alignment
    // Coordinates:
    double origin_space[dim];
    double delta_space[dim];
    double time;
    double delta_time;
    // Grid structure properties:
    cl_ptrdiff_t gsh[dim];
    cl_ptrdiff_t lbnd[dim];
    cl_ptrdiff_t lssh[dim];
    cl_ptrdiff_t lsh[dim];
    // Loop settings:
    cl_ptrdiff_t lmin[dim];     // loop region
    cl_ptrdiff_t lmax[dim];
    cl_ptrdiff_t imin[dim];     // active region
    cl_ptrdiff_t imax[dim];
  };
  
  
  
  enum memory_model_t {
    mm_always_mapped, mm_copy, mm_map
  };
  
  // Global data, defining platform, device etc.
  struct OpenCLDevice {
    cl_device_type device_type;
    cl_context context;
    cl_device_id device_id;
    
    // point  (smallest unit)
    // vector (same execution path)
    // unroll (unrolled kernel loop)
    // group  (closely coupled threads, sharing cache, "CUDA thread block")
    // tile   (explicit kernel loop)
    // grid   (largest unit, loosely coupled threads, separate caches,
    //         "CUDA grid")
    memory_model_t memory_model;
    bool memory_aligned;
    cl_uint vector_size[dim];
    cl_uint unroll_size[dim];
    cl_uint group_size[dim];
    cl_uint tile_size[dim];
    grid_t grid;
    
    cl_command_queue queue;
    
    vector<vector<cl_mem> > mems; //  [vi][tl]
  };
  
  OpenCLDevice * device = NULL;
  
  
  
  struct OpenCLKernel {
    char const * name;
    cl_program program;
    struct arg_t {
      int vi, tl;
      string alias;
    };
    vector<arg_t> args;
    cl_kernel kernel;
    
    grid_t grid;
    cl_mem mem_grid;
    cl_mem mem_params;
  };
  
  
  
  extern "C"
  char const * OpenCLMacros_GetSource ();
  
  
  
  extern "C"
  void OpenCLRunTime_Setup (CCTK_ARGUMENTS)
  {
    DECLARE_CCTK_ARGUMENTS;
    DECLARE_CCTK_PARAMETERS;
    
    cl_int errcode;
    
    assert (cctkGH);
    
    CCTK_INFO ("Setting up OpenCL device");
    
    /*** Allocate memory for global information *******************************/
    
    assert (not device);
    device = new OpenCLDevice;
    
    /*** Choose a platform ****************************************************/
    
    cl_uint num_platforms;
    checkErr (clGetPlatformIDs(0, NULL, &num_platforms));
    cl_platform_id platforms[num_platforms];
    checkErr (clGetPlatformIDs(num_platforms, &platforms[0], &num_platforms));
    // Arbitrarily choose first platform
    assert (num_platforms > 0);
    cl_platform_id const platform = platforms[0];
    size_t platform_name_size;
    checkErr (clGetPlatformInfo(platform, CL_PLATFORM_NAME,
                                0, NULL, &platform_name_size));
    char platform_name[platform_name_size];
    checkErr (clGetPlatformInfo(platform, CL_PLATFORM_NAME,
                                platform_name_size, platform_name, NULL));
    CCTK_VInfo (CCTK_THORNSTRING,
                "Selected platform: %s", platform_name);
    
    /*** Choose a context (basically a device) ********************************/
    
    if (CCTK_EQUALS(opencl_device_type, "CPU")) {
      device->device_type = CL_DEVICE_TYPE_CPU;
    } else if (CCTK_EQUALS(opencl_device_type, "GPU")) {
      device->device_type = CL_DEVICE_TYPE_GPU;
    } else {
      CCTK_VWarn (CCTK_WARN_ALERT, __LINE__, __FILE__, CCTK_THORNSTRING,
                  "Unknown device type \"%s\" selected", opencl_device_type);
    }
    cl_context_properties const cprops[] =
      {CL_CONTEXT_PLATFORM, (cl_context_properties)platform,
       0};
    device->context =
      clCreateContextFromType (cprops, device->device_type, NULL, NULL,
                               &errcode);
    if (errcode != CL_SUCCESS) {
      CCTK_VWarn (CCTK_WARN_ABORT, __LINE__, __FILE__, CCTK_THORNSTRING,
                "Could not create OpenCL context for device type \"%s\"",
                  opencl_device_type);
    }
    size_t context_devices_size;
    checkErr (clGetContextInfo (device->context, CL_CONTEXT_DEVICES,
                                0, NULL, &context_devices_size));
    cl_uint const num_devices = context_devices_size / sizeof(cl_device_id);
    cl_device_id devices[num_devices];
    checkErr (clGetContextInfo (device->context, CL_CONTEXT_DEVICES,
                                context_devices_size, devices, NULL));
    // Arbitrarily choose first matching device
    assert (num_devices > 0);
    device->device_id = devices[0];
    size_t device_name_size;
    checkErr (clGetDeviceInfo(device->device_id, CL_DEVICE_NAME,
                              0, NULL, &device_name_size));
    char device_name[device_name_size];
    checkErr (clGetDeviceInfo(device->device_id, CL_DEVICE_NAME,
                              device_name_size, device_name, NULL));
    CCTK_VInfo (CCTK_THORNSTRING,
                "Selected device: %s", device_name);
    cl_device_type type;
    checkErr (clGetDeviceInfo(device->device_id, CL_DEVICE_TYPE,
                              sizeof type, &type, NULL));
    CCTK_VInfo (CCTK_THORNSTRING,
                "   Device type: %s",
                type == CL_DEVICE_TYPE_CPU         ? "CPU"        :
                type == CL_DEVICE_TYPE_GPU         ? "GPU"        :
                type == CL_DEVICE_TYPE_ACCELERATOR ? "ACCELERATOR":
                NULL);
    
    /*** Choose looping configuration *****************************************/
    
    // Memory model
    // TODO: compare with CL_DEVICE_HOST_UNIFIED_MEMORY
    if (CCTK_EQUALS(memory_model, "always-mapped")) {
      device->memory_model = mm_always_mapped;
    } else if (CCTK_EQUALS(memory_model, "copy")) {
      device->memory_model = mm_copy;
    } else if (CCTK_EQUALS(memory_model, "map")) {
      device->memory_model = mm_map;
    } else {
      CCTK_WARN (CCTK_WARN_ABORT, "internal error");
    }
    
    device->memory_aligned =
      device->memory_model == mm_copy or
      (vector_size_x == 1 and
       vector_size_y == 1 and
       vector_size_z == 1) or
      ((VECTORISE and VECTORISE_ALIGNED_ARRAYS) and
       vector_size_x <= CCTK_REAL_VEC_SIZE and
       vector_size_y == 1 and
       vector_size_z == 1);
    
    // Vector size
    device->vector_size[0] = vector_size_x;
    device->vector_size[1] = vector_size_y;
    device->vector_size[2] = vector_size_z;
    if (device->vector_size[0] == 0) {
      checkErr (clGetDeviceInfo (device->device_id,
                                 CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE,
                                 sizeof device->vector_size[0],
                                 device->vector_size, NULL));
    }
    if (device->vector_size[0] == 0) {
      // If double vectors are not supported, try long instead
      checkErr (clGetDeviceInfo (device->device_id,
                                 CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG,
                                 sizeof device->vector_size[0],
                                 device->vector_size, NULL));
    }
    if (device->vector_size[0] == 0) {
      CCTK_WARN (CCTK_WARN_ABORT, "Could not determine preferred vector size");
    }
    CCTK_VInfo (CCTK_THORNSTRING,
                "Vector size: %2d %2d %2d",
                device->vector_size[0],
                device->vector_size[1],
                device->vector_size[2]);
    
    // Unrolled loops
    device->unroll_size[0] = unroll_size_x;
    device->unroll_size[1] = unroll_size_y;
    device->unroll_size[2] = unroll_size_z;
    CCTK_VInfo (CCTK_THORNSTRING,
                "Unroll size: %2d %2d %2d",
                device->unroll_size[0],
                device->unroll_size[1],
                device->unroll_size[2]);
    
    // Closely coupled threads (aka OpenCL groups)
    device->group_size[0] = group_size_x;
    device->group_size[1] = group_size_y;
    device->group_size[2] = group_size_z;
    CCTK_VInfo (CCTK_THORNSTRING,
                "Group size:  %2d %2d %2d",
                device->group_size[0],
                device->group_size[1],
                device->group_size[2]);
    
    // Explicit kernel loops (aka loop tiling)
    device->tile_size[0] = tile_size_x;
    device->tile_size[1] = tile_size_y;
    device->tile_size[2] = tile_size_z;
    CCTK_VInfo (CCTK_THORNSTRING,
                "Tile size:   %2d %2d %2d",
                device->tile_size[0],
                device->tile_size[1],
                device->tile_size[2]);
    
    // Describe grid structure
    for (int d=0; d<dim; ++d) {
      device->grid.gsh[d] = cctkGH->cctk_gsh[d];
      device->grid.lbnd[d] = cctkGH->cctk_lbnd[d];
      device->grid.lssh[d] = cctkGH->CCTK_LSSH(0,d);
      int const granularity = device->vector_size[d];
      int const good_lsh = divup (cctk_lsh[d], granularity) * granularity;
      device->grid.lsh[d] = device->memory_aligned ? good_lsh : cctk_lsh[d];
      assert (device->grid.gsh[d] >= 0);
      assert (device->grid.lbnd[d] >= 0);
      assert (device->grid.lbnd[d] <= device->grid.gsh[d]);
      assert (device->grid.lssh[d] >= 0);
      assert (device->grid.lbnd[d] + device->grid.lssh[d] <=
              device->grid.gsh[d]);
      assert (device->grid.lsh[d] >= 0);
      assert (device->grid.lssh[d] <= device->grid.lsh[d]);
      device->grid.origin_space[d] = cctkGH->cctk_origin_space[d];
      device->grid.delta_space[d] = cctkGH->cctk_delta_space[d];
    }
    device->grid.time = cctkGH->cctk_time;
    device->grid.delta_time = cctkGH->cctk_delta_time;
    CCTK_VInfo (CCTK_THORNSTRING,
                "gsh:  %4d %4d %4d",
                (int)device->grid.gsh[0],
                (int)device->grid.gsh[1],
                (int)device->grid.gsh[2]);
    CCTK_VInfo (CCTK_THORNSTRING,
                "lbnd: %4d %4d %4d",
                (int)device->grid.lbnd[0],
                (int)device->grid.lbnd[1],
                (int)device->grid.lbnd[2]);
    CCTK_VInfo (CCTK_THORNSTRING,
                "lssh: %4d %4d %4d",
                (int)device->grid.lssh[0],
                (int)device->grid.lssh[1],
                (int)device->grid.lssh[2]);
    CCTK_VInfo (CCTK_THORNSTRING,
                "lsh:  %4d %4d %4d",
                (int)device->grid.lsh[0],
                (int)device->grid.lsh[1],
                (int)device->grid.lsh[2]);
    
    /*** Create execution queue ***********************************************/
    
    checkErr ((device->queue =
               clCreateCommandQueue (device->context, device->device_id,
                                     /*CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | */
                                     CL_QUEUE_PROFILING_ENABLE,
                                     &errcode),
               errcode));
    
    /*** Set up memory buffers ************************************************/
    
    device->mems.resize (CCTK_NumVars());
  }
  
  
  
  extern "C"
  CCTK_INT OpenCLRunTime_Cycle (CCTK_POINTER_TO_CONST const cctkGH_)
  {
    cGH const *restrict const cctkGH = static_cast<cGH const*>(cctkGH_);
    DECLARE_CCTK_PARAMETERS;
    
    if (veryverbose) {
      CCTK_VInfo (CCTK_THORNSTRING, "Cycle");
    }
    
    if (device->memory_model == mm_always_mapped) return 0;
    
#if 0
    
    // Could also copy buffers instead
    for (int vi=0; vi<CCTK_NumVars(); ++vi) {
      int const num_tl = device->mems.at(vi).size();
      if (num_tl > 1) {
        cl_mem const tmp = device->mems.at(vi).at(num_tl-1);
        for (int tl=num_tl-1; tl>0; --tl) {
          device->mems.at(vi).at(tl) = device->mems.at(vi).at(tl-1);
        }
        device->mems.at(vi)[0] = tmp;
      }
    }
    
#endif
    
    BEGIN_LOCAL_MAP_LOOP(cctkGH, CCTK_GF) {
      BEGIN_LOCAL_COMPONENT_LOOP(cctkGH, CCTK_GF) {
        
        int const NP =
          device->grid.lsh[0] * device->grid.lsh[1] * device->grid.lsh[2];
        
        for (int vi=0; vi<CCTK_NumVars(); ++vi) {
          int const num_tl = device->mems.at(vi).size();
          if (num_tl > 1) {
            cl_event event;
            bool have_event = false;
            for (int tl=num_tl-1; tl>0; --tl) {
              cl_event new_event;
              checkErr (clEnqueueCopyBuffer (device->queue,
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
        
      } END_LOCAL_COMPONENT_LOOP;
    } END_LOCAL_MAP_LOOP;
    
    return 0;
  }
  
  
  
  extern "C"
  void OpenCLRunTime_CopyBuffers (cGH const *const cctkGH,
                                  int const varindices[],
                                  int const nvars)
  {
    DECLARE_CCTK_ARGUMENTS;
    
    assert (nvars>=0);
    
    int const NP =
      device->grid.lsh[0] * device->grid.lsh[1] * device->grid.lsh[2];
    
    for (int var=0; var<nvars; ++var) {
      int const vi = varindices[var];
      assert(device->mems.at(vi).size() > 1);
      checkErr (clEnqueueCopyBuffer (device->queue,
                                     device->mems.at(vi).at(1),
                                     device->mems.at(vi).at(0),
                                     0, 0, NP*sizeof(CCTK_REAL),
                                     0, NULL, NULL));
    }
  }
  
  
  
  void setup_kernel (cGH const *const cctkGH,
                     char const *const thorn,
                     char const *const name,
                     char const *const sources[],
                     char const *const groups[],
                     int const varindices[],
                     int const timelevels[],
                     char const *const aliases[],
                     int const nvars,
                     OpenCLKernel **const pkernel)
  {
    DECLARE_CCTK_PARAMETERS;
    
    cl_int errcode;
    
    assert (cctkGH);
    assert (name);
    assert (sources);
    assert (pkernel);
    
    CCTK_VInfo (CCTK_THORNSTRING, "Setting up OpenCL kernel %s", name);
    
    assert (not *pkernel);
    *pkernel = new OpenCLKernel;
    OpenCLKernel *restrict const kernel = *pkernel;  
    
    kernel->name = strdup (name);
    
    /*** Determine arguments for calling the kernel ***************************/
    
    if (nvars == -1) {
      // Determine arguments by group name
      
      assert (groups);
      assert (not varindices);
      assert (not timelevels);
      assert (not aliases);
      
      for (int group=0; groups[group]; ++group) {
        int const gi = CCTK_GroupIndex(groups[group]);
        assert (gi>=0);
        int const nv = CCTK_NumVarsInGroupI(gi);
        assert (nv>=0);
        if (nv > 0) {
          int const v0 = CCTK_FirstVarIndexI(gi);
          assert (v0>=0);
          int const num_tl = CCTK_ActiveTimeLevelsGI(cctkGH, gi);
          assert (num_tl >= 0);
          for (int vi=v0; vi<v0+nv; ++vi) {
            string alias(CCTK_VarName(vi));
            for (int tl=0; tl<num_tl; ++tl) {
              OpenCLKernel::arg_t const arg = {vi, tl, alias};
              kernel->args.push_back (arg);
              alias += "_p";
            }
          }
        }
      }
      
    } else {
      // Arguments are given explicitly
      
      assert (nvars >= 0);
      assert (not groups);
      assert (varindices);
      assert (timelevels);
      assert (aliases);
      
      for (int var=0; var<nvars; ++var) {
        int const vi = varindices[var];
        assert (vi>=0 and vi<CCTK_NumVars());
        int const tl = timelevels[var];
        assert (tl>=0);
        OpenCLKernel::arg_t const arg = {vi, tl, aliases[var]};
        kernel->args.push_back (arg);
      }
      
    }
    
    for (vector<OpenCLKernel::arg_t>::const_iterator
           argi = kernel->args.begin(), arge = kernel->args.end();
         argi != arge; ++argi)
    {
      int const vi = argi->vi;
      int const tl = argi->tl;
      int const gi = CCTK_GroupIndexFromVarI(vi);
      if (CCTK_GroupTypeI(gi) != CCTK_GF) {
        char *const group_name = CCTK_GroupName(gi);
        CCTK_VWarn (CCTK_WARN_ABORT, __LINE__, __FILE__, CCTK_THORNSTRING,
                    "OpenCL kernel %s uses the grid variable group %s, which is not a grid function",
                    kernel->name, group_name);
        free (group_name);
      }
      if (CCTK_VarTypeI(vi) != CCTK_VARIABLE_REAL) {
        char *const group_name = CCTK_GroupName(gi);
        CCTK_VWarn (CCTK_WARN_ABORT, __LINE__, __FILE__, CCTK_THORNSTRING,
                    "OpenCL kernel %s uses the grid variable group %s, which is not of variable type CCTK_REAL",
                    kernel->name, group_name);
        free (group_name);
      }
      if (tl >= CCTK_ActiveTimeLevelsGI(cctkGH, gi)) {
        char *const group_name = CCTK_GroupName(gi);
        CCTK_VWarn (CCTK_WARN_ABORT, __LINE__, __FILE__, CCTK_THORNSTRING,
                    "OpenCL kernel %s uses timelevel %d of the grid variable group %s, which does not exist",
                    kernel->name, tl, group_name);
        free (group_name);
      }
    }
    
    /*** Determine parameters for calling the kernel **************************/
    
    stringstream paramdecls, paramdefs;
    vector<char> paramvalues;
    
    paramdecls << "typedef struct {\n";
    paramdefs << "#define DECLARE_CCTK_PARAMETERS";
    
    // Emit CCTK_REAL parameters first, then CCTK_INT, to ensure
    // proper alignment
    int const param_types[] = {PARAMETER_REAL, PARAMETER_INT};
    // TODO: all other parameter types are currently ignored; this
    // should not be so
    for (int param_type_idx=0; param_type_idx<2; ++param_type_idx) {
      int const param_type = param_types[param_type_idx];
      
      int first = 1;
      while (true) {
        
        cParamData const *data;
        int const istat = CCTK_ParameterWalk(first, thorn, NULL, &data);
        assert(istat>=0);
        if (istat > 0) break;
        assert(data->array_size==0);
        first = 0;
        
        // Ignore all types except one in this param_type iteration
        if (data->type != param_type) continue;
        
        void const *const paramval =
          CCTK_ParameterGet(data->name, data->thorn, NULL);
        
        string type_name;
        int type_size;
        switch (data->type) {
        case PARAMETER_INT:
          type_name = "CCTK_INT";
          type_size = sizeof(CCTK_INT);
          break;
        case PARAMETER_REAL:
          type_name = "CCTK_REAL";
          type_size = sizeof(CCTK_REAL);
          break;
        default: assert(0);
        }
        
        paramdecls << "  " << type_name << " " << data->name << ";\n";
        
        paramdefs << " \\\n"
                  << "  " << type_name << " const " <<  data->name << " __attribute__((__unused__)) = cctk_parameters->" << data->name << ";";
        
        size_t const oldpos = paramvalues.size();
        paramvalues.resize(oldpos + type_size);
        memcpy(&paramvalues.at(oldpos), paramval, type_size);
      }
      
    }
    
    if (paramvalues.empty()) {
      // Add dummy byte because OpenCL doesn't like zero-length memory
      // objects
      paramvalues.push_back('\0');
    }
    
    paramdecls << "} cctk_parameters_t;\n";
    paramdefs << "\n";
    
    /*** Create source ********************************************************/
    
    assert (sources[0]);
    assert (sources[1]);
    assert (not sources[2]);
    
    stringstream buf;
    buf << "// -*-C-*-\n"
        << "\n"
        << "// Code generation choices:\n"
        << "#define SIZEOF_PTRDIFF_T         " << sizeof(cl_ptrdiff_t) << "\n"
        << "#define VECTORISE_ALIGNED_ARRAYS " << device->memory_aligned << "\n"
        << "\n"
        << "// Loop traversal choices:\n"
        << "#define VECTOR_SIZE_I " << device->vector_size[0] << "\n"
        << "#define VECTOR_SIZE_J " << device->vector_size[1] << "\n"
        << "#define VECTOR_SIZE_K " << device->vector_size[2] << "\n"
        << "#define UNROLL_SIZE_I " << device->unroll_size[0] << "\n"
        << "#define UNROLL_SIZE_J " << device->unroll_size[1] << "\n"
        << "#define UNROLL_SIZE_K " << device->unroll_size[2] << "\n"
        << "#define GROUP_SIZE_I  " << device->group_size[0] << "\n"
        << "#define GROUP_SIZE_J  " << device->group_size[1] << "\n"
        << "#define GROUP_SIZE_K  " << device->group_size[2] << "\n"
        << "#define TILE_SIZE_I   " << device->tile_size[0] << "\n"
        << "#define TILE_SIZE_J   " << device->tile_size[1] << "\n"
        << "#define TILE_SIZE_K   " << device->tile_size[2] << "\n"
        << "\n"
        << "// OpenCL RunTime definitions:\n"
        << OpenCLMacros_GetSource()
        << "\n"
        << "// Cactus parameters:\n"
        << paramdecls.str()
        << paramdefs.str()
        << "\n"
        << "// Kranc's FD operators:\n"
        << sources[0]
        << "\n"
        << "// Kernel Function:\n"
        << "kernel\n"
        << "__attribute__((vec_type_hint(CCTK_REAL_VEC)))\n"
        << "__attribute__((reqd_work_group_size(GROUP_SIZE_I, GROUP_SIZE_J, GROUP_SIZE_K)))\n"
        << "void " << name << "\n"
        << "  (cGH constant *restrict const cctkGH,\n"
        << "   cctk_parameters_t constant *restrict const cctk_parameters";
    // Cactus grid functions
    for (int arg=0; arg<int(kernel->args.size()); ++arg) {
      buf << ",\n"
          << "   CCTK_REAL global *restrict const " << kernel->args[arg].alias;
    }
    buf << ")\n"
        << "{\n"
        << "  DECLARE_CCTK_ARGUMENTS;\n"
        << "  DECLARE_CCTK_PARAMETERS;\n"
        << "\n"
        << "  // The Kernel:\n"
        << sources[1]           // Kranc generated kernel code
        << "}\n";
    string const sbuf = buf.str();
    
    // Write source
    {
      stringstream filename;
      filename << out_dir << "/" << name << ".cl";
      ofstream file (filename.str().c_str());
      file << sbuf;
      file.close();
    }
    
    /*** Build program from source ********************************************/
    
    char const *source[] = {sbuf.c_str()};
    
    checkErr ((kernel->program =
               clCreateProgramWithSource (device->context, 1, source, NULL,
                                          &errcode),
               errcode));
    
    // Ignore build errors
    checkWarn (clBuildProgram (kernel->program, 1, &device->device_id,
                               opencl_options, NULL, NULL));
    size_t log_size;
    checkErr (clGetProgramBuildInfo (kernel->program, device->device_id,
                                     CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size));
    char build_log[log_size];
    checkErr (clGetProgramBuildInfo (kernel->program, device->device_id,
                                     CL_PROGRAM_BUILD_LOG,
                                     log_size, build_log, NULL));
    
    // Write log
    {
      stringstream filename;
      filename << out_dir << "/" << name << ".log";
      ofstream file (filename.str().c_str());
      file << build_log;
      file.close();
    }
    
    cl_build_status build_status;
    checkErr (clGetProgramBuildInfo (kernel->program, device->device_id,
                                     CL_PROGRAM_BUILD_STATUS,
                                     sizeof build_status, &build_status, NULL));
    if (veryverbose) {
      CCTK_VInfo (CCTK_THORNSTRING,
                  "Build status: %s",
                  build_status == CL_BUILD_NONE        ? "none"        :
                  build_status == CL_BUILD_ERROR       ? "error"       :
                  build_status == CL_BUILD_SUCCESS     ? "success"     :
                  build_status == CL_BUILD_IN_PROGRESS ? "in_progress" :
                  NULL);
    }
    if (build_status == CL_BUILD_ERROR) {
      CCTK_WARN (CCTK_WARN_ABORT, "Build error");
    }
    
    // Create kernel from the program
    checkErr ((kernel->kernel =
               clCreateKernel (kernel->program, kernel->name, &errcode),
               errcode));
    
    /*** Assign buffers to all variables used by this kernel ******************/
    
    checkErr ((kernel->mem_grid =
               clCreateBuffer (device->context,
                               CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                               sizeof kernel->grid, &kernel->grid, &errcode),
               errcode));
    
    cl_mem_flags mem_flags;
    switch (device->memory_model) {
    case mm_always_mapped:
      // Re-use the memory of the grid functions; we never map or copy
      mem_flags = CL_MEM_USE_HOST_PTR;
      break;
    case mm_copy:
      // Allocate memory, and copy (don't map) from/to grid functions
      mem_flags = 0 /*CL_MEM_ALLOC_HOST_PTR*/;
      break;
    case mm_map:
      // Allocate memory, and (don't copy) from/to grid functions
      mem_flags = CL_MEM_ALLOC_HOST_PTR;
      break;
    default:
      assert(0);
    }
    bool const need_ptr =
      (mem_flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR)) != 0;
    
    int const np =
      device->grid.lsh[0] * device->grid.lsh[1] * device->grid.lsh[2];
    for (int arg=0; arg<int(kernel->args.size()); ++arg) {
      int const vi = kernel->args[arg].vi;
      int const tl = kernel->args[arg].tl;
      if (int(device->mems.at(vi).size()) <= tl) {
        assert (int(device->mems.at(vi).size()) == tl);
        device->mems.at(vi).resize(tl+1);
        void * const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
        checkErr ((device->mems.at(vi).at(tl) =
                   clCreateBuffer (device->context, mem_flags,
                                   np*sizeof(CCTK_REAL), need_ptr ? ptr : NULL,
                                   &errcode),
                   errcode));
      }
    }
    
    /*** Set up parameters for calling the kernel *****************************/
    
    checkErr ((kernel->mem_params =
               clCreateBuffer (device->context,
                               CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY,
                               paramvalues.size(), &paramvalues.front(),
                               &errcode),
               errcode));
    
    /*** Set up arguments for calling the kernel ******************************/
    
    checkErr (clSetKernelArg (kernel->kernel, 0,
                              sizeof kernel->mem_grid, &kernel->mem_grid));
    
    checkErr (clSetKernelArg (kernel->kernel, 1,
                              sizeof kernel->mem_params, &kernel->mem_params));
    
    for (int arg=0; arg<(kernel->args.size()); ++arg) {
      int const vi = kernel->args[arg].vi;
      int const tl = kernel->args[arg].tl;
      checkErr (clSetKernelArg (kernel->kernel, arg+2,
                                sizeof device->mems.at(vi).at(tl),
                                &device->mems.at(vi).at(tl)));
    }
  }
  
  
  
  void OpenCLRunTime_CallKernel (cGH const *const cctkGH,
                                 char const *const thorn,
                                 char const *const name,
                                 char const *const sources[],
                                 char const *const groups[],
                                 int const varindices[],
                                 int const timelevels[],
                                 char const *const aliases[],
                                 int const nvars, 
                                 int const imin[],
                                 int const imax[],
                                 OpenCLKernel **const pkernel)
  {
    DECLARE_CCTK_PARAMETERS;
    
    assert (cctkGH);
    assert (pkernel);
    
    // Ensure the device has been set up
    assert (device);
    
    // If this is the first call for this kernel, build the kernel and
    // set up all kernel data structures
    if (not *pkernel) {
      setup_kernel
        (cctkGH, thorn, name, sources,
         groups, varindices, timelevels, aliases, nvars,
         pkernel);
    }
    OpenCLKernel *restrict const kernel = *pkernel;
    
    if (veryverbose) {
      CCTK_VInfo (CCTK_THORNSTRING, "Enqueuing OpenCL kernel %s", name);
    }
    
    // Set up grid description
    kernel->grid = device->grid;
    
    for (int d=0; d<dim; ++d) {
      kernel->grid.imin[d] = imin[d];
      kernel->grid.imax[d] = imax[d];
      assert (kernel->grid.imin[d] >= 0);
      assert (kernel->grid.imax[d] <= kernel->grid.lssh[d]);
      assert (kernel->grid.imin[d] <= kernel->grid.imax[d]);
    }
    
    if (veryverbose) {
      CCTK_VInfo (CCTK_THORNSTRING,
                  "Looping region minimum: %4d %4d %4d",
                  (int)imin[0],
                  (int)imin[1],
                  (int)imin[2]);
      CCTK_VInfo (CCTK_THORNSTRING,
                  "Looping region maximum: %4d %4d %4d",
                  (int)imax[0],
                  (int)imax[1],
                  (int)imax[2]);
    }
    
    for (int d=0; d<dim; ++d) {
      // alignment of lower bound
      int const align_lo = device->vector_size[d];
      // alignment of region size
      int const align_sz = device->vector_size[d] * device->unroll_size[d];
      kernel->grid.lmin[d] = kernel->grid.imin[d] / align_lo * align_lo;
      kernel->grid.lmax[d] =
        kernel->grid.lmin[d] +
        divup(kernel->grid.imax[d] - kernel->grid.lmin[d], align_sz) * align_sz;
    }
    for (int d=0; d<dim; ++d) {
      assert (kernel->grid.lmin[d] >= 0);
      assert (kernel->grid.imin[d] >= kernel->grid.lmin[d]);
      assert (kernel->grid.imax[d] <= kernel->grid.lmax[d]);
      assert (kernel->grid.lmax[d] <= kernel->grid.lsh[d]);
      assert (kernel->grid.lmin[d] % device->vector_size[d] == 0);
      assert ((kernel->grid.lmax[d] - kernel->grid.lmin[d]) %
              (device->vector_size[d] * device->unroll_size[d]) == 0);
    }
    
    kernel->grid.time       = cctkGH->cctk_time;
    kernel->grid.delta_time = cctkGH->cctk_delta_time;
    
    // We use a blocking write because the next call for the same
    // kernel may have a different grid configuration, and we then
    // overwrite the host memory where this is stored.
    checkErr (clEnqueueWriteBuffer (device->queue, kernel->mem_grid, CL_TRUE,
                                    0, sizeof kernel->grid, &kernel->grid,
                                    0, NULL, NULL));
    
    // Calculate number of thread groups
    
    size_t local_work_size[dim];
    for (int d=0; d<dim; ++d) {
      local_work_size[d] = device->group_size[d];
    }
    if (veryverbose) {
      CCTK_VInfo (CCTK_THORNSTRING,
                  "Local work group size:  %4d %4d %4d",
                  (int)local_work_size[0],
                  (int)local_work_size[1],
                  (int)local_work_size[2]);
    }
    
    size_t global_work_size[dim];
    for (int d=0; d<dim; ++d) {
      global_work_size[d] =
        divup (kernel->grid.lmax[d] - kernel->grid.lmin[d],
               device->vector_size[d] * device->unroll_size[d] *
               device->group_size[d] * device->tile_size[d]) *
        local_work_size[d];
    }
    if (veryverbose) {
      CCTK_VInfo (CCTK_THORNSTRING,
                  "Global work group size: %4d %4d %4d",
                  (int)global_work_size[0],
                  (int)global_work_size[1],
                  (int)global_work_size[2]);
    }
    
    // Queue a single execution of the kernel
    checkErr (clEnqueueNDRangeKernel (device->queue, kernel->kernel, dim,
                                      NULL, global_work_size, local_work_size,
                                      0, NULL, NULL));
    
    // This finish prevents nans on the outer boundary when running on
    // multiple processes (with the Intel implementation). I don't
    // know why it is necessary. The nans appear randomly, so this is
    // probably a timing issue.
#if 1
    checkErr (clFinish (device->queue));
#endif
  }
  
  
  
  extern "C"
  CCTK_INT OpenCLRunTime_PreSync (CCTK_POINTER_TO_CONST const cctkGH_,
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
      CCTK_VInfo (CCTK_THORNSTRING, "Pre-syncing%s", buf.str().c_str());
    }
    
    if (device->memory_model == mm_always_mapped) return 0;
    
    // Queue reading for ghost zones
    BEGIN_LOCAL_MAP_LOOP(cctkGH, CCTK_GF) {
      BEGIN_LOCAL_COMPONENT_LOOP(cctkGH, CCTK_GF) {
        DECLARE_CCTK_ARGUMENTS;
        
        for (int group=0; group<ngroups; ++group) {
          int const gi = groups[group];
          assert (gi>=0);
          int const nv = CCTK_NumVarsInGroupI(gi);
          assert (nv>=0);
          if (nv > 0) {
            int const v0 = CCTK_FirstVarIndexI(gi);
            assert (v0>=0);
            for (int vi=v0; vi<v0+nv; ++vi) {
              
              if (sync_copy_whole_buffer) {
                
                size_t offset[dim];
                size_t length[dim];
                for (int d=0; d<dim; ++d) {
                  offset[d] = 0;
                  length[d] = cctkGH->CCTK_LSSH(0,d);
                }
                
                int const dI = sizeof(CCTK_REAL);
                int const dJ = dI * device->grid.lsh[0];
                int const dK = dJ * device->grid.lsh[1];
                int const di = sizeof(CCTK_REAL);
                int const dj = di * cctk_lsh[0];
                int const dk = dj * cctk_lsh[1];
                offset[0] *= di;
                length[0] *= di;
                
                int const tl = 0; // only copy current timelevel
                void *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
                
                assert (int(device->mems.at(vi).size()) > tl);
                checkErr (clEnqueueReadBufferRect (device->queue,
                                                   device->mems.at(vi).at(tl),
                                                   CL_FALSE,
                                                   offset, offset, length,
                                                   dJ, dK, dj, dk,
                                                   ptr,
                                                   0, NULL, NULL));
                
              } else {
                
                cl_event event;
                bool have_event = false;
                for (int dir=0; dir<dim; ++dir) {
                  for (int face=0; face<2; ++face) {
                    if (not cctk_bbox[2*dir+face]) {
                      
                      int imin[dim];
                      int imax[dim];
                      for (int d=0; d<dim; ++d) {
                        // Whole domain
                        imin[d] = 0;
                        imax[d] = CCTK_LSSH(0,d);
                        // Skip ghost points
                        if (not cctk_bbox[2*d+0]) {
                          imin[d] += cctk_nghostzones[d];
                        }
                        if (not cctk_bbox[2*d+1]) {
                          imax[d] -= cctk_nghostzones[d];
                        }
                        // Skip points that will be copied via later
                        // directions
                        if (d > dir) {
                          if (not cctk_bbox[2*d+0]) {
                            imin[d] += cctk_nghostzones[d];
                          }
                          if (not cctk_bbox[2*d+1]) {
                            imax[d] -= cctk_nghostzones[d];
                          }
                        }
                      }
                      if (face==0) {
                        imin[dir] = cctk_nghostzones[dir];
                        imax[dir] = imin[dir] + cctk_nghostzones[dir];
                      } else {
                        imax[dir] = CCTK_LSSH(0,dir) - cctk_nghostzones[dir];
                        imin[dir] = imax[dir] - cctk_nghostzones[dir];
                      }
                      for (int d=0; d<dim; ++d) {
                        assert (imin[d] >= 0);
                        assert (imin[d] <= imax[d]);
                        assert (imax[d] <= CCTK_LSSH(0,d));
                      }
                      size_t offset[dim];
                      size_t length[dim];
                      for (int d=0; d<dim; ++d) {
                        offset[d] = imin[d];
                        length[d] = imax[d] - imin[d];
                      }
                      
                      int const dI = sizeof(CCTK_REAL);
                      int const dJ = dI * device->grid.lsh[0];
                      int const dK = dJ * device->grid.lsh[1];
                      int const di = sizeof(CCTK_REAL);
                      int const dj = di * cctk_lsh[0];
                      int const dk = dj * cctk_lsh[1];
                      offset[0] *= di;
                      length[0] *= di;
                      
                      int const tl = 0; // only copy current timelevel
                      void *const ptr = CCTK_VarDataPtrI (cctkGH, tl, vi);
                      
                      assert (int(device->mems.at(vi).size()) > tl);
                      cl_event new_event;
                      checkErr (clEnqueueReadBufferRect (device->queue,
                                                         device->
                                                         mems.at(vi).at(tl),
                                                         CL_FALSE,
                                                         offset, offset, length,
                                                         dJ, dK, dj, dk,
                                                         ptr,
                                                         have_event ? 1 : 0,
                                                         have_event ? &event : NULL,
                                                         &new_event));
                      // event = new_event;
                      // have_event = true;
                      
                    }
                  }
                }
                
              }
              
            }
          }
        }
        
      } END_LOCAL_COMPONENT_LOOP;
    } END_LOCAL_MAP_LOOP;
    
    // Finish, because we synchronise
    checkErr (clFinish (device->queue));
    
    return 0;
  }
  
  extern "C"
  CCTK_INT OpenCLRunTime_PostSync (CCTK_POINTER_TO_CONST const cctkGH_,
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
      CCTK_VInfo (CCTK_THORNSTRING, "Post-syncing%s", buf.str().c_str());
    }
    
    if (device->memory_model == mm_always_mapped) return 0;
    
    // Queue writing ghost zones
    BEGIN_LOCAL_MAP_LOOP(cctkGH, CCTK_GF) {
      BEGIN_LOCAL_COMPONENT_LOOP(cctkGH, CCTK_GF) {
        DECLARE_CCTK_ARGUMENTS;
        
        for (int group=0; group<ngroups; ++group) {
          int const gi = groups[group];
          assert (gi>=0);
          int const nv = CCTK_NumVarsInGroupI(gi);
          assert (nv>=0);
          if (nv > 0) {
            int const v0 = CCTK_FirstVarIndexI(gi);
            assert (v0>=0);
            for (int vi=v0; vi<v0+nv; ++vi) {
              
              if (sync_copy_whole_buffer) {
                
                size_t offset[dim];
                size_t length[dim];
                for (int d=0; d<dim; ++d) {
                  offset[d] = 0;
                  length[d] = cctkGH->CCTK_LSSH(0,d);
                }
                
                int const dI = sizeof(CCTK_REAL);
                int const dJ = dI * device->grid.lsh[0];
                int const dK = dJ * device->grid.lsh[1];
                int const di = sizeof(CCTK_REAL);
                int const dj = di * cctk_lsh[0];
                int const dk = dj * cctk_lsh[1];
                offset[0] *= di;
                length[0] *= di;
                
                int const tl = 0; // only copy current timelevel
                void *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
                
                assert (int(device->mems.at(vi).size()) > tl);
                checkErr (clEnqueueWriteBufferRect (device->queue,
                                                    device->
                                                    mems.at(vi).at(tl),
                                                    CL_FALSE,
                                                    offset, offset, length,
                                                    dJ, dK, dj, dk,
                                                    ptr,
                                                    0, NULL, NULL));
                
              } else {
                
                cl_event event;
                bool have_event = false;
                for (int dir=0; dir<dim; ++dir) {
                  for (int face=0; face<2; ++face) {
                    if (not cctk_bbox[2*dir+face]) {
                      
                      int imin[dim];
                      int imax[dim];
                      for (int d=0; d<dim; ++d) {
                      // Whole domain
                        imin[d] = 0;
                        imax[d] = CCTK_LSSH(0,d);
                        // Skip points that will be copied via later
                        // directions
                        if (d > dir) {
                          if (not cctk_bbox[2*d+0]) {
                            imin[d] += cctk_nghostzones[d];
                          }
                          if (not cctk_bbox[2*d+1]) {
                            imax[d] -= cctk_nghostzones[d];
                          }
                        }
                      }
                      if (face==0) {
                        imin[dir] = 0;
                        imax[dir] = imin[dir] + cctk_nghostzones[dir];
                      } else {
                        imax[dir] = CCTK_LSSH(0,dir);
                        imin[dir] = imax[dir] - cctk_nghostzones[dir];
                      }
                      for (int d=0; d<dim; ++d) {
                        assert (imin[d] >= 0);
                        assert (imin[d] <= imax[d]);
                        assert (imax[d] <= CCTK_LSSH(0,d));
                      }
                      size_t offset[dim];
                      size_t length[dim];
                      for (int d=0; d<dim; ++d) {
                        offset[d] = imin[d];
                        length[d] = imax[d] - imin[d];
                      }
                      
                      int const dI = sizeof(CCTK_REAL);
                      int const dJ = dI * device->grid.lsh[0];
                      int const dK = dJ * device->grid.lsh[1];
                      int const di = sizeof(CCTK_REAL);
                      int const dj = di * cctk_lsh[0];
                      int const dk = dj * cctk_lsh[1];
                      offset[0] *= di;
                      length[0] *= di;
                      
                      int const tl = 0; // only copy current timelevel
                      void const *const ptr = CCTK_VarDataPtrI (cctkGH, tl, vi);
                      
                      assert (int(device->mems.at(vi).size()) > tl);
                      cl_event new_event;
                      checkErr (clEnqueueWriteBufferRect (device->queue,
                                                          device->
                                                          mems.at(vi).at(tl),
                                                          CL_FALSE,
                                                          offset, offset, length,
                                                          dJ, dK, dj, dk,
                                                          ptr,
                                                          have_event ? 1 : 0,
                                                          have_event ? &event : NULL,
                                                          &new_event));
                      // event = new_event;
                      // have_event = true;
                      
                    }
                  }
                }
                
              }
              
            }
          }
        }
        
      } END_LOCAL_COMPONENT_LOOP;
    } END_LOCAL_MAP_LOOP;
    
    return 0;
  }
  
  
  
  extern "C"
  void OpenCLRunTime_Analyse (CCTK_ARGUMENTS)
  {
    DECLARE_CCTK_ARGUMENTS;
    DECLARE_CCTK_PARAMETERS;
    
    if (device->memory_model == mm_always_mapped) return;
    
    int const np = cctk_lsh[0] * cctk_lsh[1] * cctk_lsh[2];
    int const NP =
      device->grid.lsh[0] * device->grid.lsh[1] * device->grid.lsh[2];
    if (np == NP) {
      // If the host and device extents are the same, read the whole
      // buffer
      
      for (int vi=0; vi<int(device->mems.size()); ++vi) {
        int const tl=0;       // only copy current timelevel
        if (int(device->mems.at(vi).size()) > tl) {
          void * const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
          checkErr (clEnqueueReadBuffer (device->queue,
                                         device->mems.at(vi).at(tl),
                                         CL_FALSE,
                                         0, np*sizeof(CCTK_REAL), ptr,
                                         0, NULL, NULL));
        }
      }
      
    } else {
      // If the host and device extents differ, read only a rectangle
      
      size_t offset[dim];
      size_t length[dim];
      for (int d=0; d<dim; ++d) {
        offset[d] = 0;
        length[d] = cctkGH->CCTK_LSSH(0,d);
      }
      
      int const dI = sizeof(CCTK_REAL);
      int const dJ = dI * device->grid.lsh[0];
      int const dK = dJ * device->grid.lsh[1];
      int const di = sizeof(CCTK_REAL);
      int const dj = di * cctk_lsh[0];
      int const dk = dj * cctk_lsh[1];
      offset[0] *= di;
      length[0] *= di;
      
      for (int vi=0; vi<int(device->mems.size()); ++vi) {
        int const tl=0;       // only copy current timelevel
        if (int(device->mems.at(vi).size()) > tl) {
          void * const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
          checkErr (clEnqueueReadBufferRect (device->queue,
                                             device->mems.at(vi).at(tl),
                                             CL_FALSE,
                                             offset, offset, length,
                                             dJ, dK, dj, dk,
                                             ptr,
                                             0, NULL, NULL));
        }
      }
      
    }
    
    // Finish, because we output
    checkErr (clFinish (device->queue));
  }
  
  
  
  extern "C"
  void OpenCLRunTime_Terminate (CCTK_ARGUMENTS)
  {
    DECLARE_CCTK_ARGUMENTS;
    DECLARE_CCTK_PARAMETERS;
    
    // Do nothing (yet)
  }
  
} // namespace OpenCLRunTime
