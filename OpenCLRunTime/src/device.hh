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
  
  
  
  // Out host/device memory model
  enum memory_model_t {
    mm_always_mapped,           // device memory is directly
                                // accessible (not supported by all
                                // devices)
    mm_copy,                    // we copy explicitly
    mm_map                      // we map the device memory when the
                                // host needs access (not yet
                                // supported)
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
    
    vector<vector<cl_mem> > mems;  // [vi][tl]
    vector<bool> mem_host_valid;   // host copy is valid
    vector<bool> mem_device_valid; // device copy is valid
    
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
