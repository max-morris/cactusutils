#include <cctk.h>
#include <cctk_Arguments.h>
#include <cctk_Parameters.h>

#undef VECTORISE_STREAMING_STORES
#define VECTORISE_STREAMING_STORES 1
#include <vectors.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace std;



// OpenMP is only used to provide an easy-to-use low-latency timer

#ifdef _OPENMP
#  include <omp.h>
#else
#  include <sys/time.h>
namespace {
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
  
  // Information about the CPU, as determined by this thorn
  struct cpu_info_t {
    double cycle_speed;
    double flop_speed;
    double iop_speed;
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
  
  
  
  void measure_cpu_cycle_speed()
  {
    DECLARE_CCTK_PARAMETERS;
    
    printf("  CPU frequency:");
    if (verbose) {
      printf("\n");
    }
    // Run the benchmark for at least this long
    double min_elapsed = 1.0;   // seconds
    // Run the benchmark initially for this many iterations
    ptrdiff_t max_count = 1000000;
    // The last benchmark run took that long
    double elapsed = 0.0;       // seconds
    // Loop until the run time of the benchmark is longer than the
    // minimum run time
    for (;;) {
      if (verbose) {
        printf("    iterations=%td...", max_count);
        fflush(stdout);
      }
      // Start timing
      const double t0 = omp_get_wtime();
      CCTK_REAL_VEC s0, s1, s2, s3, s4, s5, s6, s7;
      s0 = s1 = s2 = s3 = s4 = s5 = s6 = s7 = vec_set1(1.0);
      for (ptrdiff_t count=0; count<max_count; ++count) {
        s0 = kadd(vec_set1(1.0), s0);
        s1 = kadd(vec_set1(1.0), s1);
        s2 = kadd(vec_set1(1.0), s2);
        s3 = kadd(vec_set1(1.0), s3);
        s4 = kadd(vec_set1(1.0), s4);
        s5 = kadd(vec_set1(1.0), s5);
        s6 = kadd(vec_set1(1.0), s6);
        s7 = kadd(vec_set1(1.0), s7);
      }
      // Store sum of results into a volatile variable, so that the
      // compiler does not optimize away the calculation
      volatile CCTK_REAL_VEC use_s CCTK_ATTRIBUTE_UNUSED =
        kadd(kadd(kadd(s0, s1), kadd(s2, s3)),
             kadd(kadd(s4, s5), kadd(s6, s7)));
      // End timing
      const double t1 = omp_get_wtime();
      elapsed = t1 - t0;
      if (verbose) {
        printf(" time=%g sec\n", elapsed);
      }
      // Are we done?
      if (elapsed >= min_elapsed) break;
      // Estimate how many iterations we need. Run 1.1 times longer to
      // ensure we don't fall short by a tiny bit. Increase the number
      // of iterations at least by 2, at most by 10.
      max_count *= llrint(max(2.0, min(10.0, 1.1 * min_elapsed / elapsed)));
    }
    // Repeat benchmark with one fewer operation
    double elapsed2 = 0.0;
    {
      if (verbose) {
        printf("    iterations=%td...", max_count);
        fflush(stdout);
      }
      const double t0 = omp_get_wtime();
      CCTK_REAL_VEC s0, s1, s2, s3, s4, s5, s6;
      s0 = s1 = s2 = s3 = s4 = s5 = s6 = vec_set1(1.0);
      for (ptrdiff_t count=0; count<max_count; ++count) {
        s0 = kadd(vec_set1(1.0), s0);
        s1 = kadd(vec_set1(1.0), s1);
        s2 = kadd(vec_set1(1.0), s2);
        s3 = kadd(vec_set1(1.0), s3);
        s4 = kadd(vec_set1(1.0), s4);
        s5 = kadd(vec_set1(1.0), s5);
        s6 = kadd(vec_set1(1.0), s6);
      }
      // Store sum of results into a volatile variable, so that the
      // compiler does not optimize away the calculation
      volatile CCTK_REAL_VEC use_s CCTK_ATTRIBUTE_UNUSED =
        kadd(kadd(kadd(s0, s1), kadd(s2, s3)),
             kadd(kadd(s4, s5), s6));
      // End timing
      const double t1 = omp_get_wtime();
      elapsed2 = t1 - t0;
      if (verbose) {
        printf(" time=%g sec\n", elapsed2);
      }
    }
    cpu_info.cycle_speed = max_count / (elapsed - elapsed2);
    if (verbose) {
      printf("    result:");
    }
    printf(" %g GHz\n", cpu_info.cycle_speed / 1.0e+9);
  }
  
  
  
