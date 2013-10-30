#include <cctk.h>
#include <cctk_Arguments.h>
#include <cctk_Parameters.h>

#undef VECTORISE_STREAMING_STORES
#define VECTORISE_STREAMING_STORES 1
#include <vectors.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace std;



#ifdef HAVE_CAPABILITY_MPI
#  include <mpi.h>
#else
namespace {
  typedef int MPI_Comm;
  enum { MPI_DOUBLE, MPI_INT } MPI_Datatype;
  enum { MPI_SUM } MPI_Op;
  int MPI_Barrier(MPI_Comm) { return 0; }
  int MPI_Bcast(void*,int,MPI_Datatype,int,MPI_Comm) { return 0; }
  int MPI_Comm_free(MPI_Comm*) { return 0; }
  int MPI_Comm_split(MPI_Comm,int,int,MPI_Comm*) { return 0; }
  int MPI_Allreduce(void* sendbuf, void* recvbuf, int count,
                    MPI_Datatype datatype, MPI_Op, MPI_Comm)
  {
    switch (datatype) {
    case MPI_DOUBLE: memcpy(recvbuf, sendbuf, count*sizeof(double)); break;
    case MPI_INT: memcpy(recvbuf, sendbuf, count*sizeof(int)); break;
    default: CCTK_BUILTIN_UNREACHABLE();
    }
    return 0;
  }
  int MPI_Reduce(void* sendbuf, void* recvbuf, int count,
                 MPI_Datatype datatype, MPI_Op, int, MPI_Comm)
  {
    switch (datatype) {
    case MPI_DOUBLE: memcpy(recvbuf, sendbuf, count*sizeof(double)); break;
    case MPI_INT: memcpy(recvbuf, sendbuf, count*sizeof(int)); break;
    default: CCTK_BUILTIN_UNREACHABLE();
    }
    return 0;
  }
}
#endif



#ifdef _OPENMP
#  include <omp.h>
#else
#  include <sys/time.h>
namespace {
  int omp_get_max_threads() { return 1; }
  // Fall back to gettimeofday if OpenMP is not available
  double omp_get_wtime()
  {
    timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + 1.0e-6 * tv.tv_usec;
  }
}
#endif



namespace {
  double mpi_average(MPI_Comm comm, double x)
  {
    double sum;
    MPI_Allreduce(&x, &sum, 1, MPI_DOUBLE, MPI_SUM, comm);
    int size;
    MPI_Comm_size(comm, &size);
    return sum / size;
  }
}



namespace {
  
  // Information about MPI and processes, as obtained from hwloc
  
  struct mpi_info_t {
    int mpi_num_procs, mpi_proc_num;
    int mpi_num_hosts, mpi_host_num;
    int mpi_num_procs_on_host, mpi_proc_num_on_host;
    
    double latency;
    double bandwidth;
  };
  mpi_info_t mpi_info;
  
  // Query hwloc about MPI
  void load_mpi_info()
  {
    CCTK_INT mpi_num_procs, mpi_proc_num;
    CCTK_INT mpi_num_hosts, mpi_host_num;
    CCTK_INT mpi_num_procs_on_host, mpi_proc_num_on_host;
    GetMPIProcessInfo(&mpi_num_procs,
                      &mpi_proc_num,
                      &mpi_num_hosts,
                      &mpi_host_num,
                      &mpi_num_procs_on_host,
                      &mpi_proc_num_on_host);
    mpi_info.mpi_num_procs         = mpi_num_procs;
    mpi_info.mpi_proc_num          = mpi_proc_num;
    mpi_info.mpi_num_hosts         = mpi_num_hosts;
    mpi_info.mpi_host_num          = mpi_host_num;
    mpi_info.mpi_num_procs_on_host = mpi_num_procs_on_host;
    mpi_info.mpi_proc_num_on_host  = mpi_proc_num_on_host;
  }
  
  
  
  // Information about the CPU, as determined by this thorn
  struct cpu_info_t {
    double cycle_speed;
    double flop_speed;
    double iop_speed;
    double allocation_speed;
  };
  cpu_info_t cpu_info;
  
  // Information about each cache level and the memory, as obtained
  // from hwloc and determined by this routine
  struct cache_info_t {
    // Information obtained from hwloc
    string    name;
    int       type;
    ptrdiff_t size;
    int       linesize;
    int       stride;
    int       num_pus;
    
    // Information determined by this thorn
    double read_latency;
    double read_bandwidth;
    double write_latency;
    double write_bandwidth;
    double stencil_performance;
  };
  vector<cache_info_t> cache_info;
  
  
  
  // Query hwloc about each cache level and the memory
  void load_cache_info()
  {
    const int num_cache_levels = GetCacheInfo1(0, 0, 0, 0, 0, 0, 0);
    vector<CCTK_POINTER_TO_CONST> names_(num_cache_levels);
    vector<CCTK_INT>              types_(num_cache_levels);
    vector<CCTK_POINTER_TO_CONST> sizes_(num_cache_levels);
    vector<CCTK_INT>              linesizes_(num_cache_levels);
    vector<CCTK_INT>              strides_(num_cache_levels);
    vector<CCTK_INT>              num_puss_(num_cache_levels);
    GetCacheInfo1(&names_[0], &types_[0],
                  &sizes_[0], &linesizes_[0], &strides_[0], &num_puss_[0],
                  num_cache_levels);
    cache_info.resize(num_cache_levels);
    for (int n=0; n<num_cache_levels; ++n) {
      cache_info[n].name     = (const char*)(names_[n]);
      cache_info[n].type     = types_[n];
      cache_info[n].size     = ptrdiff_t(sizes_[n]);
      cache_info[n].linesize = linesizes_[n];
      cache_info[n].stride   = strides_[n];
      cache_info[n].num_pus  = num_puss_[n];
    }
  }
  
  
  
