#include "copy.hh"
#include "device.hh"

#include <cctk_Parameters.h>
#include <util_Table.h>

#include <carpet.hh>

#include <cassert>
#include <cstdlib>
#include <sstream>

#ifdef CL_VERSION_1_1
#define HAVE_BUFFER_RECT_OPS 1
#else
#define HAVE_BUFFER_RECT_OPS 0
#endif

namespace OpenCLRunTime {

static string vars_to_string(CCTK_INT const vis[], CCTK_INT const rls[],
                             CCTK_INT const tls[], CCTK_INT const nvars) {
  stringstream buf;
  for (int var = 0; var < nvars; ++var) {
    int const vi = vis[var];
    int const rl = rls[var];
    int const tl = tls[var];
    char *const fullname = CCTK_FullName(vi);
    if (var > 0)
      buf << " ";
    buf << fullname << ",rl=" << rl << ",tl=" << tl;
    free(fullname);
  }
  return buf.str();
}

extern "C" CCTK_INT
OpenCLRunTime_CreateVariables(CCTK_POINTER_TO_CONST const cctkGH_,
                              CCTK_INT const vis[], CCTK_INT const rls[],
                              CCTK_INT const tls[], CCTK_INT const nvars) {
  cGH const *restrict const cctkGH = static_cast<cGH const *>(cctkGH_);
  DECLARE_CCTK_PARAMETERS;

  if (veryverbose) {
    CCTK_VInfo(CCTK_THORNSTRING, "CreateVariables: %s",
               vars_to_string(vis, rls, tls, nvars).c_str());
  }

  if (nvars == 0)
    return 0;

  assert(Carpet::is_local_mode());
  assert(device);
  assert(device->have_grid());

  cl_mem_flags mem_flags;
  switch (device->mem_model) {
  case mm_always_mapped:
    // Re-use the memory of the grid functions; we never map or copy
    mem_flags = CL_MEM_USE_HOST_PTR;
    break;
  case mm_copy:
    // Allocate memory, and copy (don't map) from/to grid functions
    mem_flags = 0 /*CL_MEM_ALLOC_HOST_PTR*/;
    break;
  case mm_map:
    // Re-use the memory of the grid functions; map if necessary
    mem_flags = CL_MEM_USE_HOST_PTR;
    break;
  default:
    assert(0);
  }
  bool const need_ptr =
      mem_flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR);

  int const NP =
      device->grid.ash[0] * device->grid.ash[1] * device->grid.ash[2];

  for (int var = 0; var < nvars; ++var) {
    int const vi = vis[var];
    int const rl = rls[var];
    assert(rl == 0); // TODO
    int const tl = tls[var];

    // Ensure that this variable does not yet exist
    assert(int(device->mems.at(vi).size()) <= tl);
    // Ensure that we don't have to create multiple variables
    assert(int(device->mems.at(vi).size()) == tl);

    // Allocate variable
    device->mems.at(vi).resize(tl + 1);

    void *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);
    assert(ptr);

    cl_int errcode;
    checkErr((device->mems.at(vi).at(tl).mem = clCreateBuffer(
                  device->context, mem_flags, NP * sizeof(CCTK_REAL),
                  need_ptr ? ptr : NULL, &errcode),
              errcode));
  }

  return 0;
}

extern "C" CCTK_INT OpenCLRunTime_CopyCycle(CCTK_POINTER_TO_CONST const cctkGH_,
                                            CCTK_INT const vis[],
                                            CCTK_INT const rls[],
                                            CCTK_INT const tls[],
                                            CCTK_INT const nvars) {
  cGH const *restrict const cctkGH CCTK_ATTRIBUTE_UNUSED =
      static_cast<cGH const *>(cctkGH_);
  DECLARE_CCTK_PARAMETERS;

  if (veryverbose) {
    CCTK_VInfo(CCTK_THORNSTRING, "CopyCycle: %s",
               vars_to_string(vis, rls, tls, nvars).c_str());
  }

  if (nvars == 0)
    return 0;

  assert(Carpet::is_local_mode());
  assert(device);
  assert(device->have_grid());

  if (device->mem_model == mm_always_mapped)
    return 0;
  assert(device->mem_model != mm_map);

  int const NP =
      device->grid.ash[0] * device->grid.ash[1] * device->grid.ash[2];

  cl_event old_event;
  for (int var = 0; var < nvars; ++var) {
    int const vi = vis[var];
    int const rl = rls[var];
    assert(rl == 0); // TODO
    int const tl = tls[var];
    assert(tl > 0);

    // Ensure we see variables in ascending and timelevels in
    // strictly descending order
    if (var > 0)
      assert(vi >= vis[var - 1]);
    if (var > 0 and vi == vis[var - 1])
      assert(tl < tls[var - 1]);

    // Track dependencies for each variable
    bool const have_old_event = var > 0 and vi == vis[var - 1];
    cl_event event;
    checkErr(clEnqueueCopyBuffer(
        device->queue, device->mems.at(vi).at(tl - 1).mem,
        device->mems.at(vi).at(tl).mem, 0, 0, NP * sizeof(CCTK_REAL),
        have_old_event ? 1 : 0, have_old_event ? &old_event : NULL, &event));
    old_event = event;
  }

  return 0;
}