  void measure_cpu_flop_speed()
  {
    DECLARE_CCTK_PARAMETERS;
    
    printf("  CPU floating point performance:");
    if (verbose) {
      printf("\n");
    }
    // Run the benchmark for at least this long
    double min_elapsed = 1.0;   // seconds
    // Run the benchmark initially for this many iterations
    ptrdiff_t max_count = 1000000;
    // The last benchmark run took that long
    double elapsed = 0.0;       // seconds
    // Loop until the run time of the benchmark is longer than the
    // minimum run time
    for (;;) {
      if (verbose) {
        printf("    iterations=%td...", max_count);
        fflush(stdout);
      }
      // Start timing
      const double t0 = omp_get_wtime();
      CCTK_REAL_VEC s0, s1, s2, s3, s4, s5, s6, s7;
      s0 = s1 = s2 = s3 = s4 = s5 = s6 = s7 = vec_set1(1.0);
      // Explicitly unrolled loop, performing multiply-add operations.
      // See latex file for a more detailed description. Note: The
      // constants have been chosen so that the results don't over- or
      // underflow
      for (ptrdiff_t count=0; count<max_count; ++count) {
        s0 = kmadd(vec_set1(1.1), s0, vec_set1(-0.1));
        s1 = kmadd(vec_set1(1.1), s1, vec_set1(-0.1));
        s2 = kmadd(vec_set1(1.1), s2, vec_set1(-0.1));
        s3 = kmadd(vec_set1(1.1), s3, vec_set1(-0.1));
        s4 = kmadd(vec_set1(1.1), s4, vec_set1(-0.1));
        s5 = kmadd(vec_set1(1.1), s5, vec_set1(-0.1));
        s6 = kmadd(vec_set1(1.1), s6, vec_set1(-0.1));
        s7 = kmadd(vec_set1(1.1), s7, vec_set1(-0.1));
      }
      // Store sum of results into a volatile variable, so that the
      // compiler does not optimize away the calculation
      volatile CCTK_REAL_VEC use_s CCTK_ATTRIBUTE_UNUSED =
        kadd(kadd(kadd(s0, s1), kadd(s2, s3)),
             kadd(kadd(s4, s5), kadd(s6, s7)));
      // End timing
      const double t1 = omp_get_wtime();
      elapsed = t1 - t0;
      if (verbose) {
        printf(" time=%g sec\n", elapsed);
      }
      // Are we done?
      if (elapsed >= min_elapsed) break;
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
      printf("    result:");
    }
    printf(" %g Gflop/sec for each PU\n", cpu_info.flop_speed / 1.0e+9);
  }
  
  
  
