#ifndef NOMPI_H
#define NOMPI_H

/* Provide (dummy) replacements for many MPI routines in case MPI is
   not available */

#include <cctk.h>

#ifndef CCTK_MPI

#include <stdlib.h>

typedef ptrdiff_t MPI_Aint;

typedef int MPI_Comm; /* no content */

typedef MPI_Aint MPI_Datatype;

typedef enum {
  MPI_LOR,
  MPI_MAX,
  MPI_MIN,
  MPI_PROD,
  MPI_SUM,
} MPI_Op;

typedef int MPI_Request; /* no content */

typedef int MPI_Status; /* no contend */

MPI_Comm const MPI_COMM_NULL = -1;
MPI_Comm const MPI_COMM_WORLD = 0;

MPI_Datatype const MPI_DATATYPE_NULL = 0;
MPI_Datatype const MPI_CHAR = sizeof(char);
MPI_Datatype const MPI_SHORT = sizeof(short);
MPI_Datatype const MPI_INT = sizeof(int);
MPI_Datatype const MPI_LONG = sizeof(long);
MPI_Datatype const MPI_LONG_LONG_INT = sizeof(long long int);
MPI_Datatype const MPI_UNSIGNED_CHAR = sizeof(unsigned char);
MPI_Datatype const MPI_UNSIGNED_SHORT = sizeof(unsigned short);
MPI_Datatype const MPI_UNSIGNED = sizeof(unsigned);
MPI_Datatype const MPI_UNSIGNED_LONG = sizeof(unsigned long);
MPI_Datatype const MPI_FLOAT = sizeof(float);
MPI_Datatype const MPI_DOUBLE = sizeof(double);
MPI_Datatype const MPI_LONG_DOUBLE = sizeof(long double);
MPI_Datatype const MPI_UB = -1;

MPI_Request const MPI_REQUEST_NULL = -1;

MPI_Status *const MPI_STATUSES_IGNORE = NULL;

#ifdef __cplusplus
extern "C" {
#endif

int MPI_Abort(MPI_Comm const comm, int const errorcode);

int MPI_Allgather(void *const sendbuf, int const sendcnt,
                  MPI_Datatype const sendtype, void *const recvbuf,
                  int const recvcnt, MPI_Datatype const recvtype,
                  MPI_Comm const comm);

int MPI_Allgatherv(void *const sendbuf, int const sendcnt,
                   MPI_Datatype const sendtype, void *const recvbuf,
                   int *const recvcnts, int *const recvoffs,
                   MPI_Datatype const recvtype, MPI_Comm const comm);

int MPI_Allreduce(void *const sendbuf, void *const recvbuf, int const cnt,
                  MPI_Datatype const datatype, MPI_Op const op,
                  MPI_Comm const comm);

int MPI_Alltoall(void *const sendbuf, int const sendcnt,
                 MPI_Datatype const sendtype, void *const recvbuf,
                 int const recvcnt, MPI_Datatype const recvtype,
                 MPI_Comm const comm);

int MPI_Alltoallv(void *const sendbuf, int *const sendcnts, int *const sendoffs,
                  MPI_Datatype const sendtype, void *const recvbuf,
                  int *const recvcnts, int *const recvoffs,
                  MPI_Datatype const recvtype, MPI_Comm const comm);

int MPI_Barrier(MPI_Comm const comm);

int MPI_Bcast(void *const buf, int const cnt, MPI_Datatype const datatype,
              int const root, MPI_Comm const comm);

int MPI_Comm_rank(MPI_Comm const comm, int *const rank);

int MPI_Comm_size(MPI_Comm const comm, int *const size);

int MPI_Comm_split(MPI_Comm const comm, int const color, int const key,
                   MPI_Comm *const newcomm);

int MPI_Dims_create(int const nnodes, int const ndims, int *const dims);

int MPI_Finalize(void);

int MPI_Gather(void *const sendbuf, int const sendcnt,
               MPI_Datatype const sendtype, void *const recvbuf,
               int const recvcnt, MPI_Datatype const recvtype, int const root,
               MPI_Comm const comm);

int MPI_Gatherv(void *const sendbuf, int const sendcnt,
                MPI_Datatype const sendtype, void *const recvbuf,
                int *const recvcnts, int *const recvoffs,
                MPI_Datatype const recvtype, int const root,
                MPI_Comm const comm);

int MPI_Init(int *const argc, char ***const argv);

int MPI_Irecv(void *const buf, int const cnt, MPI_Datatype const datatype,
              int const source, int const tag, MPI_Comm const comm,
              MPI_Request *const request);

int MPI_Isend(void *const buf, int const count, MPI_Datatype const datatype,
              int const dest, int const tag, MPI_Comm const comm,
              MPI_Request *const request);

int MPI_Pcontrol(int const level, ...);

int MPI_Reduce(void *const sendbuf, void *const recvbuf, int const cnt,
               MPI_Datatype const datatype, MPI_Op const op, int const root,
               MPI_Comm const comm);

int MPI_Send(void *const buf, int const count, MPI_Datatype const datatype,
             int const dest, int const tag, MPI_Comm const comm);

int MPI_Ssend(void *const buf, int const count, MPI_Datatype const datatype,
              int const dest, int const tag, MPI_Comm const comm);

int MPI_Type_commit(MPI_Datatype *const datatype);

int MPI_Type_contiguous(int const cnt, MPI_Datatype const oldtype,
                        MPI_Datatype *const newtype);

int MPI_Type_free(MPI_Datatype *const datatype);

int MPI_Type_lb(MPI_Datatype const datatype, MPI_Aint *const displacement);

int MPI_Type_size(MPI_Datatype const datatype, int *const size);

int MPI_Type_struct(int const cnt, int *const array_of_blocklengths,
                    MPI_Aint *const array_of_displacements,
                    MPI_Datatype *const array_of_types,
                    MPI_Datatype *const newtype);

int MPI_Type_ub(MPI_Datatype const datatype, MPI_Aint *const displacement);

int MPI_Type_vector(int const cnt, int const blocklength, int const stride,
                    MPI_Datatype const oldtype, MPI_Datatype *const newtype);

int MPI_Waitall(int const cnt, MPI_Request *const array_of_requests,
                MPI_Status *const array_of_statuses);

double MPI_Wtime(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef CCTK_MPI */

#endif /* #ifndef NOMPI_H */
