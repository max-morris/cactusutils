#ifndef KERNEL_H
#define KERNEL_H

#include "defs.h"
#include "device.h"

#include <vector>

#ifdef __APPLE__
#  include <OpenCL/opencl.h>
#else
#  include <CL/opencl.h>
#endif	



namespace OpenCLRunTime {
  
  
  
  struct OpenCLKernel {
    char const *name;
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
    
    OpenCLKernel(cGH const *const cctkGH,
                 char const *const thorn,
                 char const *const name,
                 char const *const sources[],
                 char const *const groups[],
                 int const varindices[],
                 int const timelevels[],
                 char const *const aliases[],
                 int const nvars);
    
    void call(cGH const *const cctkGH,
              int const imin[],
              int const imax[]);
  };
  
  
  
} // namespace OpenCLRunTime



#endif  // #ifndef KERNEL_H
