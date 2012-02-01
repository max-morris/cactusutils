#include "copy.hh"
#include "device.hh"

#include <cctk_Parameters.h>
#include <util_Table.h>

#include <carpet.hh>

#include <cassert>

#ifdef CL_VERSION_1_1
#  define HAVE_BUFFER_RECT_OPS 1
#else
#  define HAVE_BUFFER_RECT_OPS 0
#endif



namespace OpenCLRunTime {
  
  
  
  extern "C"
  CCTK_INT
  OpenCLRunTime_CopyCycle(CCTK_POINTER_TO_CONST const cctkGH_,
                          CCTK_INT const vis[],
                          CCTK_INT const tls[],
                          CCTK_INT const nvars)
  {
    cGH const *restrict const cctkGH = static_cast<cGH const*>(cctkGH_);
    assert(Carpet::is_local_mode());
    
    if (device->mem_model == mm_always_mapped) return 0;
    assert(device->mem_model != mm_map);
    
    int const NP =
      device->grid.lsh[0] * device->grid.lsh[1] * device->grid.lsh[2];
    
    for (int var=0; var<nvars; ++var) {
      int const vi = vis[var];
      int const num_tl = device->mems.at(vi).size();
      if (num_tl > 1) {
        cl_event event;
        bool have_event = false;
        for (int tl=num_tl-1; tl>0; --tl) {
          cl_event new_event;
          checkErr(clEnqueueCopyBuffer(device->queue,
                                       device->mems.at(vi).at(tl-1).mem,
                                       device->mems.at(vi).at(tl).mem,
                                       0, 0, NP*sizeof(CCTK_REAL),
                                       have_event ? 1 : 0,
                                       have_event ? &event : NULL,
                                       &new_event));
          event = new_event;
          have_event = true;
        }
      }
    }
    
    return 0;
  }
  
  
  
  extern "C"
  CCTK_INT
  OpenCLRunTime_CopyFromPast(CCTK_POINTER_TO_CONST const cctkGH_,
                             CCTK_INT const vis[],
                             CCTK_INT const tls[],
                             CCTK_INT const nvars)
  {
    cGH const *restrict const cctkGH = static_cast<cGH const*>(cctkGH_);
    assert(Carpet::is_local_mode());
    
    if (device->mem_model == mm_always_mapped) return 0;
    assert(device->mem_model != mm_map);
    
    int const NP =
      device->grid.lsh[0] * device->grid.lsh[1] * device->grid.lsh[2];
    
    for (int var=0; var<nvars; ++var) {
      int const vi=vis[var];
      assert(vi>=0);
      int const tl=tls[var];
      assert(tl>=0);
      assert(tl+1 < int(device->mems.at(vi).size()));
      
      checkErr(clEnqueueCopyBuffer(device->queue,
                                   device->mems.at(vi).at(tl+1).mem,
                                   device->mems.at(vi).at(tl).mem,
                                   0, 0, NP*sizeof(CCTK_REAL),
                                   0, NULL, NULL));
    } // for var
    
    return 0;
  }
  
  
  