  void measure_cpu_cycle_speed(const MPI_Comm comm, const int num_threads)
  {
    DECLARE_CCTK_PARAMETERS;
    
    printf("    CPU frequency:");
    if (verbose) {
      printf("\n");
    }
    // Run the benchmark for at least this long
    double min_elapsed = 1.0;   // seconds
    // Run the benchmark initially for this many iterations
    ptrdiff_t max_count = 1000000;
    // The last benchmark run took that long
    double elapsed;             // seconds
    // loop until the run time of the benchmark is longer than the
    // minimum run time
    for (;;) {
      if (verbose) {
        printf("      iterations=%td...", max_count);
        fflush(stdout);
      }
      MPI_Barrier(comm);
      elapsed = 0.0;
      volatile CCTK_REAL use_s CCTK_ATTRIBUTE_UNUSED = 0.0;
#pragma omp parallel num_threads(num_threads) reduction(+: elapsed, use_s)
      {
#pragma omp barrier
        // Start timing
        CCTK_REAL_VEC s0 = vec_set1(1.00000);
        CCTK_REAL_VEC s1 = vec_set1(1.00001);
        CCTK_REAL_VEC s2 = vec_set1(1.00002);
        CCTK_REAL_VEC s3 = vec_set1(1.00003);
        CCTK_REAL_VEC s4 = vec_set1(1.00004);
        CCTK_REAL_VEC s5 = vec_set1(1.00005);
        CCTK_REAL_VEC s6 = vec_set1(1.00006);
        CCTK_REAL_VEC s7 = vec_set1(1.00007);
        const double t0 = omp_get_wtime();
        for (ptrdiff_t count=0; count<max_count; ++count) {
          s0 = kadd(vec_set1(+1.0), s0);
          s1 = kadd(vec_set1(+1.0), s1);
          s2 = kadd(vec_set1(+1.0), s2);
          s3 = kadd(vec_set1(+1.0), s3);
          s4 = kadd(vec_set1(+1.0), s4);
          s5 = kadd(vec_set1(+1.0), s5);
          s6 = kadd(vec_set1(+1.0), s6);
          s7 = kadd(vec_set1(+1.0), s7);
        }
        // End timing
        const double t1 = omp_get_wtime();
        elapsed += t1 - t0;
        // Store sum of results into a volatile variable, so that the
        // compiler does not optimize away the calculation
        use_s += vec_elt(kadd(kadd(kadd(s0, s1), kadd(s2, s3)),
                              kadd(kadd(s4, s5), kadd(s6, s7))), 0);
      }
      elapsed = mpi_average(comm, elapsed / num_threads);
      if (verbose) {
        printf(" time=%g sec\n", elapsed);
      }
      // Are we done?
      int done = elapsed >= min_elapsed;
      MPI_Bcast(&done, 1, MPI_INT, 0, comm);
      if (done) break;
      // Estimate how many iterations we need. Run 1.1 times longer to
      // ensure we don't fall short by a tiny bit. Increase the number
      // of iterations at least by 2, at most by 10.
      max_count *= llrint(max(2.0, min(10.0, 1.1 * min_elapsed / elapsed)));
    }
    // Repeat benchmark with one fewer operation
    double elapsed2;
    {
      if (verbose) {
        printf("      iterations=%td...", max_count);
        fflush(stdout);
      }
      MPI_Barrier(comm);
      elapsed2 = 0.0;
      volatile CCTK_REAL use_s CCTK_ATTRIBUTE_UNUSED = 0.0;
#pragma omp parallel num_threads(num_threads) reduction(+: elapsed2, use_s)
      {
#pragma omp barrier
        // Start timing
        CCTK_REAL_VEC s0 = vec_set1(1.00000);
        CCTK_REAL_VEC s1 = vec_set1(1.00001);
        CCTK_REAL_VEC s2 = vec_set1(1.00002);
        CCTK_REAL_VEC s3 = vec_set1(1.00003);
        CCTK_REAL_VEC s4 = vec_set1(1.00004);
        CCTK_REAL_VEC s5 = vec_set1(1.00005);
        const double t0 = omp_get_wtime();
        for (ptrdiff_t count=0; count<max_count; ++count) {
          s0 = kadd(vec_set1(+1.0), s0);
          s1 = kadd(vec_set1(+1.0), s1);
          s2 = kadd(vec_set1(+1.0), s2);
          s3 = kadd(vec_set1(+1.0), s3);
          s4 = kadd(vec_set1(+1.0), s4);
          s5 = kadd(vec_set1(+1.0), s5);
        }
        // End timing
        const double t1 = omp_get_wtime();
        elapsed2 += t1 - t0;
        // Store sum of results into a volatile variable, so that the
        // compiler does not optimize away the calculation
        use_s += vec_elt(kadd(kadd(kadd(s0, s1), kadd(s2, s3)),
                              kadd(s4, s5)), 0);
      }
      elapsed2 = mpi_average(comm, elapsed2 / num_threads);
      if (verbose) {
        printf(" time=%g sec\n", elapsed2);
      }
    }
    cpu_info.cycle_speed = 2 * max_count / (elapsed - elapsed2);
    if (verbose) {
      printf("      result:");
    }
    printf(" %g GHz\n", cpu_info.cycle_speed / 1.0e+9);
  }
  
  
  
  void measure_cpu_flop_speed(const MPI_Comm comm, const int num_threads)
  {
    DECLARE_CCTK_PARAMETERS;
    
    printf("    CPU floating point performance:");
    if (verbose) {
      printf("\n");
    }
    // Run the benchmark for at least this long
    double min_elapsed = 1.0;   // seconds
    // Run the benchmark initially for this many iterations
    ptrdiff_t max_count = 1000000;
    // The last benchmark run took that long
    double elapsed;             // seconds
    // Loop until the run time of the benchmark is longer than the
    // minimum run time
    for (;;) {
      if (verbose) {
        printf("      iterations=%td...", max_count);
        fflush(stdout);
      }
      MPI_Barrier(comm);
      elapsed = 0.0;
      volatile CCTK_REAL use_s CCTK_ATTRIBUTE_UNUSED = 0.0;
#pragma omp parallel num_threads(num_threads) reduction(+: elapsed, use_s)
      {
#pragma omp barrier
        // Start timing
        const double t0 = omp_get_wtime();
        CCTK_REAL_VEC s0 = vec_set1(1.00000);
        CCTK_REAL_VEC s1 = vec_set1(1.00001);
        CCTK_REAL_VEC s2 = vec_set1(1.00002);
        CCTK_REAL_VEC s3 = vec_set1(1.00003);
        CCTK_REAL_VEC s4 = vec_set1(1.00004);
        CCTK_REAL_VEC s5 = vec_set1(1.00005);
        CCTK_REAL_VEC s6 = vec_set1(1.00006);
        CCTK_REAL_VEC s7 = vec_set1(1.00007);
        // Explicitly unrolled loop, performing multiply-add
        // operations. See latex file for a more detailed description.
        // Note: The constants have been chosen so that the results
        // don't over- or underflow
        for (ptrdiff_t count=0; count<max_count; ++count) {
          s0 = kmadd(vec_set1(1.1), s0, vec_set1(-0.100001));
          s1 = kmadd(vec_set1(1.1), s1, vec_set1(-0.100001));
          s2 = kmadd(vec_set1(1.1), s2, vec_set1(-0.100001));
          s3 = kmadd(vec_set1(1.1), s3, vec_set1(-0.100001));
          s4 = kmadd(vec_set1(1.1), s4, vec_set1(-0.100001));
          s5 = kmadd(vec_set1(1.1), s5, vec_set1(-0.100001));
          s6 = kmadd(vec_set1(1.1), s6, vec_set1(-0.100001));
          s7 = kmadd(vec_set1(1.1), s7, vec_set1(-0.100001));
        }
        // End timing
        const double t1 = omp_get_wtime();
        elapsed += t1 - t0;
        // Store sum of results into a volatile variable, so that the
        // compiler does not optimize away the calculation
        use_s += vec_elt(kadd(kadd(kadd(s0, s1), kadd(s2, s3)),
                              kadd(kadd(s4, s5), kadd(s6, s7))), 0);
      }
      elapsed = mpi_average(comm, elapsed / num_threads);
      if (verbose) {
        printf(" time=%g sec\n", elapsed);
      }
      // Are we done?
      int done = elapsed >= min_elapsed;
      MPI_Bcast(&done, 1, MPI_INT, 0, comm);
      if (done) break;
      // Estimate how many iterations we need. Run 1.1 times longer to
      // ensure we don't fall short by a tiny bit. Increase the number
      // of iterations at least by 2, at most by 10.
      max_count *= llrint(max(2.0, min(10.0, 1.1 * min_elapsed / elapsed)));
    }
    // Calculate CPU performance: max_count is the number of
    // iterations, 8 is the unroll factor, CCTK_REAL_VEC_SIZE is the
    // vector size, and there are 2 operations in each kmadd.
    cpu_info.flop_speed = max_count * 8 * CCTK_REAL_VEC_SIZE * 2 / elapsed;
    if (verbose) {
      printf("      result:");
    }
    printf(" %g Gflop/sec\n", cpu_info.flop_speed / 1.0e+9);
  }
  
  
  
