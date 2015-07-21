#include <cctk.h>

#ifndef CCTK_MPI

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// IRIX wants this before <time.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#elif HAVE_TIME_H
#include <time.h>
#endif
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "nompi.h"

int MPI_Abort(MPI_Comm const comm, int const errorcode) { exit(errorcode); }

int MPI_Allgather(void *const sendbuf, int const sendcnt,
                  MPI_Datatype const sendtype, void *const recvbuf,
                  int const recvcnt, MPI_Datatype const recvtype,
                  MPI_Comm const comm) {
  assert(sendbuf);
  assert(sendcnt >= 0);
  assert(recvbuf);
  assert(recvcnt >= 0);
  assert(recvcnt == sendcnt);
  assert(recvtype == sendtype);
  MPI_Aint const recvsize = recvtype;
  assert(recvsize > 0);
  memcpy(recvbuf, sendbuf, recvcnt * recvsize);
  return 0;
}

int MPI_Allgatherv(void *const sendbuf, int const sendcnt,
                   MPI_Datatype const sendtype, void *const recvbuf,
                   int *const recvcnts, int *const recvoffs,
                   MPI_Datatype const recvtype, MPI_Comm const comm) {
  assert(sendbuf);
  assert(sendcnt >= 0);
  assert(recvbuf);
  assert(recvcnts);
  assert(recvcnts[0] == sendcnt);
  assert(recvoffs[0] == 0);
  assert(recvtype == sendtype);
  MPI_Aint const recvsize = recvtype;
  assert(recvsize > 0);
  memcpy(recvbuf, sendbuf, recvcnts[0] * recvsize);
  return 0;
}

int MPI_Allreduce(void *const sendbuf, void *const recvbuf, int const cnt,
                  MPI_Datatype const datatype, MPI_Op const op,
                  MPI_Comm const comm) {
  assert(sendbuf);
  assert(recvbuf);
  assert(cnt >= 0);
  MPI_Aint const recvsize = datatype;
  assert(recvsize > 0);
  memcpy(recvbuf, sendbuf, cnt * recvsize);
  return 0;
}

int MPI_Alltoall(void *const sendbuf, int const sendcnt,
                 MPI_Datatype const sendtype, void *const recvbuf,
                 int const recvcnt, MPI_Datatype const recvtype,
                 MPI_Comm const comm) {
  assert(sendbuf);
  assert(sendcnt >= 0);
  assert(recvbuf);
  assert(recvcnt >= 0);
  assert(recvcnt == sendcnt);
  assert(recvtype == sendtype);
  MPI_Aint const recvsize = recvtype;
  assert(recvsize > 0);
  memcpy(recvbuf, sendbuf, recvcnt * recvsize);
  return 0;
}

int MPI_Alltoallv(void *const sendbuf, int *const sendcnts, int *const sendoffs,
                  MPI_Datatype const sendtype, void *const recvbuf,
                  int *const recvcnts, int *const recvoffs,
                  MPI_Datatype const recvtype, MPI_Comm const comm) {
  assert(sendbuf);
  assert(sendcnts);
  assert(sendoffs);
  assert(sendoffs[0] == 0);
  assert(recvbuf);
  assert(recvcnts);
  assert(recvcnts[0] == sendcnts[0]);
  assert(recvoffs);
  assert(recvoffs[0] == 0);
  assert(recvtype == sendtype);
  MPI_Aint const recvsize = recvtype;
  assert(recvsize > 0);
  memcpy(recvbuf, sendbuf, recvcnts[0] * recvsize);
  return 0;
}

int MPI_Barrier(MPI_Comm const comm) { return 0; }

int MPI_Bcast(void *const buf, int const cnt, MPI_Datatype const datatype,
              int const root, MPI_Comm const comm) {
  assert(buf);
  assert(cnt >= 0);
  assert(root == 0);
  return 0;
}

int MPI_Comm_rank(MPI_Comm const comm, int *const rank) {
  assert(rank);
  *rank = 0;
  return 0;
}

int MPI_Comm_size(MPI_Comm const comm, int *const size) {
  assert(size);
  *size = 1;
  return 0;
}

int MPI_Comm_split(MPI_Comm const comm, int const color, int const key,
                   MPI_Comm *const newcomm) {
  assert(newcomm);
  return 0;
}

int MPI_Dims_create(int const nnodes, int const ndims, int *const dims) {
  assert(dims);
  int npoints = 1;
  for (int d = 0; d < ndims; ++d) {
    assert(dims[d] >= 0);
    if (dims[d] > 0)
      npoints *= dims[d];
  }
  assert(npoints % nnodes == 0);
  assert(nnodes == 1); /* Assume there is one node per MPI process */
  for (int d = 0; d < ndims; ++d) {
    if (dims[d] == 0)
      dims[d] = 1;
  }
  return 0;
}

int MPI_Finalize(void) { return 0; }

int MPI_Gather(void *const sendbuf, int const sendcnt,
               MPI_Datatype const sendtype, void *const recvbuf,
               int const recvcnt, MPI_Datatype const recvtype, int const root,
               MPI_Comm const comm) {
  assert(sendbuf);
  assert(sendcnt >= 0);
  assert(recvbuf);
  assert(recvcnt >= 0);
  assert(recvtype == sendtype);
  assert(root == 0);
  MPI_Aint const recvsize = recvtype;
  assert(recvsize > 0);
  memcpy(recvbuf, sendbuf, recvcnt * recvsize);
  return 0;
}