extern "C" CCTK_INT
OpenCLRunTime_CopyFromPast(CCTK_POINTER_TO_CONST const cctkGH_,
                           CCTK_INT const vis[], CCTK_INT const rls[],
                           CCTK_INT const tls[], CCTK_INT const nvars) {
  cGH const *restrict const cctkGH CCTK_ATTRIBUTE_UNUSED =
      static_cast<cGH const *>(cctkGH_);
  DECLARE_CCTK_PARAMETERS;

  if (veryverbose) {
    CCTK_VInfo(CCTK_THORNSTRING, "CopyFromPast: %s",
               vars_to_string(vis, rls, tls, nvars).c_str());
  }

  if (nvars == 0)
    return 0;

  assert(Carpet::is_local_mode());
  assert(device);
  assert(device->have_grid());

  if (device->mem_model == mm_always_mapped)
    return 0;
  assert(device->mem_model != mm_map);

  int const NP =
      device->grid.ash[0] * device->grid.ash[1] * device->grid.ash[2];

  for (int var = 0; var < nvars; ++var) {
    int const vi = vis[var];
    assert(vi >= 0);
    int const rl = rls[var];
    assert(rl == 0); // TODO
    assert(rl >= 0);
    int const tl = tls[var];
    assert(tl >= 0);
    assert(tl + 1 < int(device->mems.at(vi).size()));

    checkErr(clEnqueueCopyBuffer(device->queue,
                                 device->mems.at(vi).at(tl + 1).mem,
                                 device->mems.at(vi).at(tl).mem, 0, 0,
                                 NP * sizeof(CCTK_REAL), 0, NULL, NULL));
  } // for var

  return 0;
}

extern "C" CCTK_INT
OpenCLRunTime_CopyToDevice(CCTK_POINTER_TO_CONST const cctkGH_,
                           CCTK_INT const vis[], CCTK_INT const rls[],
                           CCTK_INT const tls[], CCTK_INT const nvars,
                           CCTK_INT *const moved) {
  cGH const *restrict const cctkGH = static_cast<cGH const *>(cctkGH_);
  DECLARE_CCTK_PARAMETERS;

  if (veryverbose) {
    CCTK_VInfo(CCTK_THORNSTRING, "CopyToDevice: %s",
               vars_to_string(vis, rls, tls, nvars).c_str());
  }

  *moved = 0;

  if (nvars == 0)
    return 0;

  assert(Carpet::is_local_mode());
  assert(device);
  assert(device->have_grid());

  switch (device->mem_model) {

  case mm_always_mapped:
    // do nothing
    break;

  case mm_copy: {

    int const np =
        cctkGH->cctk_ash[0] * cctkGH->cctk_ash[1] * cctkGH->cctk_ash[2];

    int const di = sizeof(CCTK_REAL);
    int const dj = di * cctkGH->cctk_ash[0];
    int const dk = dj * cctkGH->cctk_ash[1];
    int const dI = sizeof(CCTK_REAL);
    int const dJ = dI * device->grid.ash[0];
    int const dK = dJ * device->grid.ash[1];

    size_t offset[dim];
    size_t length[dim];
    for (int d = 0; d < dim; ++d) {
      offset[d] = 0;
      length[d] = cctkGH->cctk_lsh[d];
    }
    offset[0] *= di;
    length[0] *= di;

    for (int var = 0; var < nvars; ++var) {
      int const vi = vis[var];
      assert(vi >= 0);
      int const rl = rls[var];
      assert(rl == 0); // TODO
      assert(rl >= 0);
      int const tl = tls[var];
      assert(tl >= 0);
      assert(tl < int(device->mems.at(vi).size()));

      void *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);

      if (HAVE_BUFFER_RECT_OPS and device->same_padding) {
        checkErr(clEnqueueWriteBuffer(
            device->queue, device->mems.at(vi).at(tl).mem, CL_FALSE, 0,
            np * sizeof(CCTK_REAL), ptr, 0, NULL, NULL));
      } else {
#if HAVE_BUFFER_RECT_OPS
        checkErr(clEnqueueWriteBufferRect(
            device->queue, device->mems.at(vi).at(tl).mem, CL_FALSE, offset,
            offset, length, dJ, dK, dj, dk, ptr, 0, NULL, NULL));
#else
        assert(0);
#endif
      }
    } // for var

    break;
  }

  case mm_map: {
    for (int var = 0; var < nvars; ++var) {
      int const vi = vis[var];
      assert(vi >= 0);
      int const tl = tls[var];
      assert(tl >= 0);
      assert(tl < int(device->mems.at(vi).size()));

      void *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);

      assert(device->same_padding);

      checkErr(clEnqueueUnmapMemObject(
          device->queue, device->mems.at(vi).at(tl).mem, ptr, 0, NULL, NULL));
    } // for var

    *moved = 1;

    break;
  }

  default:
    assert(0);
  }

  return 0;
}

