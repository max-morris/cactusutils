#include "defs.hh"
#include "kernel.hh"

#include <cctk.h>
#include <cctk_Parameters.h>

#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace OpenCLRunTime {

// Output a disassembled listing
void OpenCLKernel::disassemble() const {
  DECLARE_CCTK_PARAMETERS;

  if (not disassemble_kernels)
    return;

  // Only disassemble on the root process
  if (CCTK_MyProc(NULL) != 0)
    return;

// Don't use fork, MPI may not like it
#if 0
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
#endif

  cl_platform_id platform_id;
  checkErr(clGetDeviceInfo(device->device_id, CL_DEVICE_PLATFORM,
                           sizeof platform_id, &platform_id, NULL));
  size_t platform_name_size;
  checkErr(clGetPlatformInfo(platform_id, CL_PLATFORM_NAME, 0, NULL,
                             &platform_name_size));
  char platform_name[platform_name_size];
  checkErr(clGetPlatformInfo(platform_id, CL_PLATFORM_NAME, platform_name_size,
                             platform_name, NULL));
  enum vendor_t { v_AMD, v_Apple, v_Intel, v_Nvidia, v_pocl };
  vendor_t vendor;
  if (strcasestr(platform_name, "AMD")) {
    vendor = v_AMD;
  } else if (strcasestr(platform_name, "Apple")) {
    vendor = v_Apple;
  } else if (strcasestr(platform_name, "Intel")) {
    vendor = v_Intel;
  } else if (strcasestr(platform_name, "Nvidia")) {
    vendor = v_Nvidia;
  } else if (strcasestr(platform_name, "pocl")) {
    vendor = v_pocl;
  } else {
    CCTK_WARN(CCTK_WARN_ALERT, "Unknown OpenCL architecture");
    // _exit(0);
    return;
  }

  switch (vendor) {

  case v_Nvidia:
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
       << "LD_LIBRARY_PATH=\"" << path
       << "/lib64/OpenCL/vendors/intel:$LD_LIBRARY_PATH\" "
       << "PATH=\"" << path << "/lib64/OpenCL/vendors/intel:$PATH\" "
       << "\"" << path << "/bin/ioc\" "
       << "-input=\"" << out_dir << "/" << name << ".cl\" "
       << "-asm=\"" << out_dir << "/" << name << ".s\" "
       << ">/dev/null 2>&1";
    stringstream filename;
    filename << out_dir << "/" << name << ".log";
    fstream file(filename.str().c_str(), ios::out | ios::app);
    file << "\n"
         << "Disassembling:\n" << cc.str() << "\n";
    file.close();
    system(cc.str().c_str());
    break;
  }

  case v_AMD: {
    // Call gdb to disassemble the memory content

    char *const cmdfilename = tempnam(NULL, NULL);
    ofstream cmds(cmdfilename);
    cmds << "disassemble __OpenCL_" << name << "_kernel\n";
    cmds.close();

    char **argv;
    int const argc = CCTK_CommandLine(&argv);
    assert(argc >= 1);
    pid_t const pid = getpid();

    stringstream gdb;
    gdb << "time gdb -batch -x " << cmdfilename << " " << argv[0] << " " << pid
        << " "
        << ">" << out_dir << "/" << name << ".s 2>&1";
    system(gdb.str().c_str());

    remove(cmdfilename);

    break;
  }

  case v_Apple: {
    // Post-process the object file, then call objdump

    // Read the file into memory
    string contents;
    {
      stringstream buf;
      buf << out_dir << "/" << name << ".0.o";
      string const filename = buf.str();
      ifstream file(filename.c_str(), ios::in | ios::binary);
      assert(file);
      file.seekg(0, ios::end);
      contents.resize(file.tellg());
      file.seekg(0, ios::beg);
      file.read(&contents[0], contents.size());
      file.close();
    }

    // Skip the beginning of the object file until the byte sequence
    // cf fa ed fe is encountered
    {
      string const magic = "\xcf\xfa\xed\xfe";
      size_t const pos = contents.find(magic);
      assert(pos != string::npos);
      contents = contents.substr(pos);
    }

    // Write the new contents
    {
      stringstream buf;
      buf << out_dir << "/" << name << ".o";
      string const filename = buf.str();
      ofstream file(filename.c_str(), ios::out | ios::binary);
      assert(file);
      file << contents;
      file.close();
    }

    // Call objdump
    {
      stringstream cmd;
      cmd << "gobjdump -d " << out_dir << "/" << name << ".o "
          << ">" << out_dir << "/" << name << ".s 2>&1";
      system(cmd.str().c_str());
    }

    break;
  }

  default:
    CCTK_WARN(CCTK_WARN_ALERT, "Unknown OpenCL architecture");
    // _exit(0);
    return;
  }

#if 0
    // Exit the hard way, not via exit(), so that MPI etc. don't get
    // confused. Note that we don't need to clean up anything.
    _exit(0);
#endif
}

} // namespace OpenCLRunTime