  void measure_cpu_iop_speed(const MPI_Comm comm, const int num_threads)
  {
    DECLARE_CCTK_PARAMETERS;
    
    printf("    CPU integer performance:");
    if (verbose) {
      printf("\n");
    }
    // The basic benchmark harness is the same as above, no comments
    // here
    double min_elapsed = 1.0;
    ptrdiff_t max_count = 1000000;
    double elapsed;
    for (;;) {
      if (verbose) {
        printf("      iterations=%td...", max_count);
        fflush(stdout);
      }
      MPI_Barrier(comm);
      elapsed = 0.0;
      volatile ptrdiff_t use_s CCTK_ATTRIBUTE_UNUSED = 0;
#pragma omp parallel num_threads(num_threads) reduction(+: elapsed, use_s)
      {
#pragma omp barrier
        const double t0 = omp_get_wtime();
        vector<CCTK_REAL> base(1000);
        size_t s0, s1, s2, s3, s4, s5, s6, s7;
        s0 = s1 = s2 = s3 = s4 = s5 = s6 = s7 = 0; 
        // Explicitly unrolled loop, performing integer multiply and
        // add operations. See latex file for a more detailed
        // description.
        for (ptrdiff_t count=0; count<max_count; ++count) {
          s0 = ptrdiff_t(&base[  s0]);
          s1 = ptrdiff_t(&base[2*s1]);
          s2 = ptrdiff_t(&base[3*s2]);
          s3 = ptrdiff_t(&base[4*s3]);
          s4 = ptrdiff_t(&base[5*s4]);
          s5 = ptrdiff_t(&base[6*s5]);
          s6 = ptrdiff_t(&base[7*s6]);
          s7 = ptrdiff_t(&base[8*s7]);
        }
        const double t1 = omp_get_wtime();
        elapsed += t1 - t0;
        // Store sum of results into a volatile variable, so that the
        // compiler does not optimize away the calculation
        use_s += s0 + s1 + s2 + s3 + s4 + s5 + s6 + s7;
      }
      elapsed = mpi_average(comm, elapsed / num_threads);
      if (verbose) {
        printf(" time=%g sec\n", elapsed);
      }
      int done = elapsed >= min_elapsed;
      MPI_Bcast(&done, 1, MPI_INT, 0, comm);
      if (done) break;
      max_count *= llrint(max(2.0, min(10.0, 1.1 * min_elapsed / elapsed)));
    }
    const double iop_speed = max_count * 8 * 2 / elapsed;
    cpu_info.iop_speed = iop_speed;
    if (verbose) {
      printf("      result:");
    }
    printf(" %g Giop/sec\n", cpu_info.iop_speed / 1.0e+9);
  }
  
  
  
  // Determine the size (in bytes) for a particular cache level or
  // memory type. skipsize returns the number of bytes to allocate but
  // then to not use, so that e.g. the node-local memory can be
  // skipped. size returns the number of bytes to use for the
  // benchmark.
  void calc_sizes(int cache, ptrdiff_t& skipsize, ptrdiff_t& size)
  {
    if (cache_info[cache].type==1) {
      // Memory
      if (cache>0 && cache_info[cache-1].type==1) {
        // Global memory, and there is also local memory
        skipsize = cache_info[cache-1].size;
        size = (cache_info[cache].size - skipsize) / 4;
        assert(size >= skipsize/4);
      } else {
        // Local memory or only memory
        skipsize = 0;
        size = cache_info[cache].size / 2;
      }
    } else {
      // Cache
      skipsize = 0;
      size = cache_info[cache].size * 3 / 4;
    }
  }
  
  
  
  void measure_allocation_speed(const MPI_Comm comm, const int num_threads)
  {
    DECLARE_CCTK_PARAMETERS;
    
    cpu_info.allocation_speed = -1.0;
    for (int cache=0; cache<int(cache_info.size()); ++cache) {
      if (cache_info[cache].type==1) { // only if memory
        ptrdiff_t skipsize, size;
        calc_sizes(cache, skipsize, size);
        assert(size>0);
        int comm_size;
        MPI_Comm_size(comm, &comm_size);
        if (verbose) {
          printf("    Memory allocation performance for %s (using %td bytes):\n",
                 cache_info[cache].name.c_str(), size);
          fflush(stdout);
        } else {
          printf("    Memory allocation performance for %s (for %d PUs):",
                 cache_info[cache].name.c_str(), cache_info[cache].num_pus);
        }
        const ptrdiff_t node_memory = cache_info[cache_info.size()-1].size;
        if ((skipsize + size) * comm_size > node_memory * 3 / 4) {
          printf(" [skipped -- too many MPI processes]\n");
          continue;
        }
        vector<char> skiparray(skipsize, 1);
        double min_elapsed = 1.0;
        ptrdiff_t max_count = 1;
        double elapsed;
        for (;;) {
          if (verbose) {
            printf("      iterations=%td...", max_count);
            fflush(stdout);
          }
          elapsed = 0.0;
          for (int count=0; count<max_count; ++count) {
            const double t0 = omp_get_wtime();
            // Allocate array, set all elements to 1
            vector<char> raw_array(size, 1);
            const double t1 = omp_get_wtime();
            elapsed += t1 - t0;
            volatile char use_array CCTK_ATTRIBUTE_UNUSED = raw_array[size-1];
          }
          if (verbose) {
            printf(" time=%g sec\n", elapsed);
          }
          int done = elapsed >= min_elapsed;
          if (done) break;
          max_count *= llrint(max(2.0, min(10.0, 1.1 * min_elapsed / elapsed)));
        }
        cpu_info.allocation_speed = max_count * size / elapsed;
        if (verbose) {
          printf("      result:");
        }
        printf(" %g GByte/sec\n",
               cpu_info.allocation_speed / 1.0e+9);
      }
    }
  }



