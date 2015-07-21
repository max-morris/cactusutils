#ifndef KERNEL_H
#define KERNEL_H

// Handle kernels and their arguments

#include "defs.hh"
#include "device.hh"

#include <vector>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

namespace OpenCLRunTime {

// Define a kernel
struct OpenCLKernel {
  static list<OpenCLKernel *> kernels;

  char const *name;
  cl_program program;

  // Arguments
  struct arg_t {
    int vi, tl;
    string alias;
  };
  vector<arg_t> args;

  cl_kernel kernel;

  list<cl_event> events;

  grid_t grid;
  cl_mem mem_grid;
#if 0
    cl_mem mem_params;
#endif

  OpenCLKernel(cGH const *const cctkGH, char const *const thorn,
               char const *const name, char const *const sources[],
               char const *const groups[], int const varindices[],
               int const timelevels[], char const *const aliases[],
               int const nvars);

  void setup_args(cGH const *const cctkGH, char const *const groups[],
                  int const varindices[], int const timelevels[],
                  char const *const aliases[], int const nvars);

  void call(cGH const *const cctkGH, int const imin[], int const imax[]);

  void disassemble() const;

  static void statistics(cGH const *const cctkGH);
};

} // namespace OpenCLRunTime

#endif // #ifndef KERNEL_H
