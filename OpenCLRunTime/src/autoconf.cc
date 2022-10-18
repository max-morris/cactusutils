#include "device.hh"

#include <cctk.h>
#include <cctk_Arguments.h>
#include <cctk_Parameters.h>

#include <fstream>
#include <string>

extern "C" char const *const
    OpenCL_source_OpenCLRunTime_test_attribute_opencl_unroll_hint;
extern "C" char const *const OpenCL_source_OpenCLRunTime_test_attribute_unused;
extern "C" char const *const OpenCL_source_OpenCLRunTime_test_builtin_expect;
extern "C" char const *const OpenCL_source_OpenCLRunTime_test_opencl;
extern "C" char const *const OpenCL_source_OpenCLRunTime_test_pragma_unroll;

extern "C" void OpenCLRunTime_Autoconf(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  cl_int errcode;

  CCTK_INFO("Testing for OpenCL features");

  struct test_t {
    char const *name;        // test name
    char const *description; // human-readable test description
    char const *macro;       // feature macro
    char const *source;      // source code
  };

  test_t const tests[] = {
      {"test_opencl", "whether OpenCL works", "HAVE_OPENCL",
       OpenCL_source_OpenCLRunTime_test_opencl},
      {"test_attribute_opencl_unroll_hint",
       "for __attribute__((opencl_unroll_hint))",
       "HAVE_ATTRIBUTE_OPENCL_UNROLL_HINT",
       OpenCL_source_OpenCLRunTime_test_attribute_opencl_unroll_hint},
      {"test_attribute_unused", "for __attribute__((unused))",
       "HAVE_ATTRIBUTE_UNUSED",
       OpenCL_source_OpenCLRunTime_test_attribute_unused},
      {"test_builtin_expect", "for __builtin_expect", "HAVE_BUILTIN_EXPECT",
       OpenCL_source_OpenCLRunTime_test_builtin_expect},
      {"test_pragma_unroll", "for #pragma unroll", "HAVE_PRAGMA_UNROLL",
       OpenCL_source_OpenCLRunTime_test_pragma_unroll}};
  int const ntests = sizeof tests / sizeof *tests;

  for (int test = 0; test < ntests; ++test) {

    cerr.flush();
    cout.flush();
    cout << "Testing " << tests[test].description << "...";
    cout.flush();

    // Try to build source
    char const *source[] = {tests[test].source};
    cl_program program;
    checkErr((program = clCreateProgramWithSource(device->context, 1, source,
                                                  NULL, &errcode),
              errcode));
    clBuildProgram(program, 1, &device->device_id, opencl_options, NULL, NULL);

    // Output build log
    size_t log_size;
    checkErr(clGetProgramBuildInfo(program, device->device_id,
                                   CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size));
    char build_log[log_size];
    checkErr(clGetProgramBuildInfo(program, device->device_id,
                                   CL_PROGRAM_BUILD_LOG, log_size, build_log,
                                   NULL));
    stringstream filename;
    filename << out_dir << "/" << tests[test].name << ".log";
    ofstream file(filename.str().c_str());
    file << build_log;
    file.close();

    // Check for build problems
    cl_build_status build_status;
    checkErr(clGetProgramBuildInfo(program, device->device_id,
                                   CL_PROGRAM_BUILD_STATUS, sizeof build_status,
                                   &build_status, NULL));
    cerr.flush();
    cout.flush();
    if (build_status == CL_BUILD_SUCCESS) {
      device->autoconf_options += " -D";
      cout << " yes\n";
    } else {
      device->autoconf_options += " -U";
      cout << " no\n";
    }
    device->autoconf_options += tests[test].macro;
    cout.flush();

    // Abort if things look far too bad
    if (tests[test].source == OpenCL_source_OpenCLRunTime_test_opencl and
        build_status != CL_BUILD_SUCCESS) {
      CCTK_ERROR("Cannot execute trivial OpenCL programs");
    }

  } // for test
}