  void measure_read_latency(const MPI_Comm comm, const int num_threads)
  {
    DECLARE_CCTK_PARAMETERS;
    
    // Loop over all cache levels and memory types
    for (int cache=0; cache<int(cache_info.size()); ++cache) {
      // Determine size
      ptrdiff_t skipsize, size;
      calc_sizes(cache, skipsize, size);
      assert(size>0);
      const ptrdiff_t step = cache_info[cache].linesize;
      assert(step>0);
      int comm_size;
      MPI_Comm_size(comm, &comm_size);
      const int total_pus = comm_size * num_threads;
      const int mem_pus = cache_info[cache].num_pus;
      // const bool small_cache = num_threads % mem_pus == 0;
      // const bool large_cache = mem_pus % num_threads == 0;
      const bool small_cache = num_threads >= mem_pus;
      const bool large_cache = mem_pus >= num_threads;
      assert(small_cache || large_cache);
      const int num_active_procs = small_cache ? comm_size : 1;
      // assert(mem_pus % num_active_procs == 0);
      // number of local arrays
      // assert(total_pus % mem_pus == 0);
      const int num_allocs = (total_pus + mem_pus - 1) / mem_pus;
      assert(num_allocs % num_active_procs == 0);
      const int num_local_allocs = num_allocs / num_active_procs;
      if (verbose) {
        printf("    Read latency of %s (for %d PUs) (using %d*%td bytes):\n",
               cache_info[cache].name.c_str(), cache_info[cache].num_pus,
               num_allocs, size);
        fflush(stdout);
      } else {
        printf("    Read latency of %s (for %d PUs):",
               cache_info[cache].name.c_str(), cache_info[cache].num_pus);
      }
      const ptrdiff_t node_memory = cache_info[cache_info.size()-1].size;
      if (comm_size > num_active_procs ||
          (skipsize + size) * comm_size > node_memory * 3 / 4)
      {
        printf(" [skipped -- too many MPI processes]\n");
        continue;
      }
      // Allocate skipped memory, filling it with 1 so that it is
      // actually allocated by the operating system
      vector<char> skiparray(skipsize, 1);
      const ptrdiff_t offset = 0xa1d2d5ff; // a random number
      const ptrdiff_t nmax = size / sizeof(void*);
      // Linked list (see latex)
      vector<vector<void*> > arrays(num_local_allocs);
      for (int alloc=0; alloc<num_local_allocs; ++alloc) {
        arrays[alloc].resize(nmax);
        arrays[alloc][0] = NULL;
      }
#pragma omp parallel num_threads(num_threads)
      if (omp_get_thread_num() % mem_pus == 0) {
        const int alloc = omp_get_thread_num() / mem_pus;
        assert(alloc < num_local_allocs);
        void** const array = &arrays[alloc][0];
        ptrdiff_t i = 0;
        for (ptrdiff_t n=0; n<nmax; ++n) {
          ptrdiff_t next_i = (i+offset) % nmax;
          if (array[i] && n != nmax-1) ++next_i;
          assert(!array[i]);
          array[i] = &array[next_i];
          i = next_i;
        }
        assert(i == 0);
      }
      for (int alloc=0; alloc<num_local_allocs; ++alloc) {
        assert(arrays[alloc][0] != NULL);
      }
      // The basic benchmark harness is the same as above, no comments
      // here
      double min_elapsed = 1.0;
      ptrdiff_t max_count = 1000;
      double elapsed;
      for (;;) {
        if (verbose) {
          printf("      iterations=%td...", max_count);
          fflush(stdout);
        }
        MPI_Barrier(comm);
        elapsed = 0.0;
        volatile ptrdiff_t use_ptr CCTK_ATTRIBUTE_UNUSED = 0;
#pragma omp parallel num_threads(num_threads) reduction(+: elapsed, use_ptr)
        {
          const int alloc = omp_get_thread_num() / mem_pus;
          assert(alloc < num_local_allocs);
          void** const array = &arrays[alloc][0];
#pragma omp barrier
          const double t0 = omp_get_wtime();
          void* ptr = &array[0];
          // Chase linked list (see latex)
          for (ptrdiff_t count=0; count<max_count; ++count) {
#define REPEAT10(x) x x x x x x x x x x
            REPEAT10(REPEAT10(ptr = *(void**)ptr;))
#undef REPEAT10
          }
          const double t1 = omp_get_wtime();
          elapsed += t1 - t0;
          use_ptr += ptrdiff_t(ptr);
        }
        elapsed = mpi_average(comm, elapsed / num_threads);
        if (verbose) {
          printf(" time=%g sec\n", elapsed);
        }
        int done = elapsed >= min_elapsed;
        MPI_Bcast(&done, 1, MPI_INT, 0, comm);
        if (done) break;
        max_count *= llrint(max(2.0, min(10.0, 1.1 * min_elapsed / elapsed)));
      }
      cache_info[cache].read_latency = elapsed / (max_count * 100);
      if (verbose) {
        printf("      result:");
      }
      printf(" %g nsec\n", cache_info[cache].read_latency * 1.0e+9);
    }
  }
  
  
  
  void measure_read_bandwidth(const MPI_Comm comm, const int num_threads)
  {
    DECLARE_CCTK_PARAMETERS;
    
    // The basic benchmark harness is the same as above, no comments
    // here
    for (int cache=0; cache<int(cache_info.size()); ++cache) {
      ptrdiff_t skipsize, size;
      calc_sizes(cache, skipsize, size);
      assert(size>0);
      int comm_size;
      MPI_Comm_size(comm, &comm_size);
      const int total_pus = comm_size * num_threads;
      const int mem_pus = cache_info[cache].num_pus;
      // const bool small_cache = num_threads % mem_pus == 0;
      // const bool large_cache = mem_pus % num_threads == 0;
      const bool small_cache = num_threads >= mem_pus;
      const bool large_cache = mem_pus >= num_threads;
      assert(small_cache || large_cache);
      const int num_active_procs = small_cache ? comm_size : 1;
      // assert(mem_pus % num_active_procs == 0);
      // number of local arrays
      // assert(total_pus % mem_pus == 0);
      const int num_allocs = (total_pus + mem_pus - 1) / mem_pus;
      assert(num_allocs % num_active_procs == 0);
      const int num_local_allocs = num_allocs / num_active_procs;
      if (verbose) {
        printf("    Read bandwidth of %s (for %d PUs) (using %d*%td bytes):\n",
               cache_info[cache].name.c_str(), cache_info[cache].num_pus,
               num_allocs, size);
        fflush(stdout);
      } else {
        printf("    Read bandwidth %s (for %d PUs):",
               cache_info[cache].name.c_str(), cache_info[cache].num_pus);
      }
      const ptrdiff_t node_memory = cache_info[cache_info.size()-1].size;
      if (comm_size > num_active_procs ||
          (skipsize + size) * comm_size > node_memory * 3 / 4)
      {
        printf(" [skipped -- too many MPI processes]\n");
        continue;
      }
      vector<char> skiparray(skipsize, 1);
      const ptrdiff_t nmax = size / sizeof(CCTK_REAL);
      // Allocate array, set all elements to 1.0
      vector<vector<CCTK_REAL> > raw_arrays(num_local_allocs);
      vector<CCTK_REAL*> arrays(num_local_allocs);
      for (int alloc=0; alloc<num_local_allocs; ++alloc) {
        raw_arrays[alloc].resize(nmax + CCTK_REAL_VEC_SIZE-1);
        arrays[alloc] =
          (CCTK_REAL*)(ptrdiff_t(&raw_arrays[alloc][CCTK_REAL_VEC_SIZE-1]) &
                       -sizeof(CCTK_REAL_VEC));
        arrays[alloc][0] = 0.0;
      }
#pragma omp parallel num_threads(num_threads)
      if (omp_get_thread_num() % mem_pus == 0) {
        const int alloc = omp_get_thread_num() / mem_pus;
        assert(alloc < num_local_allocs);
        CCTK_REAL* restrict const array = &arrays[alloc][0];
        for (ptrdiff_t n=0; n<nmax; ++n) {
          array[n] = 1.0;
        }
      }
      for (int alloc=0; alloc<num_local_allocs; ++alloc) {
        assert(arrays[alloc][0] == 1.0);
      }
      double min_elapsed = 1.0;
      ptrdiff_t max_count = 1;
      double elapsed;
      for (;;) {
        if (verbose) {
          printf("      iterations=%td...", max_count);
          fflush(stdout);
        }
        MPI_Barrier(comm);
        elapsed = 0.0;
        volatile CCTK_REAL use_s CCTK_ATTRIBUTE_UNUSED = 0.0;
#pragma omp parallel num_threads(num_threads) reduction(+: elapsed, use_s)
        {
          const int alloc = omp_get_thread_num() / mem_pus;
          assert(alloc < num_local_allocs);
          CCTK_REAL* restrict const array = &arrays[alloc][0];
#pragma omp barrier
          const double t0 = omp_get_wtime();
          for (ptrdiff_t count=0; count<max_count; ++count) {
            CCTK_REAL_VEC s0, s1, s2, s3, s4, s5, s6, s7;
            s0 = s1 = s2 = s3 = s4 = s5 = s6 = s7 = vec_set1(0.0);
            const ptrdiff_t dn = CCTK_REAL_VEC_SIZE;
            // Access memory with unit stride, consuming data via
            // multiply and add operations (see latex)
            for (ptrdiff_t n=0; n<nmax;) {
              s0 = kmadd(vec_load(array[n]), s0, vec_load(array[n+dn]));
              n += 2*dn;
              s1 = kmadd(vec_load(array[n]), s1, vec_load(array[n+dn]));
              n += 2*dn;
              s2 = kmadd(vec_load(array[n]), s2, vec_load(array[n+dn]));
              n += 2*dn;
              s3 = kmadd(vec_load(array[n]), s3, vec_load(array[n+dn]));
              n += 2*dn;
              s4 = kmadd(vec_load(array[n]), s4, vec_load(array[n+dn]));
              n += 2*dn;
              s5 = kmadd(vec_load(array[n]), s5, vec_load(array[n+dn]));
              n += 2*dn;
              s6 = kmadd(vec_load(array[n]), s6, vec_load(array[n+dn]));
              n += 2*dn;
              s7 = kmadd(vec_load(array[n]), s7, vec_load(array[n+dn]));
              n += 2*dn;
            }
            use_s += vec_elt(kadd(kadd(kadd(s0, s1), kadd(s2, s3)),
                                  kadd(kadd(s4, s5), kadd(s6, s7))), 0);
          }
          const double t1 = omp_get_wtime();
          elapsed += t1 - t0;
        }
        elapsed = mpi_average(comm, elapsed / num_threads);
        if (verbose) {
          printf(" time=%g sec\n", elapsed);
        }
        int done = elapsed >= min_elapsed;
        MPI_Bcast(&done, 1, MPI_INT, 0, comm);
        if (done) break;
        max_count *= llrint(max(2.0, min(10.0, 1.1 * min_elapsed / elapsed)));
      }
      cache_info[cache].read_bandwidth = max_count * size / elapsed;
      if (verbose) {
        printf("      result:");
      }
      printf(" %g GByte/sec\n",
             cache_info[cache].read_bandwidth / 1.0e+9);
    }
  }
  
  
  
