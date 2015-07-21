#include "device.hh"

#include <cassert>
#include <vectors.h>

#include <cctk_Arguments.h>
#include <cctk_Parameters.h>

namespace OpenCLRunTime {

// Global variable
OpenCLDevice *device = NULL;

char const *error_string(int const error_code) {
  switch (error_code) {
  case CL_SUCCESS:
    return "CL_SUCCESS";
  case CL_DEVICE_NOT_FOUND:
    return "CL_DEVICE_NOT_FOUND";
  case CL_DEVICE_NOT_AVAILABLE:
    return "CL_DEVICE_NOT_AVAILABLE";
  case CL_COMPILER_NOT_AVAILABLE:
    return "CL_COMPILER_NOT_AVAILABLE";
  case CL_MEM_OBJECT_ALLOCATION_FAILURE:
    return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
  case CL_OUT_OF_RESOURCES:
    return "CL_OUT_OF_RESOURCES";
  case CL_OUT_OF_HOST_MEMORY:
    return "CL_OUT_OF_HOST_MEMORY";
  case CL_PROFILING_INFO_NOT_AVAILABLE:
    return "CL_PROFILING_INFO_NOT_AVAILABLE";
  case CL_MEM_COPY_OVERLAP:
    return "CL_MEM_COPY_OVERLAP";
  case CL_IMAGE_FORMAT_MISMATCH:
    return "CL_IMAGE_FORMAT_MISMATCH";
  case CL_IMAGE_FORMAT_NOT_SUPPORTED:
    return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
  case CL_BUILD_PROGRAM_FAILURE:
    return "CL_BUILD_PROGRAM_FAILURE";
  case CL_MAP_FAILURE:
    return "CL_MAP_FAILURE";
#ifdef CL_MISALIGNED_SUB_BUFFER_OFFSET
  case CL_MISALIGNED_SUB_BUFFER_OFFSET:
    return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
#endif
#ifdef CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST
  case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
    return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
#endif
  case CL_INVALID_VALUE:
    return "CL_INVALID_VALUE";
  case CL_INVALID_DEVICE_TYPE:
    return "CL_INVALID_DEVICE_TYPE";
  case CL_INVALID_PLATFORM:
    return "CL_INVALID_PLATFORM";
  case CL_INVALID_DEVICE:
    return "CL_INVALID_DEVICE";
  case CL_INVALID_CONTEXT:
    return "CL_INVALID_CONTEXT";
  case CL_INVALID_QUEUE_PROPERTIES:
    return "CL_INVALID_QUEUE_PROPERTIES";
  case CL_INVALID_COMMAND_QUEUE:
    return "CL_INVALID_COMMAND_QUEUE";
  case CL_INVALID_HOST_PTR:
    return "CL_INVALID_HOST_PTR";
  case CL_INVALID_MEM_OBJECT:
    return "CL_INVALID_MEM_OBJECT";
  case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
    return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
  case CL_INVALID_IMAGE_SIZE:
    return "CL_INVALID_IMAGE_SIZE";
  case CL_INVALID_SAMPLER:
    return "CL_INVALID_SAMPLER";
  case CL_INVALID_BINARY:
    return "CL_INVALID_BINARY";
  case CL_INVALID_BUILD_OPTIONS:
    return "CL_INVALID_BUILD_OPTIONS";
  case CL_INVALID_PROGRAM:
    return "CL_INVALID_PROGRAM";
  case CL_INVALID_PROGRAM_EXECUTABLE:
    return "CL_INVALID_PROGRAM_EXECUTABLE";
  case CL_INVALID_KERNEL_NAME:
    return "CL_INVALID_KERNEL_NAME";
  case CL_INVALID_KERNEL_DEFINITION:
    return "CL_INVALID_KERNEL_DEFINITION";
  case CL_INVALID_KERNEL:
    return "CL_INVALID_KERNEL";
  case CL_INVALID_ARG_INDEX:
    return "CL_INVALID_ARG_INDEX";
  case CL_INVALID_ARG_VALUE:
    return "CL_INVALID_ARG_VALUE";
  case CL_INVALID_ARG_SIZE:
    return "CL_INVALID_ARG_SIZE";
  case CL_INVALID_KERNEL_ARGS:
    return "CL_INVALID_KERNEL_ARGS";
  case CL_INVALID_WORK_DIMENSION:
    return "CL_INVALID_WORK_DIMENSION";
  case CL_INVALID_WORK_GROUP_SIZE:
    return "CL_INVALID_WORK_GROUP_SIZE";
  case CL_INVALID_WORK_ITEM_SIZE:
    return "CL_INVALID_WORK_ITEM_SIZE";
  case CL_INVALID_GLOBAL_OFFSET:
    return "CL_INVALID_GLOBAL_OFFSET";
  case CL_INVALID_EVENT_WAIT_LIST:
    return "CL_INVALID_EVENT_WAIT_LIST";
  case CL_INVALID_EVENT:
    return "CL_INVALID_EVENT";
  case CL_INVALID_OPERATION:
    return "CL_INVALID_OPERATION";
  case CL_INVALID_GL_OBJECT:
    return "CL_INVALID_GL_OBJECT";
  case CL_INVALID_BUFFER_SIZE:
    return "CL_INVALID_BUFFER_SIZE";
  case CL_INVALID_MIP_LEVEL:
    return "CL_INVALID_MIP_LEVEL";
  case CL_INVALID_GLOBAL_WORK_SIZE:
    return "CL_INVALID_GLOBAL_WORK_SIZE";
#ifdef CL_INVALID_PROPERTY
  case CL_INVALID_PROPERTY:
    return "CL_INVALID_PROPERTY";
#endif
#ifdef CL_PLATFORM_NOT_FOUND_KHR
  case CL_PLATFORM_NOT_FOUND_KHR:
    return "CL_PLATFORM_NOT_FOUND_KHR";
#endif
#ifdef CL_DEVICE_PARTITION_FAILED_EXT
  case CL_DEVICE_PARTITION_FAILED_EXT:
    return "CL_DEVICE_PARTITION_FAILED_EXT";
  case CL_INVALID_PARTITION_COUNT_EXT:
    return "CL_INVALID_PARTITION_COUNT_EXT";
  case CL_INVALID_PARTITION_NAME_EXT:
    return "CL_INVALID_PARTITION_NAME_EXT";
#endif
  }
  return "unknown error";
}

void checkErr1(cl_int const errcode, char const *const cmd,
               char const *const file, int const line) {
  if (errcode == CL_SUCCESS)
    return;
  CCTK_VWarn(CCTK_WARN_ABORT, line, file, CCTK_THORNSTRING, "%s\nError %d: %s",
             cmd, int(errcode), error_string(errcode));
}

void checkWarn1(cl_int const errcode, char const *const cmd,
                char const *const file, int const line) {
  if (errcode == CL_SUCCESS)
    return;
  CCTK_VWarn(CCTK_WARN_ALERT, __LINE__, __FILE__, CCTK_THORNSTRING,
             "%s\nError %d: %s", cmd, int(errcode), error_string(errcode));
}

//////////////////////////////////////////////////////////////////////////////

OpenCLDevice::OpenCLDevice() : f_have_grid(false) {
  DECLARE_CCTK_PARAMETERS;

  cl_int errcode;

  /*** Choose a platform and a context (basically a device) *****************/

  cl_uint num_platforms;
  checkErr(clGetPlatformIDs(0, NULL, &num_platforms));
  cl_platform_id platform_ids[num_platforms];
  checkErr(clGetPlatformIDs(num_platforms, &platform_ids[0], &num_platforms));
  assert(num_platforms > 0);

  cl_device_type want_device_types = 0;
  if (CCTK_EQUALS(opencl_device_type, "CPU")) {
    want_device_types = CL_DEVICE_TYPE_CPU;
  } else if (CCTK_EQUALS(opencl_device_type, "GPU")) {
    want_device_types = CL_DEVICE_TYPE_GPU;
  } else if (CCTK_EQUALS(opencl_device_type, "acclerator")) {
    want_device_types = CL_DEVICE_TYPE_ACCELERATOR;
  } else if (CCTK_EQUALS(opencl_device_type, "any")) {
    want_device_types =
        CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR;
  } else {
    CCTK_VWarn(CCTK_WARN_ALERT, __LINE__, __FILE__, CCTK_THORNSTRING,
               "Unknown device type \"%s\" selected", opencl_device_type);
  }

  // Loop over all platforms
  cl_platform_id platform_id;
  for (cl_uint platform = 0; platform < num_platforms; ++platform) {

    platform_id = platform_ids[platform];

    cl_context_properties const cprops[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)platform_id, 0};
    context = clCreateContextFromType(cprops, want_device_types, NULL, NULL,
                                      &errcode);
    if (errcode == CL_SUCCESS)
      goto found_context;
  }
  // Could not find a context on any platform, abort
  CCTK_VWarn(CCTK_WARN_ABORT, __LINE__, __FILE__, CCTK_THORNSTRING,
             "Could not create OpenCL context for device type \"%s\"",
             opencl_device_type);

// Found a context, continue
found_context:
  size_t platform_name_size;
  checkErr(clGetPlatformInfo(platform_id, CL_PLATFORM_NAME, 0, NULL,
                             &platform_name_size));
  char platform_name[platform_name_size];
  checkErr(clGetPlatformInfo(platform_id, CL_PLATFORM_NAME, platform_name_size,
                             platform_name, NULL));
  CCTK_VInfo(CCTK_THORNSTRING, "Selected platform: %s", platform_name);
  size_t context_devices_size;
  checkErr(clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL,
                            &context_devices_size));
  cl_uint const num_devices = context_devices_size / sizeof(cl_device_id);
  cl_device_id devices[num_devices];
  checkErr(clGetContextInfo(context, CL_CONTEXT_DEVICES, context_devices_size,
                            devices, NULL));
  // Arbitrarily choose first matching device
  assert(num_devices > 0);
  device_id = devices[0];
  size_t device_name_size;
  checkErr(
      clGetDeviceInfo(device_id, CL_DEVICE_NAME, 0, NULL, &device_name_size));
  char device_name[device_name_size];
  checkErr(clGetDeviceInfo(device_id, CL_DEVICE_NAME, device_name_size,
                           device_name, NULL));
  CCTK_VInfo(CCTK_THORNSTRING, "Selected device: %s", device_name);
  checkErr(clGetDeviceInfo(device_id, CL_DEVICE_TYPE, sizeof device_type,
                           &device_type, NULL));
  CCTK_VInfo(CCTK_THORNSTRING, "   Device type: %s",
             device_type == CL_DEVICE_TYPE_CPU
                 ? "CPU"
                 : device_type == CL_DEVICE_TYPE_GPU
                       ? "GPU"
                       : device_type == CL_DEVICE_TYPE_ACCELERATOR
                             ? "ACCELERATOR"
                             : NULL);

  // TODO: use clCreateSubdevicesEXT and CL_AFFINITY_DOMAIN_NUMA_EXT
  // to distribute threads across NUMA units. Also create memory
  // objects with USE_HOST_PTR.

  /*** Create execution queue ***********************************************/

  cl_command_queue_properties const command_queue_properties =
