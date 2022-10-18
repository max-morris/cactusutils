#include "kernel.hh"
#include "copy.hh"

#include "cctk.h"
#include "cctk_Arguments.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <map>

namespace OpenCLRunTime {

extern "C" char const *const OpenCL_source_OpenCLRunTime_OpenCLMacros;

list<OpenCLKernel *> OpenCLKernel::kernels;

OpenCLKernel::OpenCLKernel(cGH const *const cctkGH, char const *const thorn,
                           char const *const name_, char const *const sources[],
                           char const *const groups[], int const varindices[],
                           int const timelevels[], char const *const aliases[],
                           int const nvars) {
  DECLARE_CCTK_PARAMETERS;

  cl_int errcode;

  assert(cctkGH);
  assert(name_);
  assert(sources);

  if (not CCTK_IsThornActive(CCTK_THORNSTRING)) {
    CCTK_WARN(CCTK_WARN_ABORT, "Error: Thorn " CCTK_THORNSTRING
                               " has been called without being activated");
  }

  assert(device);
  assert(device->have_grid());

  name = strdup(name_);

  kernels.push_back(this);

  setup_args(cctkGH, groups, varindices, timelevels, aliases, nvars);

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
  for (int param_type_idx = 0; param_type_idx < 2; ++param_type_idx) {
    int const param_type = param_types[param_type_idx];

    int first = 1;
    while (true) {

      cParamData const *data;
      int const istat = CCTK_ParameterWalk(first, thorn, NULL, &data);
      assert(istat >= 0);
      if (istat > 0)
        break;
      assert(data->array_size == 0);
      first = 0;

      // Ignore all types except one in this param_type iteration
      if (data->type != param_type)
        continue;

      void const *const paramval =
          CCTK_ParameterGet(data->name, data->thorn, NULL);

      string type_name;
      int type_size;
      stringstream value_buf;
      switch (data->type) {
      case PARAMETER_INT:
        type_name = "CCTK_INT";
        type_size = sizeof(CCTK_INT);
        value_buf << *static_cast<CCTK_INT const *>(paramval);
        break;
      case PARAMETER_REAL:
        type_name = "CCTK_REAL";
        type_size = sizeof(CCTK_REAL);
        value_buf << setprecision(17)
                  << *static_cast<CCTK_REAL const *>(paramval);
        break;
      default:
        assert(0);
      }
      string const value_str = value_buf.str();

      paramdecls << "  " << type_name << " " << data->name << ";\n";

#if 0
        paramdefs << " \\\n"
                  << "  " << type_name << " const " <<  data->name
                  << " CCTK_ATTRIBUTE_UNUSED = "
                  << "cctk_parameters->" << data->name << ";";
#else
      // Expand parameter values in-line
      // TODO: This breaks if parameter values change and the kernel
      // is not rebuilt
      paramdefs << " \\\n"
                << "  " << type_name << " const " << data->name
                << " CCTK_ATTRIBUTE_UNUSED = " << value_str << ";";
#endif

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

  assert(sources[0]);
  assert(sources[1]);
  assert(not sources[2]);

  stringstream buf;
  buf << "// -*-C-*-\n"
      << "\n"
      << "// Code generation choices:\n"
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
      << OpenCL_source_OpenCLRunTime_OpenCLMacros << "\n"
      << "// Cactus parameters:\n"
#if 0
        << paramdecls.str()
#endif
      << paramdefs.str() << "\n"
      << "// Kranc's FD operators:\n" << sources[0] << "\n"
      << "// Kernel Function:\n"
      << "__kernel\n"
      << "__attribute__((vec_type_hint(CCTK_REAL_VEC)))\n"
      << ("__attribute__((reqd_work_group_size"
          "(GROUP_SIZE_I, GROUP_SIZE_J, GROUP_SIZE_K)))\n") << "void " << name
      << "\n"
      << " (cGH __constant *restrict const cctkGH";
#if 0
        << " (cGH __constant *restrict const cctkGH,\n";
        << "   cctk_parameters_t __constant *restrict const cctk_parameters";
#endif
  // Cactus grid functions
  // TODO: declare read-only grid functions as const
  for (int arg = 0; arg < int(args.size()); ++arg) {
    buf << ",\n"
        << "   CCTK_REAL __global *restrict const " << args[arg].alias;
  }
  buf << ")\n"
      << "{\n"
      << "  DECLARE_CCTK_ARGUMENTS;\n"
      << "  DECLARE_CCTK_PARAMETERS;\n"
      << "\n"
      << "// BEGIN USER-DEFINED KERNEL\n" << sources[1] // user's kernel code
      << "// END USER-DEFINED KERNEL\n"
      << "}\n";
  string const sbuf = buf.str();

  // Output source
  {
    stringstream filename;
    filename << out_dir << "/" << name << ".cl";
    ofstream file(filename.str().c_str());
    file << sbuf;
    file.close();
  }

  /*** Build program from source ********************************************/

  char const *source[] = {sbuf.c_str()};

  checkErr((program = clCreateProgramWithSource(device->context, 1, source,
                                                NULL, &errcode),
            errcode));

  // Ignore build errors
  checkWarn(clBuildProgram(program, 1, &device->device_id, opencl_options, NULL,
                           NULL));
  size_t log_size;
  checkErr(clGetProgramBuildInfo(program, device->device_id,
                                 CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size));
  char build_log[log_size];
  checkErr(clGetProgramBuildInfo(program, device->device_id,
                                 CL_PROGRAM_BUILD_LOG, log_size, build_log,
                                 NULL));

  // Output log
  {
    stringstream filename;
    filename << out_dir << "/" << name << ".log";
    ofstream file(filename.str().c_str());
    file << build_log;
    file.close();
  }

  cl_build_status build_status;
  checkErr(clGetProgramBuildInfo(program, device->device_id,
                                 CL_PROGRAM_BUILD_STATUS, sizeof build_status,
                                 &build_status, NULL));
  if (veryverbose) {
    CCTK_VInfo(CCTK_THORNSTRING, "Build status: %s",
               build_status == CL_BUILD_NONE
                   ? "none"
                   : build_status == CL_BUILD_ERROR
                         ? "error"
                         : build_status == CL_BUILD_SUCCESS
                               ? "success"
                               : build_status == CL_BUILD_IN_PROGRESS
                                     ? "in_progress"
                                     : NULL);
  }
  if (build_status == CL_BUILD_ERROR) {
    cout << build_log;
    CCTK_VError(__LINE__, __FILE__, CCTK_THORNSTRING,
                "Build error; see %s/%s.log for details.", out_dir, name);
  }

  // Create kernel from the program
  checkErr((kernel = clCreateKernel(program, name, &errcode), errcode));

  // Output object code
  size_t binary_sizes_size;
  checkErr(clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, 0, NULL,
                            &binary_sizes_size));
  size_t const num_devices = binary_sizes_size / sizeof(size_t);
  vector<size_t> binary_sizes(num_devices);
  checkErr(clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES,
                            num_devices * sizeof(size_t), &binary_sizes.front(),
                            NULL));
  vector<vector<char> > binaries(num_devices);
  vector<char *> binary_ptrs(num_devices);
  for (size_t n = 0; n < num_devices; ++n) {
    binaries.at(n).resize(binary_sizes.at(n));
    binary_ptrs.at(n) = &binaries.at(n).front();
  }
  checkErr(clGetProgramInfo(program, CL_PROGRAM_BINARIES,
                            num_devices * sizeof binary_ptrs[0],
                            &binary_ptrs[0], NULL));
  for (size_t n = 0; n < num_devices; ++n) {
    stringstream filename;
    filename << out_dir << "/" << name << "." << n << ".o";
    ofstream file(filename.str().c_str(), ofstream::binary);
    file.write(binary_ptrs.at(n), binary_sizes.at(n));
    file.close();
  }

  // Output disassembled listing
  disassemble();

  /*** Create buffer for cGH structure for this kernel **********************/

  // TODO: Use the same cGH structure for all kernels
  checkErr((mem_grid = clCreateBuffer(device->context,
                                      CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                      sizeof grid, &grid, &errcode),
            errcode));

#if 0
    /*** Set up parameters for calling the kernel *****************************/
    
    checkErr((mem_params =
              clCreateBuffer(device->context,
                             CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY,
                             paramvalues.size(), &paramvalues.front(),
                             &errcode),
              errcode));
#endif
}

/*** Determine arguments for calling the kernel *****************************/

void OpenCLKernel::setup_args(cGH const *const cctkGH,
                              char const *const groups[],
                              int const varindices[], int const timelevels[],
                              char const *const aliases[], int const nvars) {
  args.clear();

  if (nvars == -1) {
    // Determine arguments by group name

    assert(groups);
    assert(not varindices);
    assert(not timelevels);
    assert(not aliases);

    for (int group = 0; groups[group]; ++group) {
      int const gi = CCTK_GroupIndex(groups[group]);
      assert(gi >= 0);
      int const nv = CCTK_NumVarsInGroupI(gi);
      assert(nv >= 0);
      if (nv > 0) {
        int const v0 = CCTK_FirstVarIndexI(gi);
        assert(v0 >= 0);
        int const num_tl = CCTK_ActiveTimeLevelsGI(cctkGH, gi);
        assert(num_tl >= 0);
        for (int vi = v0; vi < v0 + nv; ++vi) {
          string alias(CCTK_VarName(vi));
          for (int tl = 0; tl < num_tl; ++tl) {
            OpenCLKernel::arg_t const arg = {vi, tl, alias};
            args.push_back(arg);
            alias += "_p";
          }
        }
      }
    }

  } else {
    // Arguments are given explicitly

    assert(nvars >= 0);
    assert(not groups);
    assert(varindices);
    assert(timelevels);
    assert(aliases);

    args.reserve(nvars);
    for (int var = 0; var < nvars; ++var) {
      int const vi = varindices[var];
      assert(vi >= 0 and vi < CCTK_NumVars());
      int const tl = timelevels[var];
      assert(tl >= 0);
      OpenCLKernel::arg_t const arg = {vi, tl, aliases[var]};
      args.push_back(arg);
    }
    assert(int(args.size()) == nvars);
  }

  for (vector<OpenCLKernel::arg_t>::const_iterator argi = args.begin(),
                                                   arge = args.end();
       argi != arge; ++argi) {
    int const vi = argi->vi;
    int const tl = argi->tl;
    int const gi = CCTK_GroupIndexFromVarI(vi);
    if (CCTK_GroupTypeI(gi) != CCTK_GF) {
      char *const group_name = CCTK_GroupName(gi);
      CCTK_VWarn(CCTK_WARN_ABORT, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "OpenCL kernel %s uses the grid variable group %s, which is "
                 "not a grid function",
                 name, group_name);
      free(group_name);
    }
    if (CCTK_VarTypeI(vi) != CCTK_VARIABLE_REAL) {
      char *const group_name = CCTK_GroupName(gi);
      CCTK_VWarn(CCTK_WARN_ABORT, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "OpenCL kernel %s uses the grid variable group %s, which is "
                 "not of variable type CCTK_REAL",
                 name, group_name);
      free(group_name);
    }
    if (tl >= CCTK_ActiveTimeLevelsGI(cctkGH, gi)) {
      char *const group_name = CCTK_GroupName(gi);
      CCTK_VWarn(CCTK_WARN_ABORT, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "OpenCL kernel %s uses timelevel %d of the grid variable "
                 "group %s, which does not exist",
                 name, tl, group_name);
      free(group_name);
    }
  }
}

void OpenCLKernel::call(cGH const *const cctkGH, int const imin[],
                        int const imax[]) {
  DECLARE_CCTK_PARAMETERS;

  assert(device);
  assert(device->have_grid());

  // Set up grid description
  grid = device->grid;

  /*** Set up arguments for calling the kernel ******************************/

  size_t arg_count = 0;

  checkErr(clSetKernelArg(kernel, arg_count++, sizeof mem_grid, &mem_grid));

#if 0
    checkErr(clSetKernelArg(kernel, arg_count++,
                            sizeof mem_params, &mem_params));
#endif

  for (size_t arg = 0; arg < args.size(); ++arg) {
    int const vi = args[arg].vi;
    int const tl = args[arg].tl;
    if (tl >= int(device->mems.at(vi).size())) {
      char *const fullname = CCTK_FullName(vi);
      CCTK_VWarn(CCTK_WARN_ABORT, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "Variable %s/%d does not exist on the device", fullname, tl);
      free(fullname);
    }

    cl_mem const *memptr;
    // We don't know which arguments are read and which are written,
    // so we and only make assumptions about accessing past
    // timelevels if both flags are set
    if (only_reads_current_timelevel and only_writes_current_timelevel) {
      // Only the current timelevel will be accessed. The current
      // timelevel must be known. Other timelevels are set to null
      // pointers.
      if (tl == 0) {
        memptr = &device->mems.at(vi).at(tl).mem;
      } else {
        memptr = NULL;
      }
    } else if (only_reads_current_timelevel or only_writes_current_timelevel) {
      // All timelevels may be accessed. The current timelevel must
      // be known, other timelevels may or may not be known. Unknown
      // timelevels will be set to null pointers.
      if (tl == 0) {
        memptr = &device->mems.at(vi).at(tl).mem;
      } else {
        if (tl < int(device->mems.at(vi).size())) {
          memptr = &device->mems.at(vi).at(tl).mem;
        } else {
          memptr = NULL;
        }
      }
    } else {
      // All timelevels may be accessed, and all timelevels must be
      // known. No null pointers will be passed.
      memptr = &device->mems.at(vi).at(tl).mem;
    }

    // It seems that passing NULL doesn't work will all OpenCL
    // implementations (although documented). We pass a pointer to
    // timelevel 0 instead.
    if (not memptr) {
      memptr = &device->mems.at(vi).at(0).mem;
    }

    checkErr(clSetKernelArg(kernel, arg_count++, sizeof *memptr, memptr));
  }

  assert(arg_count == args.size() + 1);

  /*** Prepare cGH structure ************************************************/

  for (int d = 0; d < dim; ++d) {
    grid.imin[d] = imin[d];
    grid.imax[d] = imax[d];
    assert(grid.imin[d] >= 0);
    assert(grid.imax[d] <= grid.lsh[d]);
    assert(grid.imin[d] <= grid.imax[d]);
  }

  if (veryverbose) {
    CCTK_VInfo(CCTK_THORNSTRING, "Looping region minimum: %4d %4d %4d",
               (int)imin[0], (int)imin[1], (int)imin[2]);
    CCTK_VInfo(CCTK_THORNSTRING, "Looping region maximum: %4d %4d %4d",
               (int)imax[0], (int)imax[1], (int)imax[2]);
  }

#if 0
    for (int d=0; d<dim; ++d) {
      // alignment of lower bound
      int const align_lo = device->vector_size[d];
      // alignment of region size
      int const align_sz = device->vector_size[d] * device->unroll_size[d];
      grid.lmin[d] = grid.imin[d] / align_lo * align_lo;
      grid.lmax[d] =
        grid.lmin[d] + round_up(grid.imax[d] - grid.lmin[d], align_sz);
    }
    for (int d=0; d<dim; ++d) {
      assert(grid.lmin[d] >= 0);
      assert(grid.imin[d] >= grid.lmin[d]);
      assert(grid.imax[d] <= grid.lmax[d]);
      assert(grid.lmax[d] <= grid.lsh[d]);
      assert(grid.lmin[d] % device->vector_size[d] == 0);
      assert((grid.lmax[d] - grid.lmin[d]) %
             (device->vector_size[d] * device->unroll_size[d]) == 0);
    }
#endif

  grid.time = cctkGH->cctk_time;
  grid.delta_time = cctkGH->cctk_delta_time;
  grid.iteration = cctkGH->cctk_iteration;

  static int timer_grid_copy = -1;
  if (veryverbose) {
    if (timer_grid_copy < 0) {
      timer_grid_copy = CCTK_TimerCreate("OpenCLRunTime::grid::copy");
      assert(timer_grid_copy >= 0);
    }
    CCTK_TimerStartI(timer_grid_copy);
  }
  // We use a blocking write because the next call for the same
  // kernel may have a different grid configuration, and we then
  // overwrite the host memory where this is stored.
  checkErr(clEnqueueWriteBuffer(device->queue, mem_grid, CL_TRUE, 0,
                                sizeof grid, &grid, 0, NULL, NULL));
  if (veryverbose) {
    CCTK_TimerStopI(timer_grid_copy);
    CCTK_TimerPrintDataI(timer_grid_copy, -1);
  }

  // Calculate number of thread groups

  size_t local_work_size[dim];
  for (int d = 0; d < dim; ++d) {
    local_work_size[d] = device->group_size[d];
  }
  if (veryverbose) {
    CCTK_VInfo(CCTK_THORNSTRING, "Local work group size:  %4d %4d %4d",
               (int)local_work_size[0], (int)local_work_size[1],
               (int)local_work_size[2]);
  }

  size_t global_work_size[dim];
  for (int d = 0; d < dim; ++d) {
    int const lmin = round_down(grid.imin[d], device->vector_size[d] *
                                                  device->unroll_size[d]);
    global_work_size[d] =
        div_up(grid.imax[d] - lmin,
               device->vector_size[d] * device->unroll_size[d] *
                   device->group_size[d] * device->tile_size[d]) *
        local_work_size[d];
  }
  if (veryverbose) {
    CCTK_VInfo(CCTK_THORNSTRING, "Global work group size: %4d %4d %4d",
               (int)global_work_size[0], (int)global_work_size[1],
               (int)global_work_size[2]);
  }

// Finish the queue before starting the timer
#if 1
  checkErr(clFinish(device->queue));
#endif

  static int timer_kernel_enqueue = -1;
  if (veryverbose) {
    if (timer_kernel_enqueue < 0) {
      timer_kernel_enqueue = CCTK_TimerCreate("OpenCLRunTime::kernel::enqueue");
      assert(timer_kernel_enqueue >= 0);
    }
  }

  static map<string, int> timer_kernels;
  int this_timer = -1;
  if (veryverbose) {
    string const name_string = name;
    map<string, int>::iterator const iter = timer_kernels.find(name_string);
    if (iter != timer_kernels.end()) {
      this_timer = iter->second;
    } else {
      string const timer_name =
          string("OpenCLRunTime::kernel::enqueue::") + name_string;
      this_timer = CCTK_TimerCreate(timer_name.c_str());
      assert(this_timer >= 0);
      timer_kernels.insert(timer_kernels.begin(),
                           pair<string, int>(name_string, this_timer));
    }
  }

  if (veryverbose) {
    CCTK_TimerStartI(timer_kernel_enqueue);
    CCTK_TimerStartI(this_timer);
  }

  // Queue a single execution of the kernel
  cl_event event;
  checkErr(clEnqueueNDRangeKernel(device->queue, kernel, dim, NULL,
                                  global_work_size, local_work_size, 0, NULL,
                                  &event));
  events.push_back(event);

// This finish prevents nans on the outer boundary when running on
// multiple processes (with the Intel OpenCL implementation). I
// don't know why it is necessary. The nans appear randomly, so
// this is probably a timing issue.
#if 1
  checkErr(clFinish(device->queue));
#endif

  if (veryverbose) {
    CCTK_TimerStopI(this_timer);
    CCTK_TimerStopI(timer_kernel_enqueue);
    CCTK_TimerPrintDataI(this_timer, -1);
    CCTK_TimerPrintDataI(timer_kernel_enqueue, -1);
  }
}

//////////////////////////////////////////////////////////////////////////////

void OpenCLRunTime_CallKernel(cGH const *const cctkGH, char const *const thorn,
                              char const *const name,
                              char const *const sources[],
                              char const *const groups[],
                              int const varindices[], int const timelevels[],
                              char const *const aliases[], int const nvars,
                              int const imin[], int const imax[],
                              OpenCLKernel **const pkernel) {
  DECLARE_CCTK_PARAMETERS;

  assert(cctkGH);
  assert(pkernel);

  // If this is the first call for this kernel, build the kernel and
  // set up all kernel data structures
  if (not*pkernel) {
    CCTK_VInfo(CCTK_THORNSTRING, "Setting up OpenCL kernel %s", name);
    *pkernel = new OpenCLKernel(cctkGH, thorn, name, sources, groups,
                                varindices, timelevels, aliases, nvars);
  } else {
    (*pkernel)
        ->setup_args(cctkGH, groups, varindices, timelevels, aliases, nvars);
  }

  if (veryverbose) {
    CCTK_VInfo(CCTK_THORNSTRING, "Enqueuing OpenCL kernel %s", name);
  }
  (*pkernel)->call(cctkGH, imin, imax);
}

struct stats_t {
  CCTK_REAL count, sum, sum2, minval, maxval;
  stats_t()
      : count(-1.0), sum(0.0), sum2(0.0),
        minval(numeric_limits<CCTK_REAL>::max()), maxval(0.0) {}
  CCTK_REAL get_count() const { return count; }
  CCTK_REAL get_min() const {
    return minval == numeric_limits<CCTK_REAL>::max() ? -1.0 : minval;
  }
  CCTK_REAL get_max() const { return maxval; }
  CCTK_REAL get_avg() const { return count <= 0.0 ? -1.0 : sum / count; }
  CCTK_REAL get_sdv() const {
    return count <= 0.0 ? -1.0 : sqrt(sum2 / count - pow(get_avg(), 2));
  }
  void insert(CCTK_REAL const &val) {
    if (count < 0.0) {
      // Ignore the first call, because this presumably includes a
      // significant amount of one-time setup overhead
      ++count;
    } else {
      ++count;
      sum += val;
      sum2 += pow(val, 2);
      minval = min(minval, val);
      maxval = max(maxval, val);
    }
  }
};

void OpenCLKernel::statistics(cGH const *const cctkGH) {
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  assert(device);
  // assert(device->have_grid());

  // Finish, because we are done
  checkErr(clFinish(device->queue));

  streamsize const oldprecision = cout.precision();

  double const nano = 1.0e-9; // one nanosecond in seconds

  if (verbose) {
    cout << "OpenCL detailed profiling info (times in seconds):\n";
    cout << "   " << setw(12) << "Wait"
         << "   " << setw(12) << "Startup"
         << "   " << setw(12) << "Run"
         << "   "
         << "Name\n";
    for (list<OpenCLKernel *>::const_iterator ki = kernels.begin();
         ki != kernels.end(); ++ki) {
      OpenCLKernel const &kernel = **ki;

      for (list<cl_event>::const_iterator ei = kernel.events.begin();
           ei != kernel.events.end(); ++ei) {
        cl_event const &event = *ei;

        cl_ulong queued, submit, start, end;
        size_t size;
        checkErr(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_QUEUED,
                                         sizeof queued, &queued, &size));
        checkErr(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_SUBMIT,
                                         sizeof submit, &submit, &size));
        checkErr(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START,
                                         sizeof start, &start, &size));
        checkErr(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END,
                                         sizeof end, &end, &size));
        cl_ulong const wait_time = submit - queued;
        cl_ulong const startup_time = start - submit;
        cl_ulong const run_time = end - start;
        cout << "   " << setw(12) << setprecision(9) << nano * wait_time
             << "   " << setw(12) << setprecision(9) << nano * startup_time
             << "   " << setw(12) << setprecision(9) << nano * run_time << "   "
             << kernel.name << "\n";
      }
      cout << "\n";
    }
  }

  cout << "OpenCL summary profiling info (times in seconds):\n";
  cout << "   " << setw(6) << "Count"
       << "   " << setw(10) << "Average"
       << "   " << setw(10) << "Std.Dev."
       << "   " << setw(10) << "Minimum"
       << "   " << setw(10) << "Maximum"
       << "   "
       << "Name"
       << "\n";
  for (list<OpenCLKernel *>::const_iterator ki = kernels.begin();
       ki != kernels.end(); ++ki) {
    OpenCLKernel const &kernel = **ki;

    stats_t stats;
    for (list<cl_event>::const_iterator ei = kernel.events.begin();
         ei != kernel.events.end(); ++ei) {
      cl_event const &event = *ei;

      cl_ulong start, end;
      size_t size;
      checkErr(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START,
                                       sizeof start, &start, &size));
      checkErr(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END,
                                       sizeof end, &end, &size));
      cl_ulong const run_time = end - start;
      stats.insert(run_time);
    }
    cout << "   " << setw(6) << setprecision(0) << stats.get_count() << "   "
         << setw(10) << setprecision(6) << nano * stats.get_avg() << "   "
         << setw(10) << setprecision(6) << nano * stats.get_sdv() << "   "
         << setw(10) << setprecision(6) << nano * stats.get_min() << "   "
         << setw(10) << setprecision(6) << nano * stats.get_max() << "   "
         << kernel.name << "\n";
  }

  cout.precision(oldprecision);
  cout.flush();
}

extern "C" void OpenCLRunTime_Statistics(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS;
  OpenCLKernel::statistics(cctkGH);
}

} // namespace OpenCLRunTime