  void measure_write_latency(const MPI_Comm comm, const int num_threads)
  {
    DECLARE_CCTK_PARAMETERS;
    
    // The basic benchmark harness is the same as above, no comments
    // here
    for (int cache=0; cache<int(cache_info.size()); ++cache) {
      ptrdiff_t skipsize, size;
      calc_sizes(cache, skipsize, size);
      assert(size>0);
      // Round down size to next power of two
      size = ptrdiff_t(1) << ilogb(double(size));
      // Define a mask for efficient modulo operations
      const ptrdiff_t size_mask = size - 1;
      const ptrdiff_t offset = 0xa1d2d5ff; // a random number
      assert(size>0);
      int comm_size;
      MPI_Comm_size(comm, &comm_size);
      const int total_pus = comm_size * num_threads;
      const int mem_pus = cache_info[cache].num_pus;
      // const bool small_cache = num_threads % mem_pus == 0;
      // const bool large_cache = mem_pus % num_threads == 0;
      const bool small_cache = num_threads >= mem_pus;
      const bool large_cache = mem_pus >= num_threads;
      assert(small_cache || large_cache);
      const int num_active_procs = small_cache ? comm_size : 1;
      // assert(mem_pus % num_active_procs == 0);
      // number of local arrays
      // assert(total_pus % mem_pus == 0);
      const int num_allocs = (total_pus + mem_pus - 1) / mem_pus;
      assert(num_allocs % num_active_procs == 0);
      const int num_local_allocs = num_allocs / num_active_procs;
      if (verbose) {
        printf("    Write latency of %s (for %d PUs) (using %d*%td bytes):\n",
               cache_info[cache].name.c_str(), cache_info[cache].num_pus,
               num_allocs, size);
        fflush(stdout);
      } else {
        printf("    Write latency of %s (for %d PUs):",
               cache_info[cache].name.c_str(), cache_info[cache].num_pus);
      }
      const ptrdiff_t node_memory = cache_info[cache_info.size()-1].size;
      if (comm_size > num_active_procs ||
          (skipsize + size) * comm_size > node_memory * 3 / 4)
      {
        printf(" [skipped -- too many MPI processes]\n");
        continue;
      }
      vector<char> skiparray(skipsize, 1);
      vector<vector<char> > arrays(num_local_allocs);
      for (int alloc=0; alloc<num_local_allocs; ++alloc) {
        arrays[alloc].resize(size);
        arrays[alloc][0] = 0;
      }
#pragma omp parallel num_threads(num_threads)
      if (omp_get_thread_num() % mem_pus == 0) {
        const int alloc = omp_get_thread_num() / mem_pus;
        assert(alloc < num_local_allocs);
        char* const array = &arrays[alloc][0];
        memset(array, 1, size);
      }
      for (int alloc=0; alloc<num_local_allocs; ++alloc) {
        assert(arrays[alloc][0] == 1);
      }
      double min_elapsed = 1.0;
      ptrdiff_t max_count = 1000;
      double elapsed;
      for (;;) {
        if (verbose) {
          printf("      iterations=%td...", max_count);
          fflush(stdout);
        }
        MPI_Barrier(comm);
        elapsed = 0.0;
        volatile char use_array CCTK_ATTRIBUTE_UNUSED = 0;
#pragma omp parallel num_threads(num_threads) reduction(+: elapsed, use_array)
        {
          const int alloc = omp_get_thread_num() / mem_pus;
          assert(alloc < num_local_allocs);
          char* const array = &arrays[alloc][0];
#pragma omp barrier
          const double t0 = omp_get_wtime();
          ptrdiff_t n = 0;
          // March through the array with large, pseudo-random steps
          // (see latex)
          for (ptrdiff_t count=0; count<max_count; ++count) {
            array[n & size_mask] = 2;
            n += offset;
            array[n & size_mask] = 2;
            n += offset;
            array[n & size_mask] = 2;
            n += offset;
            array[n & size_mask] = 2;
            n += offset;
            array[n & size_mask] = 2;
            n += offset;
            array[n & size_mask] = 2;
            n += offset;
            array[n & size_mask] = 2;
            n += offset;
            array[n & size_mask] = 2;
            n += offset;
          }
          const double t1 = omp_get_wtime();
          elapsed += t1 - t0;
          use_array += array[0];
        }
        elapsed = mpi_average(comm, elapsed / num_threads);
        if (verbose) {
          printf(" time=%g sec\n", elapsed);
        }
        int done = elapsed >= min_elapsed;
        MPI_Bcast(&done, 1, MPI_INT, 0, comm);
        if (done) break;
        max_count *= llrint(max(2.0, min(10.0, 1.1 * min_elapsed / elapsed)));
      }
      cache_info[cache].write_latency = elapsed / (max_count * 8);
      if (verbose) {
        printf("      result:");
      }
      printf(" %g nsec\n", cache_info[cache].write_latency * 1.0e+9);
    }
  }
  
  
  