int MPI_Gatherv(void *const sendbuf, int const sendcnt,
                MPI_Datatype const sendtype, void *const recvbuf,
                int *const recvcnts, int *const recvoffs,
                MPI_Datatype const recvtype, int const root,
                MPI_Comm const comm) {
  assert(sendbuf);
  assert(sendcnt >= 0);
  assert(recvbuf);
  assert(recvcnts);
  assert(recvcnts[0] >= 0);
  assert(recvcnts[0] == sendcnt);
  assert(recvoffs);
  assert(recvoffs[0] == 0);
  assert(recvtype == sendtype);
  assert(root == 0);
  MPI_Aint const recvsize = recvtype;
  assert(recvsize > 0);
  memcpy(recvbuf, sendbuf, recvcnts[0] * recvsize);
  return 0;
}

int MPI_Init(int *const argc, char ***const argv) { return 0; }

int MPI_Irecv(void *const buf, int const cnt, MPI_Datatype const datatype,
              int const source, int const tag, MPI_Comm const comm,
              MPI_Request *const request) {
  /* this should not be called */
  assert(0);
  return -1;
}

int MPI_Isend(void *const buf, int const count, MPI_Datatype const datatype,
              int const dest, int const tag, MPI_Comm const comm,
              MPI_Request *const request) {
  /* this should not be called */
  assert(0);
  return -1;
}

int MPI_Pcontrol(int const level, ...) {
  /* do nothing */
  return 0;
}

int MPI_Reduce(void *const sendbuf, void *const recvbuf, int const cnt,
               MPI_Datatype const datatype, MPI_Op const op, int const root,
               MPI_Comm const comm) {
  assert(sendbuf);
  assert(recvbuf);
  assert(cnt >= 0);
  assert(root == 0);
  int const datasize = datatype;
  assert(datasize > 0);
  memcpy(recvbuf, sendbuf, cnt * datasize);
  return 0;
}

int MPI_Send(void *const buf, int const count, MPI_Datatype const datatype,
             int const dest, int const tag, MPI_Comm const comm) {
  /* this should not be called */
  assert(0);
  return -1;
}

int MPI_Ssend(void *const buf, int const count, MPI_Datatype const datatype,
              int const dest, int const tag, MPI_Comm const comm) {
  /* this should not be called */
  assert(0);
  return -1;
}

int MPI_Type_commit(MPI_Datatype *const datatype) {
  /* do nothing */
  return 0;
}

int MPI_Type_contiguous(int const cnt, MPI_Datatype const oldtype,
                        MPI_Datatype *const newtype) {
  assert(oldtype > 0);
  *newtype = cnt *oldtype;
  return 0;
}

int MPI_Type_free(MPI_Datatype *const datatype) {
  /* do nothing */
  return 0;
}

int MPI_Type_lb(MPI_Datatype const datatype, MPI_Aint *const displacement) {
  assert(displacement);
  *displacement = 0;
  return 0;
}

int MPI_Type_size(MPI_Datatype const datatype, int *const size) {
  assert(datatype > 0);
  assert(size);
  *size = datatype;
  return 0;
}

int MPI_Type_struct(int const cnt, int *const array_of_blocklengths,
                    MPI_Aint *const array_of_displacements,
                    MPI_Datatype *const array_of_types,
                    MPI_Datatype *const newtype) {
  assert(cnt >= 0);
  assert(array_of_blocklengths);
  assert(array_of_displacements);
  assert(array_of_types);
  assert(newtype);
  MPI_Aint newsize = 0;
  for (int n = 0; n < cnt; ++n) {
    assert(array_of_displacements[n] >= 0);
    assert(array_of_blocklengths[n] >= 0);
    MPI_Aint size;
    if (array_of_types[n] == MPI_UB) {
      size = array_of_displacements[n];
    } else {
      assert(array_of_types[n] > 0);
      size = array_of_displacements[n] +
             array_of_blocklengths[n] * array_of_types[n];
    }
    if (size > newsize)
      newsize = size;
  }
  *newtype = newsize;
  return 0;
}

int MPI_Type_ub(MPI_Datatype const datatype, MPI_Aint *const displacement) {
  assert(datatype > 0);
  assert(displacement);
  *displacement = datatype;
  return 0;
}

int MPI_Type_vector(int const cnt, int const blocklength, int const stride,
                    MPI_Datatype const oldtype, MPI_Datatype *const newtype) {
  assert(cnt >= 0);
  assert(blocklength >= 0);
  assert(stride > 0);
  assert(oldtype > 0);
  if (cnt == 0) {
    *newtype = 0;
  } else {
    *newtype = (cnt * blocklength + (cnt - 1) * stride) * oldtype;
  }
  return 0;
}

int MPI_Waitall(int const cnt, MPI_Request *const array_of_requests,
                MPI_Status *const array_of_statuses) {
  assert(cnt >= 0);
  assert(array_of_requests);
  return 0;
}

double MPI_Wtime(void) {
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return tp.tv_sec + tp.tv_usec / 1.0e+6;
}

#endif /* #ifndef CCTK_MPI */
