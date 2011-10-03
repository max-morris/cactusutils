#include "device.h"

#include <vectors.h>

#include <cctk_Arguments.h>
#include <cctk_Parameters.h>



namespace OpenCLRunTime {
  
  // Global variable
  OpenCLDevice *device = NULL;
  
  
  
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
  
  
  
  void checkErr1(cl_int const errcode, char const *const cmd,
                 char const *const file, int const line)
  {
    if (errcode == CL_SUCCESS) return;
    CCTK_VWarn(CCTK_WARN_ABORT, line, file, CCTK_THORNSTRING,
               "%s\nError %d: %s",
               cmd, int(errcode), error_string(errcode));
  }
  
  void checkWarn1(cl_int const errcode, char const *const cmd,
                  char const *const file, int const line)
  {
    if (errcode == CL_SUCCESS) return;
    CCTK_VWarn(CCTK_WARN_ALERT, __LINE__, __FILE__, CCTK_THORNSTRING,
               "%s\nError %d: %s",
               cmd, int(errcode), error_string(errcode));
  }
  
  
  
  //////////////////////////////////////////////////////////////////////////////
  
  
  
  OpenCLDevice::OpenCLDevice()
  {
    DECLARE_CCTK_PARAMETERS;
    
    cl_int errcode;
    
    /*** Choose a platform ****************************************************/
    
    cl_uint num_platforms;
    checkErr(clGetPlatformIDs(0, NULL, &num_platforms));
    cl_platform_id platforms[num_platforms];
    checkErr(clGetPlatformIDs(num_platforms, &platforms[0], &num_platforms));
    // Arbitrarily choose first platform
    assert(num_platforms > 0);
    cl_platform_id const platform = platforms[0];
    size_t platform_name_size;
    checkErr(clGetPlatformInfo(platform, CL_PLATFORM_NAME,
                               0, NULL, &platform_name_size));
    char platform_name[platform_name_size];
    checkErr(clGetPlatformInfo(platform, CL_PLATFORM_NAME,
                               platform_name_size, platform_name, NULL));
    CCTK_VInfo(CCTK_THORNSTRING,
               "Selected platform: %s", platform_name);
    
    /*** Choose a context (basically a device) ********************************/
    
    if (CCTK_EQUALS(opencl_device_type, "CPU")) {
      device_type = CL_DEVICE_TYPE_CPU;
    } else if (CCTK_EQUALS(opencl_device_type, "GPU")) {
      device_type = CL_DEVICE_TYPE_GPU;
    } else {
      CCTK_VWarn(CCTK_WARN_ALERT, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "Unknown device type \"%s\" selected", opencl_device_type);
    }
    cl_context_properties const cprops[] =
      {CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0};
    context =
      clCreateContextFromType(cprops, device_type, NULL, NULL, &errcode);
    if (errcode != CL_SUCCESS) {
      CCTK_VWarn(CCTK_WARN_ABORT, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "Could not create OpenCL context for device type \"%s\"",
                 opencl_device_type);
    }
    size_t context_devices_size;
    checkErr(clGetContextInfo(context, CL_CONTEXT_DEVICES,
                              0, NULL, &context_devices_size));
    cl_uint const num_devices = context_devices_size / sizeof(cl_device_id);
    cl_device_id devices[num_devices];
    checkErr(clGetContextInfo(context, CL_CONTEXT_DEVICES,
                              context_devices_size, devices, NULL));
    // Arbitrarily choose first matching device
    assert(num_devices > 0);
    device_id = devices[0];
    size_t device_name_size;
    checkErr(clGetDeviceInfo(device_id, CL_DEVICE_NAME,
                             0, NULL, &device_name_size));
    char device_name[device_name_size];
    checkErr(clGetDeviceInfo(device_id, CL_DEVICE_NAME,
                             device_name_size, device_name, NULL));
    CCTK_VInfo(CCTK_THORNSTRING,
               "Selected device: %s", device_name);
    cl_device_type type;
    checkErr(clGetDeviceInfo(device_id, CL_DEVICE_TYPE,
                             sizeof type, &type, NULL));
    CCTK_VInfo(CCTK_THORNSTRING,
               "   Device type: %s",
               type == CL_DEVICE_TYPE_CPU         ? "CPU"        :
               type == CL_DEVICE_TYPE_GPU         ? "GPU"        :
               type == CL_DEVICE_TYPE_ACCELERATOR ? "ACCELERATOR":
               NULL);
    
    /*** Create execution queue ***********************************************/
    
    checkErr((queue =
              clCreateCommandQueue(context, device_id,
                                   /*CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | */
                                   CL_QUEUE_PROFILING_ENABLE,
                                   &errcode),
              errcode));
    
    /*** Set up memory buffers ************************************************/
    
    // Memory model
    // TODO: compare with CL_DEVICE_HOST_UNIFIED_MEMORY
    if (CCTK_EQUALS(memory_model, "always-mapped")) {
      mem_model = mm_always_mapped;
    } else if (CCTK_EQUALS(memory_model, "copy")) {
      mem_model = mm_copy;
    } else if (CCTK_EQUALS(memory_model, "map")) {
      mem_model = mm_map;
    } else {
      CCTK_WARN(CCTK_WARN_ABORT, "internal error");
    }
    
    mems.resize(CCTK_NumVars());
    mem_host_valid.resize(CCTK_NumVars(), true);
    mem_device_valid.resize(CCTK_NumVars(), false);
    
    have_grid = false;
  }
  
  
  