  void measure_write_bandwidth(const MPI_Comm comm, const int num_threads)
  {
    DECLARE_CCTK_PARAMETERS;
    
    // The basic benchmark harness is the same as above, no comments
    // here
    for (int cache=0; cache<int(cache_info.size()); ++cache) {
      ptrdiff_t skipsize, size;
      calc_sizes(cache, skipsize, size);
      assert(size>0);
      int comm_size;
      MPI_Comm_size(comm, &comm_size);
      const int total_pus = comm_size * num_threads;
      const int mem_pus = cache_info[cache].num_pus;
      // const bool small_cache = num_threads % mem_pus == 0;
      // const bool large_cache = mem_pus % num_threads == 0;
      const bool small_cache = num_threads >= mem_pus;
      const bool large_cache = mem_pus >= num_threads;
      assert(small_cache || large_cache);
      const int num_active_procs = small_cache ? comm_size : 1;
      // assert(mem_pus % num_active_procs == 0);
      // number of local arrays
      // assert(total_pus % mem_pus == 0);
      const int num_allocs = (total_pus + mem_pus - 1) / mem_pus;
      assert(num_allocs % num_active_procs == 0);
      const int num_local_allocs = num_allocs / num_active_procs;
      if (verbose) {
        printf("    Write bandwidth via memset for %s (for %d PUs) (using %d*%td bytes):\n",
               cache_info[cache].name.c_str(), cache_info[cache].num_pus,
               num_allocs, size);
        fflush(stdout);
      } else {
        printf("    Write bandwidth via memset for %s (for %d PUs):",
               cache_info[cache].name.c_str(), cache_info[cache].num_pus);
      }
      const ptrdiff_t node_memory = cache_info[cache_info.size()-1].size;
      if (comm_size > num_active_procs ||
          (skipsize + size) * comm_size > node_memory * 3 / 4)
      {
        printf(" [skipped -- too many MPI processes]\n");
        continue;
      }
      vector<char> skiparray(skipsize, 1);
      vector<vector<char> > arrays(num_local_allocs);
      for (int alloc=0; alloc<num_local_allocs; ++alloc) {
        arrays[alloc].resize(size);
        arrays[alloc][0] = 0;
      }
#pragma omp parallel num_threads(num_threads)
      if (omp_get_thread_num() % mem_pus == 0) {
        const int alloc = omp_get_thread_num() / mem_pus;
        assert(alloc < num_local_allocs);
        char* restrict const array = &arrays[alloc][0];
        memset(array, 1, size);
      }
      for (int alloc=0; alloc<num_local_allocs; ++alloc) {
        assert(arrays[alloc][0] == 1);
      }
      double min_elapsed = 1.0;
      ptrdiff_t max_count = 1;
      double elapsed;
      for (;;) {
        if (verbose) {
          printf("      iterations=%td...", max_count);
          fflush(stdout);
        }
        MPI_Barrier(comm);
        elapsed = 0.0;
        volatile char use_array CCTK_ATTRIBUTE_UNUSED = 0;
#pragma omp parallel num_threads(num_threads) reduction(+: elapsed, use_array)
        {
          const int alloc = omp_get_thread_num() / mem_pus;
          assert(alloc < num_local_allocs);
          char* restrict const array = &arrays[alloc][0];
#pragma omp barrier
          const double t0 = omp_get_wtime();
          // Use memset for writing (see latex)
          for (ptrdiff_t count=0; count<max_count; ++count) {
            memset(&array[0], count % 256, size);
            use_array += array[count % size];
          }
          const double t1 = omp_get_wtime();
          elapsed += t1 - t0;
        }
        elapsed = mpi_average(comm, elapsed / num_threads);
        if (verbose) {
          printf(" time=%g sec\n", elapsed);
        }
        int done = elapsed >= min_elapsed;
        MPI_Bcast(&done, 1, MPI_INT, 0, comm);
        if (done) break;
        max_count *= llrint(max(2.0, min(10.0, 1.1 * min_elapsed / elapsed)));
      }
      cache_info[cache].write_bandwidth = max_count * size / elapsed;
      if (verbose) {
        printf("      result:");
      }
      printf(" %g GByte/sec\n",
             cache_info[cache].write_bandwidth / 1.0e+9);
    }
  }
  
  
  
  void measure_write_bandwidth2(const MPI_Comm comm, const int num_threads)
  {
    DECLARE_CCTK_PARAMETERS;
    
    // The basic benchmark harness is the same as above, no comments
    // here
    for (int cache=0; cache<int(cache_info.size()); ++cache) {
      if (cache_info[cache].type==1) { // only if memory
        ptrdiff_t skipsize, size;
        calc_sizes(cache, skipsize, size);
        assert(size>0);
        int comm_size;
        MPI_Comm_size(comm, &comm_size);
        const int total_pus = comm_size * num_threads;
        const int mem_pus = cache_info[cache].num_pus;
        // const bool small_cache = num_threads % mem_pus == 0;
        // const bool large_cache = mem_pus % num_threads == 0;
        const bool small_cache = num_threads >= mem_pus;
        const bool large_cache = mem_pus >= num_threads;
        assert(small_cache || large_cache);
        const int num_active_procs = small_cache ? comm_size : 1;
        // assert(mem_pus % num_active_procs == 0);
        // number of local arrays
        // assert(total_pus % mem_pus == 0);
        const int num_allocs = (total_pus + mem_pus - 1) / mem_pus;
        assert(num_allocs % num_active_procs == 0);
        const int num_local_allocs = num_allocs / num_active_procs;
        if (verbose) {
          printf("    Write bandwidth via cache-bypassing stores for %s (for %d PUs) (using %d*%td bytes):\n",
                 cache_info[cache].name.c_str(), cache_info[cache].num_pus,
                 num_allocs, size);
          fflush(stdout);
        } else {
          printf("    Write bandwidth via cache-bypassing stores for %s (for %d PUs):",
                 cache_info[cache].name.c_str(), cache_info[cache].num_pus);
        }
        const ptrdiff_t node_memory = cache_info[cache_info.size()-1].size;
        if (comm_size > num_active_procs ||
            (skipsize + size) * comm_size > node_memory * 3 / 4)
        {
          printf(" [skipped -- too many MPI processes]\n");
          continue;
        }
        vector<char> skiparray(skipsize, 1);
        const ptrdiff_t nmax = size / sizeof(CCTK_REAL);
        // Allocate array, set all elements to 1.0
        vector<vector<CCTK_REAL> > raw_arrays(num_local_allocs);
        vector<CCTK_REAL*> arrays(num_local_allocs);
        for (int alloc=0; alloc<num_local_allocs; ++alloc) {
          raw_arrays[alloc].resize(nmax + CCTK_REAL_VEC_SIZE-1);
          arrays[alloc] =
            (CCTK_REAL*)(ptrdiff_t(&raw_arrays[alloc][CCTK_REAL_VEC_SIZE-1]) &
                         -sizeof(CCTK_REAL_VEC));
          arrays[alloc][0] = 0.0;
        }
#pragma omp parallel num_threads(num_threads)
        if (omp_get_thread_num() % mem_pus == 0) {
          const int alloc = omp_get_thread_num() / mem_pus;
          assert(alloc < num_local_allocs);
          CCTK_REAL* restrict const array = &arrays[alloc][0];
          for (ptrdiff_t n=0; n<nmax; ++n) {
            array[n] = 1.0;
          }
        }
        for (int alloc=0; alloc<num_local_allocs; ++alloc) {
          assert(arrays[alloc][0] == 1.0);
        }
        double min_elapsed = 1.0;
        ptrdiff_t max_count = 1;
        double elapsed;
        for (;;) {
          if (verbose) {
            printf("      iterations=%td...", max_count);
            fflush(stdout);
          }
          MPI_Barrier(comm);
          elapsed = 0.0;
          volatile CCTK_REAL use_array CCTK_ATTRIBUTE_UNUSED = 0.0;
#pragma omp parallel num_threads(num_threads) reduction(+: elapsed, use_array)
          {
            const int alloc = omp_get_thread_num() / mem_pus;
            assert(alloc < num_local_allocs);
            CCTK_REAL* restrict const array = &arrays[alloc][0];
#pragma omp barrier
            const double t0 = omp_get_wtime();
            // Use cache-bypassing stores
            for (ptrdiff_t count=0; count<max_count; ++count) {
              CCTK_REAL_VEC s = vec_set1(CCTK_REAL(count));
              for (ptrdiff_t n=0; n<nmax;) {
                vec_store_nta(array[n], s); n += CCTK_REAL_VEC_SIZE;
                vec_store_nta(array[n], s); n += CCTK_REAL_VEC_SIZE;
                vec_store_nta(array[n], s); n += CCTK_REAL_VEC_SIZE;
                vec_store_nta(array[n], s); n += CCTK_REAL_VEC_SIZE;
                vec_store_nta(array[n], s); n += CCTK_REAL_VEC_SIZE;
                vec_store_nta(array[n], s); n += CCTK_REAL_VEC_SIZE;
                vec_store_nta(array[n], s); n += CCTK_REAL_VEC_SIZE;
                vec_store_nta(array[n], s); n += CCTK_REAL_VEC_SIZE;
              }
              use_array += array[0];
            }
            const double t1 = omp_get_wtime();
            elapsed += t1 - t0;
          }
          elapsed = mpi_average(comm, elapsed / num_threads);
          if (verbose) {
            printf(" time=%g sec\n", elapsed);
          }
          int done = elapsed >= min_elapsed;
          MPI_Bcast(&done, 1, MPI_INT, 0, comm);
          if (done) break;
          max_count *= llrint(max(2.0, min(10.0, 1.1 * min_elapsed / elapsed)));
        }
        cache_info[cache].write_bandwidth = max_count * size / elapsed;
        if (verbose) {
          printf("      result:");
        }
        printf(" %g GByte/sec\n",
               cache_info[cache].write_bandwidth / 1.0e+9);
      }
    }
  }
  
  
  