  void measure_cpu_iop_speed()
  {
    DECLARE_CCTK_PARAMETERS;
    
    printf("  CPU integer performance:");
    if (verbose) {
      printf("\n");
    }
    // The basic benchmark harness is the same as above, no comments
    // here
    double min_elapsed = 1.0;
    ptrdiff_t max_count = 1000000;
    double elapsed = 0.0;
    for (;;) {
      if (verbose) {
        printf("    iterations=%td...", max_count);
        fflush(stdout);
      }
      const double t0 = omp_get_wtime();
      vector<CCTK_REAL> base(1000);
      ptrdiff_t s0, s1, s2, s3, s4, s5, s6, s7;
      s0 = s1 = s2 = s3 = s4 = s5 = s6 = s7 = 0; 
      // Explicitly unrolled loop, performing integer multiply and add
      // operations. See latex file for a more detailed description.
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
      volatile ptrdiff_t use_s CCTK_ATTRIBUTE_UNUSED =
        s0 + s1 + s2 + s3 + s4 + s5 + s6 + s7;
      const double t1 = omp_get_wtime();
      elapsed = t1 - t0;
      if (verbose) {
        printf(" time=%g sec\n", elapsed);
      }
      if (elapsed >= min_elapsed) break;
      max_count *= llrint(max(2.0, min(10.0, 1.1 * min_elapsed / elapsed)));
    }
    cpu_info.iop_speed = max_count * 8 * 2 / elapsed;
    if (verbose) {
      printf("    result:");
    }
    printf(" %g Giop/sec for each PU\n", cpu_info.iop_speed / 1.0e+9);
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
  
  
  
  void measure_read_latency()
  {
    DECLARE_CCTK_PARAMETERS;
    
    printf("  Read latency:\n");
    // Loop over all cache levels and memory types
    for (int cache=0; cache<int(cache_info.size()); ++cache) {
      // Determine size
      ptrdiff_t skipsize, size;
      calc_sizes(cache, skipsize, size);
      assert(size>0);
      const ptrdiff_t step = cache_info[cache].linesize;
      assert(step>0);
      if (verbose) {
        printf("    %s read latency (using %td bytes):\n",
               cache_info[cache].name.c_str(), size);
        fflush(stdout);
      } else {
        printf("    %s read latency:", cache_info[cache].name.c_str());
      }
      // Allocate skipped memory, filling it with 1 so that it is
      // actually allocated by the operating system
      vector<char> skiparray(skipsize, 1);
      const ptrdiff_t offset = 0xa1d2d5ff; // a random number
      const ptrdiff_t nmax = size / sizeof(void*);
      // Linked list (see latex)
      vector<void*> array(nmax);
      {
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
      // The basic benchmark harness is the same as above, no comments
      // here
      double min_elapsed = 1.0;
      ptrdiff_t max_count = 1000;
      double elapsed = 0.0;
      for (;;) {
        if (verbose) {
          printf("      iterations=%td...", max_count);
          fflush(stdout);
        }
        const double t0 = omp_get_wtime();
        void* ptr = &array[0];
        // Chase linked list (see latex)
        for (ptrdiff_t count=0; count<max_count; ++count) {
#define REPEAT10(x) x x x x x x x x x x
          REPEAT10(REPEAT10(ptr = *(void**)ptr;));
#undef REPEAT10
        }
        volatile bool use_ptr CCTK_ATTRIBUTE_UNUSED = ptr;
        const double t1 = omp_get_wtime();
        elapsed = t1 - t0;
        if (verbose) {
          printf(" time=%g sec\n", elapsed);
        }
        if (elapsed >= min_elapsed) break;
        max_count *= llrint(max(2.0, min(10.0, 1.1 * min_elapsed / elapsed)));
      }
      cache_info[cache].read_latency = elapsed / (max_count * 100);
      if (verbose) {
        printf("      result:");
      }
      printf(" %g nsec\n", cache_info[cache].read_latency * 1.0e+9);
    }
  }
  
  
  
  void measure_read_bandwidth()
  {
    DECLARE_CCTK_PARAMETERS;
    
    printf("  Read bandwidth:\n");
    // The basic benchmark harness is the same as above, no comments
    // here
    for (int cache=0; cache<int(cache_info.size()); ++cache) {
      ptrdiff_t skipsize, size;
      calc_sizes(cache, skipsize, size);
      assert(size>0);
      if (verbose) {
        printf("    %s read bandwidth (using %td bytes):\n",
               cache_info[cache].name.c_str(), size);
        fflush(stdout);
      } else {
        printf("    %s read bandwidth:", cache_info[cache].name.c_str());
      }
      vector<char> skiparray(skipsize, 1);
      const ptrdiff_t nmax = size / sizeof(CCTK_REAL);
      // Allocate array, set all elements to 1.0
      vector<CCTK_REAL> raw_array(nmax + CCTK_REAL_VEC_SIZE-1, 1.0);
      // Align array
      CCTK_REAL* restrict array = &raw_array[CCTK_REAL_VEC_SIZE-1];
      array = (CCTK_REAL*)(ptrdiff_t(array) & -sizeof(CCTK_REAL_VEC));
      double min_elapsed = 1.0;
      ptrdiff_t max_count = 1;
      double elapsed = 0.0;
      for (;;) {
        if (verbose) {
          printf("      iterations=%td...", max_count);
          fflush(stdout);
        }
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
          volatile CCTK_REAL_VEC use_s CCTK_ATTRIBUTE_UNUSED =
            kadd(kadd(kadd(s0, s1), kadd(s2, s3)),
                 kadd(kadd(s4, s5), kadd(s6, s7)));
        }
        const double t1 = omp_get_wtime();
        elapsed = t1 - t0;
        if (verbose) {
          printf(" time=%g sec\n", elapsed);
        }
        if (elapsed >= min_elapsed) break;
        max_count *= llrint(max(2.0, min(10.0, 1.1 * min_elapsed / elapsed)));
      }
      cache_info[cache].read_bandwidth = max_count * size / elapsed;
      if (verbose) {
        printf("      result:");
      }
      printf(" %g GByte/sec for %d PUs\n",
             cache_info[cache].read_bandwidth / 1.0e+9,
             cache_info[cache].num_pus);
    }
  }
  
  
  
  void measure_write_latency()
  {
    DECLARE_CCTK_PARAMETERS;
    
    printf("  Write latency:\n");
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
      if (verbose) {
        printf("    %s write latency (using %td bytes):\n",
               cache_info[cache].name.c_str(), size);
        fflush(stdout);
      } else {
        printf("    %s write latency:", cache_info[cache].name.c_str());
      }
      vector<char> skiparray(skipsize, 1);
      vector<char> array_(size, 1);
      char* restrict array = &array_[0];
      double min_elapsed = 1.0;
      ptrdiff_t max_count = 1000;
      double elapsed = 0.0;
      while (elapsed < min_elapsed) {
        if (verbose) {
          printf("      iterations=%td...", max_count);
          fflush(stdout);
        }
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
        volatile char use_array CCTK_ATTRIBUTE_UNUSED = array[0];
        const double t1 = omp_get_wtime();
        elapsed = t1 - t0;
        if (verbose) {
          printf(" time=%g sec\n", elapsed);
        }
        max_count *= llrint(max(2.0, min(10.0, 1.1 * min_elapsed / elapsed)));
      }
      cache_info[cache].write_latency = elapsed / (max_count * 8);
      if (verbose) {
        printf("      result:");
      }
      printf(" %g nsec\n", cache_info[cache].write_latency * 1.0e+9);
    }
  }
  
  
  
