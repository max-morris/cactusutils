#ifndef DEVICE_H
#define DEVICE_H

// Handle the device, including its memory layout

#include "defs.hh"

#include <cctk.h>

#include <carpet.hh>

#include <cstdlib>



#ifdef __APPLE__
#  include <OpenCL/opencl.h>
#else
#  include <CL/opencl.h>
#endif	



namespace OpenCLRunTime {
  
  
  
  // Convert an OpenCL error code into a string
  char const *error_string(int const error_code);
  
  // Check an OpenCL call for errors
#define checkErr(cmd) checkErr1(cmd, #cmd, __FILE__, __LINE__)
  void checkErr1(cl_int const errcode, char const *const cmd,
                 char const *const file, int const line);
#define checkWarn(cmd) checkWarn1(cmd, #cmd, __FILE__, __LINE__)
  void checkWarn1(cl_int const errcode, char const *const cmd,
                  char const *const file, int const line);
  void checkErr1(cl_int const errcode, char const *const cmd,
                 char const *const file, int const line);
  void checkWarn1(cl_int const errcode, char const *const cmd,
                  char const *const file, int const line);
  
  
  
  // Divide while rounding up
  inline size_t divup(size_t const a, size_t const b)
  {
    return (a+b-1)/b;
  }
  
  
  
  //////////////////////////////////////////////////////////////////////////////
  
  
  
  // Equivalent of cGH for the kernel
  struct grid_t {
    // Doubles first, then ints, to ensure proper alignment
    // Coordinates:
    double origin_space[dim];
    double delta_space[dim];
    double time;
    double delta_time;
    // Grid structure properties:
    int gsh[dim];
    int lbnd[dim];
    int lssh[dim];
    int lsh[dim];
    // Loop settings:
    int lmin[dim];              // loop region
    int lmax[dim];
    int imin[dim];              // active region
    int imax[dim];
  };
  
  
  
  // Out host/device memory model
  enum memory_model_t {
    mm_always_mapped,           // device memory is directly
                                // accessible (not supported by all
                                // devices)
    mm_copy,                    // copy explicitly
    mm_map                      // map the device memory when the host
                                // needs access
  };
  
  struct mem_t {
    cl_mem mem;
    // bool host_valid, device_valid;
  };
  
  // Global data, defining platform, device etc.
  struct OpenCLDevice {
    cl_device_type device_type;
    cl_context context;
    cl_device_id device_id;
    cl_command_queue queue;
    
    memory_model_t mem_model;
    bool memory_aligned;        // device memory is aligned
    bool same_padding;          // host and device have same padding
    
    vector<vector<mem_t> > mems; // [vi][tl]
    
    // point  (smallest unit)
    // vector (same execution path)
    // unroll (unrolled kernel loop)
    // group  (closely coupled threads, sharing cache, "CUDA thread block")
    // tile   (explicit kernel loop)
    // grid   (largest unit, loosely coupled threads, separate caches,
    //         "CUDA grid")
    bool have_grid;
    cl_uint vector_size[dim];
    cl_uint unroll_size[dim];
    cl_uint group_size[dim];
    cl_uint tile_size[dim];
    grid_t grid;
    
    OpenCLDevice();
    void setup_grid(cGH const *restrict const cctkGH);
  };
  
  
  
  // Global variable
  extern OpenCLDevice *device;
  
  
  
} // namespace OpenCLRunTime



#endif  // #ifndef DEVICE_H