  void measure_stencil_performance(const MPI_Comm comm, const int num_threads)
  {
    DECLARE_CCTK_PARAMETERS;
    
    // The basic benchmark harness is the same as above, no comments
    // here
    for (int cache=0; cache<int(cache_info.size()); ++cache) {
      ptrdiff_t skipsize, size;
      calc_sizes(cache, skipsize, size);
      assert(size>0);
      int comm_size;
      MPI_Comm_size(comm, &comm_size);
      const int total_pus = comm_size * num_threads;
      const int mem_pus = cache_info[cache].num_pus;
      // const bool small_cache = num_threads % mem_pus == 0;
      // const bool large_cache = mem_pus % num_threads == 0;
      const bool small_cache = num_threads >= mem_pus;
      const bool large_cache = mem_pus >= num_threads;
      assert(small_cache || large_cache);
      const int num_active_procs = small_cache ? comm_size : 1;
      // assert(mem_pus % num_active_procs == 0);
      // number of local arrays
      // assert(total_pus % mem_pus == 0);
      const int num_allocs = (total_pus + mem_pus - 1) / mem_pus;
      assert(num_allocs % num_active_procs == 0);
      const ptrdiff_t np = llrint(pow(0.5 * size / sizeof(CCTK_REAL), 1.0/3.0));
      if (verbose) {
        printf("    Stencil code performance of %s (for %d PUs) (using %td grid points):\n",
               cache_info[cache].name.c_str(), cache_info[cache].num_pus, np);
        fflush(stdout);
      } else {
        printf("    Stencil code performance %s (for %d PUs):",
               cache_info[cache].name.c_str(), cache_info[cache].num_pus);
      }
      const ptrdiff_t node_memory = cache_info[cache_info.size()-1].size;
      if (comm_size > num_active_procs ||
          (skipsize + size) * comm_size > node_memory * 3 / 4)
      {
        printf(" [skipped -- too many MPI processes]\n");
        continue;
      }
      vector<char> skiparray(skipsize, 1);
      // Allocate array
      const ptrdiff_t npa =
        (np + CCTK_REAL_VEC_SIZE - 1) / CCTK_REAL_VEC_SIZE * CCTK_REAL_VEC_SIZE;
      vector<CCTK_REAL> src(npa*np*np), dst(npa*np*np);
      const ptrdiff_t di = 1;
      const ptrdiff_t dj = di * npa;
      const ptrdiff_t dk = dj * np;
#pragma omp parallel for num_threads(num_threads) collapse(3)
      for (ptrdiff_t k=0; k<np; ++k) {
        for (ptrdiff_t j=0; j<np; ++j) {
          for (ptrdiff_t i=0; i<np; ++i) {
            const ptrdiff_t n = di*i + dj*j + dk*k;
            src[n] = 1.0;
            dst[n] = 1.0;
          }
        }
      }
      double min_elapsed = 1.0;
      ptrdiff_t max_count = 1;
      double elapsed;
      for (;;) {
        if (verbose) {
          printf("      iterations=%td...", max_count);
          fflush(stdout);
        }
        MPI_Barrier(comm);
        elapsed = 0.0;
        const double t0 = omp_get_wtime();
        for (int count=0; count<max_count; ++count) {
          const ptrdiff_t imin = 1 & -CCTK_REAL_VEC_SIZE;
          const ptrdiff_t imax = np-1;
#pragma omp parallel for num_threads(num_threads) collapse(3)
          for (ptrdiff_t k=1; k<np-1; ++k) {
            for (ptrdiff_t j=1; j<np-1; ++j) {
              for (ptrdiff_t i=imin; i<imax; i+=CCTK_REAL_VEC_SIZE) {
                const ptrdiff_t n = di*i + dj*j + dk*k;
                CCTK_REAL_VEC x = vec_load(src[n]);
                CCTK_REAL_VEC dxi, dxj, dxk;
                dxi = kadd(vec_loadu(src[n-di]), vec_loadu(src[n+di]));
                dxj = kadd(vec_load(src[n-dj]), vec_load(src[n+dj]));
                dxk = kadd(vec_load(src[n-dk]), vec_load(src[n+dk]));
                x = kadd(kmul(vec_set1(0.5), x),
                         kmul(vec_set1(0.5/3.0), kadd(kadd(dxi, dxj), dxk)));
                // vec_store(dst[n], x);
                vec_store_partial_prepare(i, imin, imax);
                vec_store_nta_partial(dst[n], x);
              }
            }
          }
          swap(src, dst);
        }
        const double t1 = omp_get_wtime();
        volatile CCTK_REAL use_s CCTK_ATTRIBUTE_UNUSED = dst[0];
        elapsed += t1 - t0;
        elapsed = mpi_average(comm, elapsed / num_threads);
        if (verbose) {
          printf(" time=%g sec\n", elapsed);
        }
        int done = elapsed >= min_elapsed;
        MPI_Bcast(&done, 1, MPI_INT, 0, comm);
        if (done) break;
        max_count *= llrint(max(2.0, min(10.0, 1.1 * min_elapsed / elapsed)));
      }
      cache_info[cache].stencil_performance = max_count * np*np*np / elapsed;
      if (verbose) {
        printf("      result:");
      }
      printf(" %g Gupdates/sec\n",
             cache_info[cache].stencil_performance / 1.0e+9);
    }
  }
  
  
  
