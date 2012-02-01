#include "accelerator.hh"
#include <cctk.h>

namespace Accelerator {
  
  extern "C"
  int Accelerator_Init()
  {
    device = new device_t;
    return 0;
  }
  
} // namespace Accelerator