  void measure_write_bandwidth()
  {
    DECLARE_CCTK_PARAMETERS;
    
    printf("  Write bandwidth via memset:\n");
    // The basic benchmark harness is the same as above, no comments
    // here
    for (int cache=0; cache<int(cache_info.size()); ++cache) {
      ptrdiff_t skipsize, size;
      calc_sizes(cache, skipsize, size);
      assert(size>0);
      if (verbose) {
        printf("    %s write bandwidth (using %td bytes):\n",
               cache_info[cache].name.c_str(), size);
        fflush(stdout);
      } else {
        printf("    %s write bandwidth:", cache_info[cache].name.c_str());
      }
      vector<char> skiparray(skipsize, 1);
      vector<char> array(size, 1);
      double min_elapsed = 1.0;
      ptrdiff_t max_count = 1;
      double elapsed = 0.0;
      for (;;) {
        if (verbose) {
          printf("      iterations=%td...", max_count);
          fflush(stdout);
        }
        const double t0 = omp_get_wtime();
        // Use memset for writing (see latex)
        for (ptrdiff_t count=0; count<max_count; ++count) {
          memset(&array[0], count % 256, size);
          volatile char use_array CCTK_ATTRIBUTE_UNUSED = array[count % size];
        }
        const double t1 = omp_get_wtime();
        elapsed = t1 - t0;
        if (verbose) {
          printf(" time=%g sec\n", elapsed);
        }
        if (elapsed >= min_elapsed) break;
        max_count *= llrint(max(2.0, min(10.0, 1.1 * min_elapsed / elapsed)));
      }
      cache_info[cache].write_bandwidth = max_count * size / elapsed;
      if (verbose) {
        printf("      result:");
      }
      printf(" %g GByte/sec for %d PUs\n",
             cache_info[cache].write_bandwidth / 1.0e+9,
             cache_info[cache].num_pus);
    }
  }
  
  
  
