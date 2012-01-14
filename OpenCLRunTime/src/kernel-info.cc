#include "defs.hh"
#include "kernel.hh"

#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

#include <sys/types.h>
#include <sys/wait.h>



namespace OpenCLRunTime {
  
  
  
  // Output a disassembled listing
  void OpenCLKernel::disassemble() const
  {
    DECLARE_CCTK_PARAMETERS;
    
    if (not disassemble_kernels) return;
    
    // Only disassemble on the root process
    if (CCTK_MyProc(NULL) != 0) return;
    
    // Disassemble in a subprocess because it may be slow
    pid_t const cpid = fork();
    if (cpid > 0) {
      cout << "Disassembling kernel in process " << cpid << "\n";
      
      if (not disassemble_in_background) {
        cout << "Waiting for disassembling to finish...";
        cout.flush();
        int status;
        waitpid(cpid, &status, 0);
        cout << " done\n";
        cout.flush();
      }
      
      return;
    }
    
    cl_platform_id platform_id;
    checkErr(clGetDeviceInfo(device->device_id, CL_DEVICE_PLATFORM,
                             sizeof platform_id, &platform_id, NULL));
    size_t platform_name_size;
    checkErr(clGetPlatformInfo(platform_id, CL_PLATFORM_NAME,
                               0, NULL, &platform_name_size));
    char platform_name[platform_name_size];
    checkErr(clGetPlatformInfo(platform_id, CL_PLATFORM_NAME,
                               platform_name_size, platform_name, NULL));
    enum vendor_t { v_AMD, v_Apple, v_Intel, v_NVidia, v_pocl };
    vendor_t vendor;
    if (strcasestr(platform_name, "AMD")) {
      vendor = v_AMD;
    } else if (strcasestr(platform_name, "Apple")) {
      vendor = v_Apple;
    } else if (strcasestr(platform_name, "Intel")) {
      vendor = v_Intel;
    } else if (strcasestr(platform_name, "NVidia")) {
      vendor = v_NVidia;
    } else if (strcasestr(platform_name, "pocl")) {
      vendor = v_pocl;
    } else {
      CCTK_WARN(CCTK_WARN_ALERT, "Unknown OpenCL architecture");
      _exit(0);
    }
    
    switch (vendor) {
      
    case v_NVidia:
    case v_pocl: {
      // Don't do anything, because the "binary" is already an
      // assembler listing
      break;
    }
      
    case v_Intel: {
      // Call the compiler to produce assembler code
      
      // TODO: determine path dynamically
      char const *const path = "/usr/local/intel_ocl_sdk_1.5_x64/usr";
      stringstream cc;
      cc << "time env "
         << "CLASSPATH=\"" << path << "/lib64/OpenCL/vendors/intel:$CLASSPATH\" "
         << "LD_LIBRARY_PATH=\"" << path << "/lib64/OpenCL/vendors/intel:$LD_LIBRARY_PATH\" "
         << "PATH=\"" << path << "/lib64/OpenCL/vendors/intel:$PATH\" "
         << "\"" << path << "/bin/ioc\" "
         << "-input=\"" << out_dir << "/" << name << ".cl\" "
         << "-asm=\"" << out_dir << "/" << name << ".s\" "
         << ">/dev/null 2>&1";
      stringstream filename;
      filename << out_dir << "/" << name << ".log";
      fstream file(filename.str().c_str(), ios::out | ios::app);
      file << "\n"
           << "Disassembling:\n"
           << cc.str() << "\n";
      file.close();
      system(cc.str().c_str());
      break;
    }
      
    case v_AMD:
    case v_Apple: {
      // Call gdb to disassemble the memory content
      
      char *const cmdfilename = tempnam(NULL, NULL);
      ofstream cmds(cmdfilename);
      switch (vendor) {
      case v_AMD:
        cmds << "disassemble __OpenCL_" << name << "_kernel\n";
        break;
      case v_Apple:
        cmds << "disassemble " << name << "_wrapper\n";
        break;
      default:
        assert(0);
      }
      cmds.close();
      
      char **argv;
      int const argc = CCTK_CommandLine(&argv);
      assert(argc >= 1);
      pid_t const pid = getpid();
      
      stringstream gdb;
      gdb << "time gdb -batch -x " << cmdfilename << " "
          << argv[0] << " " << pid << " "
          << ">" << out_dir << "/" << name << ".s 2>&1";
      system(gdb.str().c_str());
      
      remove(cmdfilename);
      
      break;
    }
      
    default:
      CCTK_WARN(CCTK_WARN_ALERT, "Unknown OpenCL architecture");
      _exit(0);
    }
    
    // Exit the hard way, not via exit(), so that MPI etc. don't get
    // confused. Note that we don't need to clean up anything.
    _exit(0);
  }
  
  
  
} // namespace OpenCLRunTime