  extern "C"
  CCTK_INT
  OpenCLRunTime_CopyToDevice(CCTK_POINTER_TO_CONST const cctkGH_,
                             CCTK_INT const vis[],
                             CCTK_INT const tls[],
                             CCTK_INT const nvars,
                             CCTK_INT *const moved)
  {
    cGH const *restrict const cctkGH = static_cast<cGH const*>(cctkGH_);
    assert(Carpet::is_local_mode());
    
    *moved = 0;
    
    switch (device->mem_model) {
      
    case mm_always_mapped:
      // do nothing
      break;
      
    case mm_copy: {
      
      int const np =
        cctkGH->cctk_lsh[0] * cctkGH->cctk_lsh[1] * cctkGH->cctk_lsh[2];
      
      int const di = sizeof(CCTK_REAL);
      int const dj = di * cctkGH->cctk_lsh[0];
      int const dk = dj * cctkGH->cctk_lsh[1];
      int const dI = sizeof(CCTK_REAL);
      int const dJ = dI * device->grid.lsh[0];
      int const dK = dJ * device->grid.lsh[1];
      
      size_t offset[dim];
      size_t length[dim];
      for (int d=0; d<dim; ++d) {
        offset[d] = 0;
        length[d] = cctkGH->CCTK_LSSH(0,d);
      }
      offset[0] *= di;
      length[0] *= di;
      
      for (int var=0; var<nvars; ++var) {
        int const vi=vis[var];
        assert(vi>=0);
        int const tl=tls[var];
        assert(tl>=0);
        if (int(device->mems.at(vi).size()) > tl) {
          
          void *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
          
          if (HAVE_BUFFER_RECT_OPS and device->same_padding) {
            checkErr(clEnqueueWriteBuffer(device->queue,
                                          device->mems.at(vi).at(tl).mem,
                                          CL_FALSE,
                                          0, np*sizeof(CCTK_REAL), ptr,
                                          0, NULL, NULL));
          } else {
#if HAVE_BUFFER_RECT_OPS
            checkErr(clEnqueueWriteBufferRect(device->queue,
                                              device->mems.at(vi).at(tl).mem,
                                              CL_FALSE,
                                              offset, offset, length,
                                              dJ, dK, dj, dk,
                                              ptr,
                                              0, NULL, NULL));
#else
            assert(0);
#endif
          }
          
        }
      } // for var
      
      break;
    }
      
    case mm_map: {
      for (int var=0; var<nvars; ++var) {
        int const vi=vis[var];
        assert(vi>=0);
        int const tl=tls[var];
        assert(tl>=0);
        if (int(device->mems.at(vi).size()) > tl) {
          
          void *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
          
          assert (device->same_padding);
          
          checkErr(clEnqueueUnmapMemObject(device->queue,
                                           device->mems.at(vi).at(tl).mem,
                                           ptr,
                                           0, NULL, NULL));
          
        }
      } // for var
      
      *moved = 1;
      
      break;
    }
      
    default:
      assert(0);
    }
    
    return 0;
  }
  
  
  
  extern "C"
  CCTK_INT
  OpenCLRunTime_CopyToHost(CCTK_POINTER_TO_CONST const cctkGH_,
                           CCTK_INT const vis[],
                           CCTK_INT const tls[],
                           CCTK_INT const nvars,
                           CCTK_INT *const moved)
  {
    cGH const *restrict const cctkGH = static_cast<cGH const*>(cctkGH_);
    assert(Carpet::is_local_mode());
    
    int const np =
      cctkGH->cctk_lsh[0] * cctkGH->cctk_lsh[1] * cctkGH->cctk_lsh[2];
    
    *moved = 0;
    
    cl_int errcode;
    
    switch (device->mem_model) {
      
    case mm_always_mapped:
      // do nothing
      break;
      
    case mm_copy: {
      
      int const di = sizeof(CCTK_REAL);
      int const dj = di * cctkGH->cctk_lsh[0];
      int const dk = dj * cctkGH->cctk_lsh[1];
      int const dI = sizeof(CCTK_REAL);
      int const dJ = dI * device->grid.lsh[0];
      int const dK = dJ * device->grid.lsh[1];
      
      size_t offset[dim];
      size_t length[dim];
      for (int d=0; d<dim; ++d) {
        offset[d] = 0;
        length[d] = cctkGH->CCTK_LSSH(0,d);
      }
      offset[0] *= di;
      length[0] *= di;
      
      for (int var=0; var<nvars; ++var) {
        int const vi=vis[var];
        assert(vi>=0);
        int const tl=tls[var];
        assert(tl>=0);
        if (int(device->mems.at(vi).size()) > tl) {
          
          void *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
          
          if (HAVE_BUFFER_RECT_OPS and device->same_padding) {
            checkErr(clEnqueueReadBuffer(device->queue,
                                         device->mems.at(vi).at(tl).mem,
                                         CL_FALSE,
                                         0, np*sizeof(CCTK_REAL), ptr,
                                         0, NULL, NULL));
          } else {
#if HAVE_BUFFER_RECT_OPS
            checkErr(clEnqueueReadBufferRect(device->queue,
                                             device->mems.at(vi).at(tl).mem,
                                             CL_FALSE,
                                             offset, offset, length,
                                             dJ, dK, dj, dk,
                                             ptr,
                                             0, NULL, NULL));
#else
            assert(0);
#endif
          }
          
        }
      } // for var
      
      break;
    }
      
    case mm_map: {
      for (int var=0; var<nvars; ++var) {
        int const vi=vis[var];
        assert(vi>=0);
        int const tl=tls[var];
        assert(tl>=0);
        if (int(device->mems.at(vi).size()) > tl) {
          
          // TODO: Check this. For now, we just assume this is true,
          // because we don't assume that all provides/requires
          // information is complete and correct.
          // assert(device->mems.at(vi).at(tl).host_valid);
          
          assert (device->same_padding);
          
          void *ptr;
          checkErr((ptr = clEnqueueMapBuffer(device->queue,
                                             device->mems.at(vi).at(tl).mem,
                                             CL_FALSE,
                                             CL_MAP_READ | CL_MAP_WRITE,
                                             0, np*sizeof(CCTK_REAL),
                                             0, NULL, NULL, &errcode),
                    errcode));
          
          assert(ptr == CCTK_VarDataPtrI(cctkGH, tl, vi));
          
        }
      } // for var
      
      *moved = 1;
      
      break;
    }
      
    default:
      assert(0);
    }
    
    // Finish, because we output
    checkErr(clFinish(device->queue));
    
    return 0;
  }
  
  
  