  void OpenCLDevice::setup_grid(cGH const *restrict const cctkGH)
  {
    DECLARE_CCTK_ARGUMENTS;
    DECLARE_CCTK_PARAMETERS;
    
    if (have_grid) return;
    have_grid = true;
    
    assert(Carpet::is_local_mode());
    
    /*** Choose looping configuration *****************************************/
    
    memory_aligned =
      mem_model == mm_copy or
      (vector_size_x == 1 and
       vector_size_y == 1 and
       vector_size_z == 1) or
      ((VECTORISE and VECTORISE_ALIGNED_ARRAYS) and
       vector_size_x <= CCTK_REAL_VEC_SIZE and
       vector_size_y == 1 and
       vector_size_z == 1);
    
    // Vector size
    vector_size[0] = vector_size_x;
    vector_size[1] = vector_size_y;
    vector_size[2] = vector_size_z;
    if (vector_size[0] == 0) {
      checkErr(clGetDeviceInfo(device_id,
                               CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE,
                               sizeof vector_size[0],
                               vector_size, NULL));
    }
    if (vector_size[0] == 0) {
      // If double vectors are not supported, try long instead
      checkErr(clGetDeviceInfo(device_id,
                               CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG,
                               sizeof vector_size[0],
                               vector_size, NULL));
    }
    if (vector_size[0] == 0) {
      CCTK_WARN(CCTK_WARN_ABORT, "Could not determine preferred vector size");
    }
    CCTK_VInfo(CCTK_THORNSTRING,
               "Vector size: %2d %2d %2d",
               vector_size[0],
               vector_size[1],
               vector_size[2]);
    
    // Unrolled loops
    unroll_size[0] = unroll_size_x;
    unroll_size[1] = unroll_size_y;
    unroll_size[2] = unroll_size_z;
    CCTK_VInfo(CCTK_THORNSTRING,
               "Unroll size: %2d %2d %2d",
               unroll_size[0],
               unroll_size[1],
               unroll_size[2]);
    
    // Closely coupled threads (aka OpenCL groups)
    group_size[0] = group_size_x;
    group_size[1] = group_size_y;
    group_size[2] = group_size_z;
    CCTK_VInfo(CCTK_THORNSTRING,
               "Group size:  %2d %2d %2d",
               group_size[0],
               group_size[1],
               group_size[2]);
    
    // Explicit kernel loops (aka loop tiling)
    tile_size[0] = tile_size_x;
    tile_size[1] = tile_size_y;
    tile_size[2] = tile_size_z;
    CCTK_VInfo(CCTK_THORNSTRING,
               "Tile size:   %2d %2d %2d",
               tile_size[0],
               tile_size[1],
               tile_size[2]);
    
    // Describe grid structure
    same_padding = true;
    for (int d=0; d<dim; ++d) {
      grid.gsh[d] = cctkGH->cctk_gsh[d];
      grid.lbnd[d] = cctkGH->cctk_lbnd[d];
      grid.lssh[d] = cctkGH->CCTK_LSSH(0,d);
      int const granularity = vector_size[d];
      int const good_lsh = divup(cctk_lsh[d], granularity) * granularity;
      grid.lsh[d] = memory_aligned ? good_lsh : cctk_lsh[d];
      same_padding &= grid.lsh[d] == cctk_lsh[d];
      assert(grid.gsh[d] >= 0);
      assert(grid.lbnd[d] >= 0);
      assert(grid.lbnd[d] <= grid.gsh[d]);
      assert(grid.lssh[d] >= 0);
      assert(grid.lbnd[d] + grid.lssh[d] <= grid.gsh[d]);
      assert(grid.lsh[d] >= 0);
      assert(grid.lssh[d] <= grid.lsh[d]);
      grid.origin_space[d] = cctkGH->cctk_origin_space[d];
      grid.delta_space[d] = cctkGH->cctk_delta_space[d];
    }
    grid.time = cctkGH->cctk_time;
    grid.delta_time = cctkGH->cctk_delta_time;
    CCTK_VInfo(CCTK_THORNSTRING,
               "gsh:  %4d %4d %4d",
               (int)grid.gsh[0],
               (int)grid.gsh[1],
               (int)grid.gsh[2]);
    CCTK_VInfo(CCTK_THORNSTRING,
               "lbnd: %4d %4d %4d",
               (int)grid.lbnd[0],
               (int)grid.lbnd[1],
               (int)grid.lbnd[2]);
    CCTK_VInfo(CCTK_THORNSTRING,
               "lssh: %4d %4d %4d",
               (int)grid.lssh[0],
               (int)grid.lssh[1],
               (int)grid.lssh[2]);
    CCTK_VInfo(CCTK_THORNSTRING,
               "lsh:  %4d %4d %4d",
               (int)grid.lsh[0],
               (int)grid.lsh[1],
               (int)grid.lsh[2]);
  }
  
  
  
  extern "C"
  int OpenCLRunTime_Setup()
  {
    DECLARE_CCTK_PARAMETERS;
    
    CCTK_INFO("Setting up OpenCL device");
    
    assert(not device);
    device = new OpenCLDevice;
    return 0;
  }
  
  
  
} // namespace OpenCLRunTime