extern "C" CCTK_INT
OpenCLRunTime_CopyToHost(CCTK_POINTER_TO_CONST const cctkGH_,
                         CCTK_INT const vis[], CCTK_INT const rls[],
                         CCTK_INT const tls[], CCTK_INT const nvars,
                         CCTK_INT *const moved) {
  cGH const *restrict const cctkGH = static_cast<cGH const *>(cctkGH_);
  DECLARE_CCTK_PARAMETERS;

  if (veryverbose) {
    CCTK_VInfo(CCTK_THORNSTRING, "CopyToHost: %s",
               vars_to_string(vis, rls, tls, nvars).c_str());
  }

  *moved = 0;

  if (nvars == 0)
    return 0;

  assert(Carpet::is_local_mode());
  assert(device);
  assert(device->have_grid());

  int const np =
      cctkGH->cctk_ash[0] * cctkGH->cctk_ash[1] * cctkGH->cctk_ash[2];

  cl_int errcode;

  switch (device->mem_model) {

  case mm_always_mapped:
    // do nothing
    break;

  case mm_copy: {

    int const di = sizeof(CCTK_REAL);
    int const dj = di * cctkGH->cctk_ash[0];
    int const dk = dj * cctkGH->cctk_ash[1];
    int const dI = sizeof(CCTK_REAL);
    int const dJ = dI * device->grid.ash[0];
    int const dK = dJ * device->grid.ash[1];

    size_t offset[dim];
    size_t length[dim];
    for (int d = 0; d < dim; ++d) {
      offset[d] = 0;
      length[d] = cctkGH->cctk_lsh[d];
    }
    offset[0] *= di;
    length[0] *= di;

    for (int var = 0; var < nvars; ++var) {
      int const vi = vis[var];
      assert(vi >= 0);
      int const rl = rls[var];
      assert(rl == 0); // TODO
      assert(rl >= 0);
      int const tl = tls[var];
      assert(tl >= 0);
      assert(tl < int(device->mems.at(vi).size()));

      void *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);

      if (HAVE_BUFFER_RECT_OPS and device->same_padding) {
        checkErr(clEnqueueReadBuffer(
            device->queue, device->mems.at(vi).at(tl).mem, CL_FALSE, 0,
            np * sizeof(CCTK_REAL), ptr, 0, NULL, NULL));
      } else {
#if HAVE_BUFFER_RECT_OPS
        checkErr(clEnqueueReadBufferRect(
            device->queue, device->mems.at(vi).at(tl).mem, CL_FALSE, offset,
            offset, length, dJ, dK, dj, dk, ptr, 0, NULL, NULL));
#else
        assert(0);
#endif
      }
    } // for var

    break;
  }

  case mm_map: {
    for (int var = 0; var < nvars; ++var) {
      int const vi = vis[var];
      assert(vi >= 0);
      int const tl = tls[var];
      assert(tl >= 0);
      assert(tl < int(device->mems.at(vi).size()));

      assert(device->same_padding);

      void *ptr;
      checkErr((ptr = clEnqueueMapBuffer(
                    device->queue, device->mems.at(vi).at(tl).mem, CL_FALSE,
                    CL_MAP_READ | CL_MAP_WRITE, 0, np * sizeof(CCTK_REAL), 0,
                    NULL, NULL, &errcode),
                errcode));

      assert(ptr == CCTK_VarDataPtrI(cctkGH, tl, vi));
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

extern "C" CCTK_INT
OpenCLRunTime_CopyPreSync(CCTK_POINTER_TO_CONST const cctkGH_,
                          CCTK_INT const vis[], CCTK_INT const rls[],
                          CCTK_INT const tls[], CCTK_INT const nvars) {
  cGH const *restrict const cctkGH = static_cast<cGH const *>(cctkGH_);
  DECLARE_CCTK_PARAMETERS;

  if (veryverbose) {
    CCTK_VInfo(CCTK_THORNSTRING, "CopyPreSync: %s",
               vars_to_string(vis, rls, tls, nvars).c_str());
  }

  if (nvars == 0)
    return 0;

  assert(Carpet::is_local_mode());
  assert(device);
  assert(device->have_grid());

  if (device->mem_model == mm_always_mapped)
    return 0;
  assert(device->mem_model != mm_map);

  int const dI = sizeof(CCTK_REAL);
  int const dJ = dI * device->grid.ash[0];
  int const dK = dJ * device->grid.ash[1];
  int const di = sizeof(CCTK_REAL);
  int const dj = di * cctkGH->cctk_ash[0];
  int const dk = dj * cctkGH->cctk_ash[1];

  for (int var = 0; var < nvars; ++var) {
    int const vi = vis[var];
    assert(vi >= 0);
    int const rl = rls[var];
    assert(rl == 0); // TODO
    assert(rl >= 0);
    int const tl = tls[var];
    assert(tl >= 0);
    assert(tl < int(device->mems.at(vi).size()));

    void *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);

#if HAVE_BUFFER_RECT_OPS

    for (int dir = 0; dir < dim; ++dir) {
      for (int face = 0; face < 2; ++face) {
        if (not cctkGH->cctk_bbox[2 * dir + face]) {

          int imin[dim];
          int imax[dim];
          for (int d = 0; d < dim; ++d) {
            // Whole domain
            imin[d] = 0;
            imax[d] = cctkGH->cctk_lsh[d];
            // Skip ghost points
            if (not cctkGH->cctk_bbox[2 * d + 0]) {
              imin[d] += cctkGH->cctk_nghostzones[d];
            }
            if (not cctkGH->cctk_bbox[2 * d + 1]) {
              imax[d] -= cctkGH->cctk_nghostzones[d];
            }
            // Skip points that will be copied via later directions
            if (d > dir) {
              if (not cctkGH->cctk_bbox[2 * d + 0]) {
                imin[d] += cctkGH->cctk_nghostzones[d];
              }
              if (not cctkGH->cctk_bbox[2 * d + 1]) {
                imax[d] -= cctkGH->cctk_nghostzones[d];
              }
            }
          }
          if (face == 0) {
            imin[dir] = cctkGH->cctk_nghostzones[dir];
            imax[dir] = imin[dir] + cctkGH->cctk_nghostzones[dir];
          } else {
            imax[dir] = cctkGH->cctk_lsh[dir] - cctkGH->cctk_nghostzones[dir];
            imin[dir] = imax[dir] - cctkGH->cctk_nghostzones[dir];
          }
          for (int d = 0; d < dim; ++d) {
            assert(imin[d] >= 0);
            assert(imin[d] <= imax[d]);
            assert(imax[d] <= cctkGH->cctk_lsh[d]);
          }

          size_t offset[dim];
          size_t length[dim];
          for (int d = 0; d < dim; ++d) {
            offset[d] = imin[d];
            length[d] = imax[d] - imin[d];
          }
          offset[0] *= di;
          length[0] *= di;

          checkErr(clEnqueueReadBufferRect(
              device->queue, device->mems.at(vi).at(tl).mem, CL_FALSE, offset,
              offset, length, dJ, dK, dj, dk, ptr, 0, NULL, NULL));

        } // if bbox
      }   // for face
    }     // for dir

#else

    bool have_sync_bnd = false;
    for (int dir = 0; dir < dim; ++dir) {
      for (int face = 0; face < 2; ++face) {
        if (not cctkGH->cctk_bbox[2 * dir + face]) {
          have_sync_bnd = true;
        }
      }
    }

    if (have_sync_bnd) {
      int const np =
          cctkGH->cctk_ash[0] * cctkGH->cctk_ash[1] * cctkGH->cctk_ash[2];

      checkErr(clEnqueueReadBuffer(device->queue,
                                   device->mems.at(vi).at(tl).mem, CL_FALSE, 0,
                                   np * sizeof(CCTK_REAL), ptr, 0, NULL, NULL));
    }

#endif

  } // for var

  // Finish, because we sync
  checkErr(clFinish(device->queue));

  return 0;
}

extern "C" CCTK_INT
OpenCLRunTime_CopyPostSync(CCTK_POINTER_TO_CONST const cctkGH_,
                           CCTK_INT const vis[], CCTK_INT const rls[],
                           CCTK_INT const tls[], CCTK_INT const nvars) {
  cGH const *restrict const cctkGH = static_cast<cGH const *>(cctkGH_);
  DECLARE_CCTK_PARAMETERS;

  if (veryverbose) {
    CCTK_VInfo(CCTK_THORNSTRING, "CopyPostSync: %s",
               vars_to_string(vis, rls, tls, nvars).c_str());
  }

  if (nvars == 0)
    return 0;

  assert(Carpet::is_local_mode());
  assert(device);
  assert(device->have_grid());

  if (device->mem_model == mm_always_mapped)
    return 0;
  assert(device->mem_model != mm_map);

  int const dI = sizeof(CCTK_REAL);
  int const dJ = dI * device->grid.ash[0];
  int const dK = dJ * device->grid.ash[1];
  int const di = sizeof(CCTK_REAL);
  int const dj = di * cctkGH->cctk_ash[0];
  int const dk = dj * cctkGH->cctk_ash[1];

  for (int var = 0; var < nvars; ++var) {
    int const vi = vis[var];
    assert(vi >= 0);
    int const rl = rls[var];
    assert(rl == 0); // TODO
    assert(rl >= 0);
    int const tl = tls[var];
    assert(tl >= 0);
    assert(tl < int(device->mems.at(vi).size()));

    void const *const ptr = CCTK_VarDataPtrI(cctkGH, tl, vi);

#if HAVE_BUFFER_RECT_OPS

    for (int dir = 0; dir < dim; ++dir) {
      for (int face = 0; face < 2; ++face) {
        if (not cctkGH->cctk_bbox[2 * dir + face]) {

          int imin[dim];
          int imax[dim];
          for (int d = 0; d < dim; ++d) {
            // Whole domain
            imin[d] = 0;
            imax[d] = cctkGH->cctk_lsh[d];
            // Skip points that will be copied via later directions
            if (d > dir) {
              if (not cctkGH->cctk_bbox[2 * d + 0]) {
                imin[d] += cctkGH->cctk_nghostzones[d];
              }
              if (not cctkGH->cctk_bbox[2 * d + 1]) {
                imax[d] -= cctkGH->cctk_nghostzones[d];
              }
            }
          }
          if (face == 0) {
            imin[dir] = 0;
            imax[dir] = imin[dir] + cctkGH->cctk_nghostzones[dir];
          } else {
            imax[dir] = cctkGH->cctk_lsh[dir];
            imin[dir] = imax[dir] - cctkGH->cctk_nghostzones[dir];
          }
          for (int d = 0; d < dim; ++d) {
            assert(imin[d] >= 0);
            assert(imin[d] <= imax[d]);
            assert(imax[d] <= cctkGH->cctk_lsh[d]);
          }

          size_t offset[dim];
          size_t length[dim];
          for (int d = 0; d < dim; ++d) {
            offset[d] = imin[d];
            length[d] = imax[d] - imin[d];
          }
          offset[0] *= di;
          length[0] *= di;

          checkErr(clEnqueueWriteBufferRect(
              device->queue, device->mems.at(vi).at(tl).mem, CL_FALSE, offset,
              offset, length, dJ, dK, dj, dk, ptr, 0, NULL, NULL));

        } // if bbox
      }   // if face
    }     // if dir

#else

    bool have_sync_bnd = false;
    for (int dir = 0; dir < dim; ++dir) {
      for (int face = 0; face < 2; ++face) {
        if (not cctkGH->cctk_bbox[2 * dir + face]) {
          have_sync_bnd = true;
        }
      }
    }

    if (have_sync_bnd) {
      int const np =
          cctkGH->cctk_ash[0] * cctkGH->cctk_ash[1] * cctkGH->cctk_ash[2];

      checkErr(clEnqueueWriteBuffer(
          device->queue, device->mems.at(vi).at(tl).mem, CL_FALSE, 0,
          np * sizeof(CCTK_REAL), ptr, 0, NULL, NULL));
    }

#endif

  } // for var

  return 0;
}

} // namespace OpenCLRunTime