  extern "C"
  CCTK_INT
  OpenCLRunTime_CopyPreSync(CCTK_POINTER_TO_CONST const cctkGH_,
                            CCTK_INT const vis[],
                            CCTK_INT const tls[],
                            CCTK_INT const nvars)
  {
    cGH const *restrict const cctkGH = static_cast<cGH const*>(cctkGH_);
    assert(Carpet::is_local_mode());
    
    if (device->mem_model == mm_always_mapped) return 0;
    assert (device->mem_model != mm_map);
    
    int const dI = sizeof(CCTK_REAL);
    int const dJ = dI * device->grid.lsh[0];
    int const dK = dJ * device->grid.lsh[1];
    int const di = sizeof(CCTK_REAL);
    int const dj = di * cctkGH->cctk_lsh[0];
    int const dk = dj * cctkGH->cctk_lsh[1];
    
    for (int var=0; var<nvars; ++var) {
      int const vi=vis[var];
      assert(vi>=0);
      int const tl=tls[var];
      assert(tl>=0);
      assert (tl < int(device->mems.at(vi).size()));
      
      void *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
      
#if HAVE_BUFFER_RECT_OPS
      
      for (int dir=0; dir<dim; ++dir) {
        for (int face=0; face<2; ++face) {
          if (not cctkGH->cctk_bbox[2*dir+face]) {
            
            int imin[dim];
            int imax[dim];
            for (int d=0; d<dim; ++d) {
              // Whole domain
              imin[d] = 0;
              imax[d] = cctkGH->CCTK_LSSH(0,d);
              // Skip ghost points
              if (not cctkGH->cctk_bbox[2*d+0]) {
                imin[d] += cctkGH->cctk_nghostzones[d];
              }
              if (not cctkGH->cctk_bbox[2*d+1]) {
                imax[d] -= cctkGH->cctk_nghostzones[d];
              }
              // Skip points that will be copied via later directions
              if (d > dir) {
                if (not cctkGH->cctk_bbox[2*d+0]) {
                  imin[d] += cctkGH->cctk_nghostzones[d];
                }
                if (not cctkGH->cctk_bbox[2*d+1]) {
                  imax[d] -= cctkGH->cctk_nghostzones[d];
                }
              }
            }
            if (face==0) {
              imin[dir] = cctkGH->cctk_nghostzones[dir];
              imax[dir] = imin[dir] + cctkGH->cctk_nghostzones[dir];
            } else {
              imax[dir] =
                cctkGH->CCTK_LSSH(0,dir) - cctkGH->cctk_nghostzones[dir];
              imin[dir] = imax[dir] - cctkGH->cctk_nghostzones[dir];
            }
            for (int d=0; d<dim; ++d) {
              assert(imin[d] >= 0);
              assert(imin[d] <= imax[d]);
              assert(imax[d] <= cctkGH->CCTK_LSSH(0,d));
            }
            
            size_t offset[dim];
            size_t length[dim];
            for (int d=0; d<dim; ++d) {
              offset[d] = imin[d];
              length[d] = imax[d] - imin[d];
            }
            offset[0] *= di;
            length[0] *= di;
            
            checkErr(clEnqueueReadBufferRect(device->queue,
                                             device->mems.at(vi).at(tl).mem,
                                             CL_FALSE,
                                             offset, offset, length,
                                             dJ, dK, dj, dk,
                                             ptr,
                                             0, NULL, NULL));
            
          } // if bbox
        }   // for face
      }     // for dir
      
#else
      
      bool have_sync_bnd = false;
      for (int dir=0; dir<dim; ++dir) {
        for (int face=0; face<2; ++face) {
          if (not cctkGH->cctk_bbox[2*dir+face]) {
            have_sync_bnd = true;
          }
        }
      }
      
      if (have_sync_bnd) {
        int const np =
          cctkGH->cctk_lsh[0] * cctkGH->cctk_lsh[1] * cctkGH->cctk_lsh[2];
        
        checkErr(clEnqueueReadBuffer(device->queue,
                                     device->mems.at(vi).at(tl).mem,
                                     CL_FALSE,
                                     0, np*sizeof(CCTK_REAL), ptr,
                                     0, NULL, NULL));
      }
      
#endif
      
    } // for var
    
    // Finish, because we sync
    checkErr(clFinish(device->queue));
    
    return 0;
  }
  
  
  