  void measure_mpi_latency(const MPI_Comm comm)
  {
    printf("    MPI latency:");
    fflush(stdout);
    int rank;
    MPI_Comm_rank(comm, &rank);
    double elapsed;
    if (rank == 0) {
      char cval;
      MPI_Recv(&cval, 1, MPI_CHAR, 1, 0, comm, MPI_STATUS_IGNORE);
      const double t0 = omp_get_wtime();
      MPI_Send(&cval, 1, MPI_CHAR, 1, 0, comm);
      MPI_Recv(&cval, 1, MPI_CHAR, 1, 0, comm, MPI_STATUS_IGNORE);
      const double t1 = omp_get_wtime();
      elapsed = t1 - t0;
    } else if (rank == 1) {
      char cval = 0;
      MPI_Send(&cval, 1, MPI_CHAR, 0, 0, comm);
      MPI_Recv(&cval, 1, MPI_CHAR, 0, 0, comm, MPI_STATUS_IGNORE);
      MPI_Send(&cval, 1, MPI_CHAR, 0, 0, comm);
    }
    MPI_Bcast(&elapsed, 1, MPI_DOUBLE, 0, comm);
    mpi_info.latency = elapsed / 2;
    printf(" %g nsec\n", mpi_info.latency * 1.0e+9);
  }
  
  
  
  void measure_mpi_bandwidth(const MPI_Comm comm)
  {
    printf("    MPI bandwidth:");
    fflush(stdout);
    int rank;
    MPI_Comm_rank(comm, &rank);
    double elapsed;
    const size_t nmax = 10000000;     // 10 MByte
    if (rank == 0) {
      vector<char> array1(nmax, 1);
      vector<char> array2(nmax, 2);
      // Pre-post receive
      MPI_Request req;
      MPI_Irecv(&array2[0], nmax, MPI_CHAR, 1, 0, comm, &req);
      // Wait for other process to be ready
      char cval;
      MPI_Recv(&cval, 1, MPI_CHAR, 1, 1, comm, MPI_STATUS_IGNORE);
      const double t0 = omp_get_wtime();
      // Send data
      MPI_Send(&array1[0], nmax, MPI_CHAR, 1, 0, comm);
      // Receive data
      MPI_Wait(&req, MPI_STATUS_IGNORE);
      const double t1 = omp_get_wtime();
      elapsed = t1 - t0;
    } else if (rank == 1) {
      vector<char> array1(nmax, 3);
      vector<char> array2(nmax, 4);
      // Pre-post receive
      MPI_Request req;
      MPI_Irecv(&array1[0], nmax, MPI_CHAR, 0, 0, comm, &req);
      // Tell other process we are ready
      char cval = 0;
      MPI_Send(&cval, 1, MPI_CHAR, 0, 1, comm);
      // Receive data
      MPI_Wait(&req, MPI_STATUS_IGNORE);
      // Send data
      MPI_Send(&array2[0], nmax, MPI_CHAR, 0, 0, comm);
    }
    MPI_Bcast(&elapsed, 1, MPI_DOUBLE, 0, comm);
    mpi_info.latency = nmax / (elapsed / 2);
    printf(" %g GByte/sec\n", mpi_info.latency / 1.0e+9);
  }
  
}



extern "C"
void MemSpeed_MeasureSpeed(CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  
  CCTK_INFO("Measuring CPU, cache, and memory speeds:");
  
  load_mpi_info();
  load_cache_info();
  
  MPI_Comm world = MPI_COMM_WORLD;
  
  {
    MPI_Comm singlecore;
    MPI_Comm_split(world,
                   mpi_info.mpi_proc_num==0 ? 0 : MPI_UNDEFINED,
                   mpi_info.mpi_proc_num,
                   &singlecore);
    
    printf("  Single-core measurements "
           "(using %d MPI processes with %d OpenMP threads each):\n", 1, 1);
    
    // Measure performance on a single core of the first node
    if (mpi_info.mpi_proc_num == 0) {
      measure_cpu_cycle_speed(singlecore, 1);
      measure_cpu_flop_speed(singlecore, 1);
      measure_cpu_iop_speed(singlecore, 1);
      measure_allocation_speed(singlecore, 1);
      measure_read_latency(singlecore, 1);
      measure_read_bandwidth(singlecore, 1);
      measure_write_latency(singlecore, 1);
      measure_write_bandwidth(singlecore, 1);
      measure_write_bandwidth2(singlecore, 1);
      measure_stencil_performance(singlecore, 1);
    }
    
    if (singlecore != MPI_COMM_NULL) MPI_Comm_free(&singlecore);
  }
  
  if (mpi_info.mpi_num_procs_on_host * omp_get_max_threads() > 1) {
    MPI_Comm singlenode;
    MPI_Comm_split(world,
                   mpi_info.mpi_host_num==0 ? 0 : MPI_UNDEFINED,
                   mpi_info.mpi_proc_num,
                   &singlenode);
    
    printf("  Single-node measurements "
           "(using %d MPI processes with %d OpenMP threads each):\n",
           mpi_info.mpi_num_procs_on_host, omp_get_max_threads());
    
    // Measure performance on all cores of the first node
    if (mpi_info.mpi_host_num == 0) {
      measure_cpu_cycle_speed(singlenode, omp_get_max_threads());
      measure_cpu_flop_speed(singlenode, omp_get_max_threads());
      measure_cpu_iop_speed(singlenode, omp_get_max_threads());
      measure_read_latency(singlenode, omp_get_max_threads());
      measure_read_bandwidth(singlenode, omp_get_max_threads());
      measure_write_latency(singlenode, omp_get_max_threads());
      measure_write_bandwidth(singlenode, omp_get_max_threads());
      measure_write_bandwidth2(singlenode, omp_get_max_threads());
      measure_stencil_performance(singlenode, omp_get_max_threads());
    }
    
    if (singlenode != MPI_COMM_NULL) MPI_Comm_free(&singlenode);
  }
  
  if (mpi_info.mpi_num_procs_on_host > 1) {
    MPI_Comm samenode;
    MPI_Comm_split(world,
                   (mpi_info.mpi_host_num == 0 &&
                    mpi_info.mpi_proc_num < 2) ? 0 : MPI_UNDEFINED,
                   mpi_info.mpi_proc_num,
                   &samenode);
    
    printf("  Single-node measurements:\n");
    
    // Measure performance on the same node
    if (mpi_info.mpi_host_num == 0 && mpi_info.mpi_proc_num < 2) {
      measure_mpi_latency(samenode);
      measure_mpi_bandwidth(samenode);
    }
    
    if (samenode != MPI_COMM_NULL) MPI_Comm_free(&samenode);
  }
  
  if (mpi_info.mpi_num_hosts > 1) {
    MPI_Comm twonodes;
    MPI_Comm_split(world,
                   (mpi_info.mpi_host_num < 2 &&
                    mpi_info.mpi_proc_num_on_host == 0) ? 0 : MPI_UNDEFINED,
                   mpi_info.mpi_proc_num,
                   &twonodes);
    
    printf("  Dual-node measurements:\n");
    
    // Measure performance between two nodes
    if (mpi_info.mpi_host_num < 2 && mpi_info.mpi_proc_num_on_host == 0) {
      measure_mpi_latency(twonodes);
      measure_mpi_bandwidth(twonodes);
    }
    
    if (twonodes != MPI_COMM_NULL) MPI_Comm_free(&twonodes);
  }
}