  void measure_write_bandwidth2()
  {
    DECLARE_CCTK_PARAMETERS;
    
    printf("  Write bandwidth via cache-bypassing stores:\n");
    // The basic benchmark harness is the same as above, no comments
    // here
    for (int cache=0; cache<int(cache_info.size()); ++cache) {
      if (cache_info[cache].type==1) { // only if memory
        ptrdiff_t skipsize, size;
        calc_sizes(cache, skipsize, size);
        assert(size>0);
        if (verbose) {
          printf("    %s write bandwidth (using %td bytes):\n",
                 cache_info[cache].name.c_str(), size);
          fflush(stdout);
        } else {
          printf("    %s write bandwidth:", cache_info[cache].name.c_str());
        }
        vector<char> skiparray(skipsize, 1);
        const ptrdiff_t nmax = size / sizeof(CCTK_REAL);
        // Allocate array, set all elements to 1.0
        vector<CCTK_REAL> raw_array(nmax + CCTK_REAL_VEC_SIZE-1, 1.0);
        // Align array
        CCTK_REAL* restrict array = &raw_array[CCTK_REAL_VEC_SIZE-1];
        array = (CCTK_REAL*)(ptrdiff_t(array) & -sizeof(CCTK_REAL_VEC));
        double min_elapsed = 1.0;
        ptrdiff_t max_count = 1;
        double elapsed = 0.0;
        for (;;) {
          if (verbose) {
            printf("      iterations=%td...", max_count);
            fflush(stdout);
          }
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
          }
          const double t1 = omp_get_wtime();
          elapsed = t1 - t0;
          if (verbose) {
            printf(" time=%g sec\n", elapsed);
          }
          if (elapsed >= min_elapsed) break;
          max_count *= llrint(max(2.0, min(10.0, 1.1 * min_elapsed / elapsed)));
        }
        cache_info[cache].write_bandwidth = max_count * size / elapsed;
        if (verbose) {
          printf("      result:");
        }
        printf(" %g GByte/sec for %d PUs\n",
               cache_info[cache].write_bandwidth / 1.0e+9,
               cache_info[cache].num_pus);
      }
    }
  }
  
}



extern "C"
void MemSpeed_MeasureSpeed(CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS;
  
  if (CCTK_MyProc(cctkGH) != 0) return;
  
  CCTK_INFO("Measuring CPU, cache, and memory speeds:");
  load_cache_info();
  measure_cpu_cycle_speed();
  measure_cpu_flop_speed();
  measure_cpu_iop_speed();
  measure_read_latency();
  measure_read_bandwidth();
  measure_write_latency();
  measure_write_bandwidth();
  measure_write_bandwidth2();
}