  extern "C"
  CCTK_INT
  OpenCLRunTime_CopyPostSync(CCTK_POINTER_TO_CONST const cctkGH_,
                             CCTK_INT const vis[],
                             CCTK_INT const tls[],
                             CCTK_INT const nvars)
  {
    cGH const *restrict const cctkGH = static_cast<cGH const*>(cctkGH_);
    assert(Carpet::is_local_mode());
    
    if (device->mem_model == mm_always_mapped) return 0;
    assert (device->mem_model != mm_map);
    
    int const dI = sizeof(CCTK_REAL);
    int const dJ = dI * device->grid.lsh[0];
    int const dK = dJ * device->grid.lsh[1];
    int const di = sizeof(CCTK_REAL);
    int const dj = di * cctkGH->cctk_lsh[0];
    int const dk = dj * cctkGH->cctk_lsh[1];
    
    for (int var=0; var<nvars; ++var) {
      int const vi=vis[var];
      assert(vi>=0);
      int const tl=tls[var];
      assert(tl>=0);
      assert (tl < int(device->mems.at(vi).size()));
      
      void const *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
      
#if HAVE_BUFFER_RECT_OPS
      
      for (int dir=0; dir<dim; ++dir) {
        for (int face=0; face<2; ++face) {
          if (not cctkGH->cctk_bbox[2*dir+face]) {
            
            int imin[dim];
            int imax[dim];
            for (int d=0; d<dim; ++d) {
              // Whole domain
              imin[d] = 0;
              imax[d] = cctkGH->CCTK_LSSH(0,d);
              // Skip points that will be copied via later directions
              if (d > dir) {
                if (not cctkGH->cctk_bbox[2*d+0]) {
                  imin[d] += cctkGH->cctk_nghostzones[d];
                }
                if (not cctkGH->cctk_bbox[2*d+1]) {
                  imax[d] -= cctkGH->cctk_nghostzones[d];
                }
              }
            }
            if (face==0) {
              imin[dir] = 0;
              imax[dir] = imin[dir] + cctkGH->cctk_nghostzones[dir];
            } else {
              imax[dir] = cctkGH->CCTK_LSSH(0,dir);
              imin[dir] = imax[dir] - cctkGH->cctk_nghostzones[dir];
            }
            for (int d=0; d<dim; ++d) {
              assert(imin[d] >= 0);
              assert(imin[d] <= imax[d]);
              assert(imax[d] <= cctkGH->CCTK_LSSH(0,d));
            }
            
            size_t offset[dim];
            size_t length[dim];
            for (int d=0; d<dim; ++d) {
              offset[d] = imin[d];
              length[d] = imax[d] - imin[d];
            }
            offset[0] *= di;
            length[0] *= di;
            
            checkErr(clEnqueueWriteBufferRect(device->queue,
                                              device->mems.at(vi).at(tl).mem,
                                              CL_FALSE,
                                              offset, offset, length,
                                              dJ, dK, dj, dk,
                                              ptr,
                                              0, NULL, NULL));

          } // if bbox
        }   // if face
      }     // if dir
      
#else
      
      bool have_sync_bnd = false;
      for (int dir=0; dir<dim; ++dir) {
        for (int face=0; face<2; ++face) {
          if (not cctkGH->cctk_bbox[2*dir+face]) {
            have_sync_bnd = true;
          }
        }
      }
      
      if (have_sync_bnd) {
        int const np =
          cctkGH->cctk_lsh[0] * cctkGH->cctk_lsh[1] * cctkGH->cctk_lsh[2];
        
        checkErr(clEnqueueWriteBuffer(device->queue,
                                      device->mems.at(vi).at(tl).mem,
                                      CL_FALSE,
                                      0, np*sizeof(CCTK_REAL), ptr,
                                      0, NULL, NULL));
      }
      
#endif            
            
    } // for var
    
    return 0;
  }
  
  
  
} // namespace OpenCLRunTime