// We want an in-order queue:
//    CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE |
#ifdef CL_QUEUE_IMMEDIATE_EXECUTION_ENABLE_INTEL
// We don't want "immediate execution":
//    CL_QUEUE_IMMEDIATE_EXECUTION_ENABLE_INTEL |
#endif
      // We do want profiling
      CL_QUEUE_PROFILING_ENABLE | 0;
  checkErr((queue = clCreateCommandQueue(context, device_id,
                                         command_queue_properties, &errcode),
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
}

void OpenCLDevice::setup_grid(cGH const *restrict const cctkGH) {
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  assert(not have_grid());
  f_have_grid = true;
  assert(have_grid());

  // We can only set up the grid in local mode
  assert(Carpet::is_local_mode());

  /*** Choose looping configuration *****************************************/

  // Vector size
  vector_size[0] = vector_size_x;
  vector_size[1] = vector_size_y;
  vector_size[2] = vector_size_z;
  if (vector_size[0] == 0) {
    checkErr(clGetDeviceInfo(device_id, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE,
                             sizeof vector_size[0], vector_size, NULL));
  }
  if (vector_size[0] == 0) {
    // If double vectors are not supported, try long instead
    checkErr(clGetDeviceInfo(device_id, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG,
                             sizeof vector_size[0], vector_size, NULL));
  }
  if (vector_size[0] == 0) {
    CCTK_WARN(CCTK_WARN_ABORT, "Could not determine preferred vector size");
  }
  CCTK_VInfo(CCTK_THORNSTRING, "Vector size: %2d %2d %2d", vector_size[0],
             vector_size[1], vector_size[2]);

  // Unrolled loops
  unroll_size[0] = unroll_size_x;
  unroll_size[1] = unroll_size_y;
  unroll_size[2] = unroll_size_z;
  CCTK_VInfo(CCTK_THORNSTRING, "Unroll size: %2d %2d %2d", unroll_size[0],
             unroll_size[1], unroll_size[2]);

  // Closely coupled threads (aka OpenCL groups)
  // TODO: use CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE and
  // clGetKernelWorkGroupInfo
  group_size[0] = group_size_x;
  group_size[1] = group_size_y;
  group_size[2] = group_size_z;
  CCTK_VInfo(CCTK_THORNSTRING, "Group size:  %2d %2d %2d", group_size[0],
             group_size[1], group_size[2]);

  // Explicit kernel loops (aka loop tiling)
  tile_size[0] = tile_size_x;
  tile_size[1] = tile_size_y;
  tile_size[2] = tile_size_z;
  CCTK_VInfo(CCTK_THORNSTRING, "Tile size:   %2d %2d %2d", tile_size[0],
             tile_size[1], tile_size[2]);

  // Describe grid structure
  memory_aligned =
      // if we copy, the device memory is allocated independently, and
      // hence is aligned
      mem_model == mm_copy or
      // if the vector size is 1, then we don't need any particular
      // alignment, and hence the device memory is aligned
      (vector_size[0] == 1 and vector_size[1] == 1 and vector_size[2] == 1) or
      // if the host aligns the memory, and the host vector size is a
      // multiple of the device vector size, then the device memory is
      // aligned
      ((VECTORISE and VECTORISE_ALIGNED_ARRAYS) and
       CCTK_REAL_VEC_SIZE % vector_size[0] == 0 and vector_size[1] == 1 and
       vector_size[2] == 1);

  same_padding = true;
  grid.time = cctkGH->cctk_time;
  grid.delta_time = cctkGH->cctk_delta_time;
  grid.iteration = cctkGH->cctk_iteration;
  for (int d = 0; d < dim; ++d) {
    grid.origin_space[d] = cctkGH->cctk_origin_space[d];
    grid.delta_space[d] = cctkGH->cctk_delta_space[d];
    grid.gsh[d] = cctkGH->cctk_gsh[d];
    grid.lbnd[d] = cctkGH->cctk_lbnd[d];
    grid.lsh[d] = cctkGH->cctk_lsh[d];
    int const granularity = vector_size[d];
    int const good_ash = round_up(cctk_ash[d], granularity);
    grid.ash[d] = memory_aligned ? good_ash : cctk_ash[d];
    same_padding &= grid.ash[d] == cctk_ash[d];
    assert(grid.gsh[d] >= 0);
    assert(grid.lbnd[d] >= 0);
    assert(grid.lbnd[d] <= grid.gsh[d]);
    assert(grid.lsh[d] >= 0);
    assert(grid.lbnd[d] + grid.lsh[d] <= grid.gsh[d]);
    assert(grid.ash[d] >= 0);
    assert(grid.lsh[d] <= grid.ash[d]);
  }
  CCTK_VInfo(CCTK_THORNSTRING, "gsh:  %4d %4d %4d", (int)grid.gsh[0],
             (int)grid.gsh[1], (int)grid.gsh[2]);
  CCTK_VInfo(CCTK_THORNSTRING, "lbnd: %4d %4d %4d", (int)grid.lbnd[0],
             (int)grid.lbnd[1], (int)grid.lbnd[2]);
  CCTK_VInfo(CCTK_THORNSTRING, "lsh:  %4d %4d %4d", (int)grid.lsh[0],
             (int)grid.lsh[1], (int)grid.lsh[2]);
  CCTK_VInfo(CCTK_THORNSTRING, "ash:  %4d %4d %4d", (int)grid.ash[0],
             (int)grid.ash[1], (int)grid.ash[2]);
}

extern "C" int OpenCLRunTime_Setup() {
  DECLARE_CCTK_PARAMETERS;

  CCTK_INFO("Setting up OpenCL device");

  assert(not device);
  device = new OpenCLDevice;
  return 0;
}

extern "C" void OpenCLRunTime_SetupDeviceGH(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  CCTK_INFO("Setting up OpenCL device grid hierarchy");

  device->setup_grid(cctkGH);
}

} // namespace OpenCLRunTime
