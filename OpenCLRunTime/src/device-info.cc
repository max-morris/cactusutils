#include "defs.hh"
#include "device.hh"

#include <cctk.h>
#include <cctk_Arguments.h>
#include <cctk_Parameters.h>

#include <fstream>
#include <string>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

namespace OpenCLRunTime {

// String
typedef char char_arr[10000];

// Integer vector
struct size_t_arr {
  size_t arr[3];
};
ostream &operator<<(ostream &os, size_t_arr const &arr) {
  for (size_t i = 0; i < sizeof(size_t_arr) / sizeof(size_t); ++i) {
    if (i)
      os << " ";
    os << arr.arr[i];
  }
  return os;
}

// Queue properties
struct command_queue_properties_t {
  cl_command_queue_properties t;
};
ostream &operator<<(ostream &os,
                    command_queue_properties_t const command_queue_properties) {
  if (command_queue_properties.t & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)
    os << "QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE ";
  if (command_queue_properties.t & CL_QUEUE_PROFILING_ENABLE)
    os << "QUEUE_PROFILING_ENABLE ";
#ifdef CL_QUEUE_IMMEDIATE_EXECUTION_ENABLE_INTEL
  if (command_queue_properties.t & CL_QUEUE_IMMEDIATE_EXECUTION_ENABLE_INTEL)
    os << "QUEUE_IMMEDIATE_EXECUTION_ENABLE_INTEL ";
#endif
  return os;
}

// Execution capabilities
struct device_exec_capabilities_t {
  cl_device_exec_capabilities t;
};
ostream &operator<<(ostream &os,
                    device_exec_capabilities_t const device_exec_capabilities) {
  if (device_exec_capabilities.t & CL_EXEC_KERNEL)
    os << "EXEC_KERNEL ";
  if (device_exec_capabilities.t & CL_EXEC_NATIVE_KERNEL)
    os << "EXEC_NATIVE_KERNEL ";
  return os;
}

// Floating point information
struct device_fp_config_t {
  cl_device_fp_config t;
};
ostream &operator<<(ostream &os, device_fp_config_t const device_fp_config) {
  if (device_fp_config.t & CL_FP_DENORM)
    os << "FP_DENORM ";
  if (device_fp_config.t & CL_FP_INF_NAN)
    os << "FP_INF_NAN ";
  if (device_fp_config.t & CL_FP_ROUND_TO_NEAREST)
    os << "FP_ROUND_TO_NEAREST ";
  if (device_fp_config.t & CL_FP_ROUND_TO_ZERO)
    os << "FP_ROUND_TO_ZERO ";
  if (device_fp_config.t & CL_FP_ROUND_TO_INF)
    os << "FP_ROUND_TO_INF ";
  return os;
}

// Local memory type
struct device_local_mem_type_t {
  cl_device_local_mem_type t;
};
ostream &operator<<(ostream &os,
                    device_local_mem_type_t const device_local_mem_type) {
  switch (device_local_mem_type.t) {
  case CL_LOCAL:
    os << "LOCAL";
    break;
  case CL_GLOBAL:
    os << "GLOBAL";
    break;
  default:
    assert(0);
  }
  return os;
}

// Memory cache type
struct device_mem_cache_type_t {
  cl_device_mem_cache_type t;
};
ostream &operator<<(ostream &os,
                    device_mem_cache_type_t const device_mem_cache_type) {
  switch (device_mem_cache_type.t) {
  case CL_NONE:
    os << "NONE";
    break;
  case CL_READ_ONLY_CACHE:
    os << "READ_ONLY_CACHE";
    break;
  case CL_READ_WRITE_CACHE:
    os << "READ_WRITE_CACHE";
    break;
  default:
    assert(0);
  }
  return os;
}

// Device type
struct device_type_t {
  cl_device_type t;
};
ostream &operator<<(ostream &os, device_type_t const device_type) {
  if (device_type.t & CL_DEVICE_TYPE_CPU)
    os << "CPU ";
  if (device_type.t & CL_DEVICE_TYPE_GPU)
    os << "GPU ";
  if (device_type.t & CL_DEVICE_TYPE_ACCELERATOR)
    os << "ACCELERATOR ";
  return os;
}

// Output information nicely formatted
#define PRINT_INFO(os, device, FUNCTION, MACRO, NAME, OFFSET, LENGTH, TYPE)    \
  do {                                                                         \
    string const spaces = "                                        ";          \
    string const macro = string(NAME) + ": " + spaces;                         \
    size_t const begin = macro.find('_', macro.find('_', 0) + 1) + 1;          \
    TYPE val;                                                                  \
    size_t size;                                                               \
    cl_int const errcode1 = FUNCTION(device, MACRO, sizeof val, &val, &size);  \
    if (errcode1 == CL_SUCCESS) {                                              \
      assert(sizeof val >= size);                                              \
      os << spaces.substr(0, OFFSET) << macro.substr(begin, LENGTH + 2) << val \
         << endl;                                                              \
    } else {                                                                   \
      os << spaces.substr(0, OFFSET) << macro.substr(begin, LENGTH + 2)        \
         << "[error " << errcode1 << "]" << endl;                              \
    }                                                                          \
  } while (0)
#define PRINT_UNSP(os, device, FUNCTION, MACRO, NAME, OFFSET, LENGTH, TYPE)    \
  do {                                                                         \
    string const spaces = "                                        ";          \
    string const macro = string(NAME) + ": " + spaces;                         \
    size_t const begin = macro.find('_', macro.find('_', 0) + 1) + 1;          \
    os << spaces.substr(0, OFFSET) << macro.substr(begin, LENGTH + 2)          \
       << "[unsupported]" << endl;                                             \
  } while (0)

// Output information about the OpenCL device
extern "C" void OpenCLRunTime_DeviceInfo(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  stringstream filename;
  filename << out_dir << "/opencl.txt";
  ofstream file(filename.str().c_str());

  // Display some information about the available compute devices
  cl_uint num_platforms;
  checkErr(clGetPlatformIDs(0, NULL, &num_platforms));
  file << "NUM_PLATFORMS:  " << num_platforms << endl;
  vector<cl_platform_id> platforms(num_platforms);
  vector<vector<cl_device_id> > devices(num_platforms);
  checkErr(clGetPlatformIDs(num_platforms, &platforms[0], &num_platforms));
  for (cl_uint plat_ind = 0; plat_ind < num_platforms; ++plat_ind) {
    file << "PLATFORM_INDEX: " << plat_ind << "\n";
    cl_platform_id const platform = platforms[plat_ind];
    file << "   PLATFORM_ID: " << platform << "\n";

#define PRINT_PLATFORM_INFO(MACRO, TYPE)                                       \
  PRINT_INFO(file, platform, clGetPlatformInfo, MACRO, #MACRO, 3, 11, TYPE);
    PRINT_PLATFORM_INFO(CL_PLATFORM_PROFILE, char_arr);
    PRINT_PLATFORM_INFO(CL_PLATFORM_VERSION, char_arr);
    PRINT_PLATFORM_INFO(CL_PLATFORM_NAME, char_arr);
    PRINT_PLATFORM_INFO(CL_PLATFORM_VENDOR, char_arr);
    PRINT_PLATFORM_INFO(CL_PLATFORM_EXTENSIONS, char_arr);

    cl_uint num_devices;
    checkErr(
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices));
    file << "   NUM_DEVICES:  " << num_devices << endl;
    devices[plat_ind].resize(num_devices);
    checkErr(clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, num_devices,
                            &devices[plat_ind][0], &num_devices));
    for (cl_uint dev_ind = 0; dev_ind < num_devices; ++dev_ind) {
      file << "   DEVICE_INDEX: " << dev_ind << "\n";
      cl_device_id const device = devices[plat_ind][dev_ind];
      file << "      DEVICE_ID:                     " << device << "\n";

#define PRINT_DEVICE_INFO(MACRO, TYPE)                                         \
  PRINT_INFO(file, device, clGetDeviceInfo, MACRO, #MACRO, 6, 29, TYPE);
#define PRINT_DEVICE_UNSP(MACRO, TYPE)                                         \
  PRINT_UNSP(file, device, clGetDeviceInfo, MACRO, #MACRO, 6, 29, TYPE);
      PRINT_DEVICE_INFO(CL_DEVICE_TYPE, device_type_t);
      PRINT_DEVICE_INFO(CL_DEVICE_VENDOR_ID, cl_uint);
      PRINT_DEVICE_INFO(CL_DEVICE_MAX_COMPUTE_UNITS, cl_uint);
      PRINT_DEVICE_INFO(CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, cl_uint);
      PRINT_DEVICE_INFO(CL_DEVICE_MAX_WORK_ITEM_SIZES, size_t_arr);
      PRINT_DEVICE_INFO(CL_DEVICE_MAX_WORK_GROUP_SIZE, size_t);
      PRINT_DEVICE_INFO(CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, cl_uint);
      PRINT_DEVICE_INFO(CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, cl_uint);
      PRINT_DEVICE_INFO(CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, cl_uint);
      PRINT_DEVICE_INFO(CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, cl_uint);
#ifdef CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF
      PRINT_DEVICE_INFO(CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, cl_uint);
#else
      PRINT_DEVICE_UNSP(CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, cl_uint);
#endif
      PRINT_DEVICE_INFO(CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, cl_uint);
      PRINT_DEVICE_INFO(CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, cl_uint);
#ifdef CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR
      PRINT_DEVICE_INFO(CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, cl_uint);
#else
      PRINT_DEVICE_UNSP(CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, cl_uint);
#endif
#ifdef CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT
      PRINT_DEVICE_INFO(CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, cl_uint);
#else
      PRINT_DEVICE_UNSP(CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, cl_uint);
#endif
#ifdef CL_DEVICE_NATIVE_VECTOR_WIDTH_INT
      PRINT_DEVICE_INFO(CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, cl_uint);
#else
      PRINT_DEVICE_UNSP(CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, cl_uint);
#endif
#ifdef CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG
      PRINT_DEVICE_INFO(CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, cl_uint);
#else
      PRINT_DEVICE_UNSP(CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, cl_uint);
#endif
#ifdef CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF
      PRINT_DEVICE_INFO(CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, cl_uint);
#else
      PRINT_DEVICE_UNSP(CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, cl_uint);
#endif
#ifdef CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT
      PRINT_DEVICE_INFO(CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, cl_uint);
#else
      PRINT_DEVICE_UNSP(CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, cl_uint);
#endif
#ifdef CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE
      PRINT_DEVICE_INFO(CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE, cl_uint);
#else
      PRINT_DEVICE_UNSP(CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE, cl_uint);
#endif
      PRINT_DEVICE_INFO(CL_DEVICE_MAX_CLOCK_FREQUENCY, cl_uint);
      PRINT_DEVICE_INFO(CL_DEVICE_ADDRESS_BITS, cl_uint);
      PRINT_DEVICE_INFO(CL_DEVICE_MAX_MEM_ALLOC_SIZE, cl_ulong);
      PRINT_DEVICE_INFO(CL_DEVICE_IMAGE_SUPPORT, cl_bool);
      PRINT_DEVICE_INFO(CL_DEVICE_MAX_READ_IMAGE_ARGS, cl_uint);
      PRINT_DEVICE_INFO(CL_DEVICE_MAX_WRITE_IMAGE_ARGS, cl_uint);
      PRINT_DEVICE_INFO(CL_DEVICE_IMAGE2D_MAX_WIDTH, size_t);
      PRINT_DEVICE_INFO(CL_DEVICE_IMAGE2D_MAX_HEIGHT, size_t);
      PRINT_DEVICE_INFO(CL_DEVICE_IMAGE3D_MAX_WIDTH, size_t);
      PRINT_DEVICE_INFO(CL_DEVICE_IMAGE3D_MAX_HEIGHT, size_t);
      PRINT_DEVICE_INFO(CL_DEVICE_IMAGE3D_MAX_DEPTH, size_t);
      PRINT_DEVICE_INFO(CL_DEVICE_MAX_SAMPLERS, cl_uint);
      PRINT_DEVICE_INFO(CL_DEVICE_MAX_PARAMETER_SIZE, size_t);
      PRINT_DEVICE_INFO(CL_DEVICE_MEM_BASE_ADDR_ALIGN, cl_uint);
      PRINT_DEVICE_INFO(CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, cl_uint);
      PRINT_DEVICE_INFO(CL_DEVICE_SINGLE_FP_CONFIG, device_fp_config_t);
#ifdef CL_DEVICE_DOUBLE_FP_CONFIG
      PRINT_DEVICE_INFO(CL_DEVICE_DOUBLE_FP_CONFIG, device_fp_config_t);
#else
      PRINT_DEVICE_UNSP(CL_DEVICE_DOUBLE_FP_CONFIG, device_fp_config_t);
#endif
#ifdef CL_DEVICE_HALF_FP_CONFIG
      PRINT_DEVICE_INFO(CL_DEVICE_HALF_FP_CONFIG, device_fp_config_t);
#else
      PRINT_DEVICE_UNSP(CL_DEVICE_HALF_FP_CONFIG, device_fp_config_t);
#endif
      PRINT_DEVICE_INFO(CL_DEVICE_GLOBAL_MEM_CACHE_TYPE,
                        device_mem_cache_type_t);
      PRINT_DEVICE_INFO(CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, cl_uint);
      PRINT_DEVICE_INFO(CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, cl_ulong);
      PRINT_DEVICE_INFO(CL_DEVICE_GLOBAL_MEM_SIZE, cl_ulong);
      PRINT_DEVICE_INFO(CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, cl_ulong);
      PRINT_DEVICE_INFO(CL_DEVICE_MAX_CONSTANT_ARGS, cl_uint);
      PRINT_DEVICE_INFO(CL_DEVICE_LOCAL_MEM_TYPE, device_local_mem_type_t);
      PRINT_DEVICE_INFO(CL_DEVICE_LOCAL_MEM_SIZE, cl_ulong);
      PRINT_DEVICE_INFO(CL_DEVICE_ERROR_CORRECTION_SUPPORT, cl_bool);
#ifdef CL_DEVICE_HOST_UNIFIED_MEMORY
      PRINT_DEVICE_INFO(CL_DEVICE_HOST_UNIFIED_MEMORY, cl_bool);
#else
      PRINT_DEVICE_UNSP(CL_DEVICE_HOST_UNIFIED_MEMORY, cl_bool);
#endif
      PRINT_DEVICE_INFO(CL_DEVICE_PROFILING_TIMER_RESOLUTION, size_t);
      PRINT_DEVICE_INFO(CL_DEVICE_ENDIAN_LITTLE, cl_bool);
      PRINT_DEVICE_INFO(CL_DEVICE_AVAILABLE, cl_bool);
      PRINT_DEVICE_INFO(CL_DEVICE_COMPILER_AVAILABLE, cl_bool);
      PRINT_DEVICE_INFO(CL_DEVICE_EXECUTION_CAPABILITIES,
                        device_exec_capabilities_t);
      PRINT_DEVICE_INFO(CL_DEVICE_QUEUE_PROPERTIES, command_queue_properties_t);
      PRINT_DEVICE_INFO(CL_DEVICE_PLATFORM, cl_platform_id);
      PRINT_DEVICE_INFO(CL_DEVICE_NAME, char_arr);
      PRINT_DEVICE_INFO(CL_DEVICE_VENDOR, char_arr);
      PRINT_DEVICE_INFO(CL_DRIVER_VERSION, char_arr);
      PRINT_DEVICE_INFO(CL_DEVICE_PROFILE, char_arr);
      PRINT_DEVICE_INFO(CL_DEVICE_VERSION, char_arr);
#ifdef CL_DEVICE_OPENCL_C_VERSION
      PRINT_DEVICE_INFO(CL_DEVICE_OPENCL_C_VERSION, char_arr);
#else
      PRINT_DEVICE_UNSP(CL_DEVICE_OPENCL_C_VERSION, char_arr);
#endif
      PRINT_DEVICE_INFO(CL_DEVICE_EXTENSIONS, char_arr);

    } // for dev_ind

  } // for plat_ind

  file.close();
}

} // namespace OpenCLRunTime
