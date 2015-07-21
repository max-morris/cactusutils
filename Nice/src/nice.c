#include "cctk.h"
#include "cctk_WarnLevel.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"

/* manpage of getpriority:
   Including  <sys/time.h>  is  not  required  these days,
   but increases portability.
*/
#include <sys/time.h>
#include <errno.h>
#include <sys/resource.h>

int Nice_Renice(void);
int Nice_Renice(void) {
  DECLARE_CCTK_PARAMETERS

  int old_nice, new_nice, nice_return_value;

  /* reset error variable */
  errno = 0;
  /* save the old nice value */
  old_nice = getpriority(PRIO_PROCESS, 0);
  /* check errno */
  if (errno) {
    switch (errno) {
    case ESRCH: /* this should never occur, since the 0 in getpriority
                   means this process, which should be found
                */
      CCTK_INFO("getpriority received ESTCH-error.");
      break;
    case EINVAL: /* this should definitly not occur */
      CCTK_INFO("getpriority received EINVAL-error.");
      break;
    case EPERM: /* this should never occur
                   It means, that the user of process did not match the
                   callers user */
      CCTK_INFO("getpriority received EPERM-error.");
      break;
    case EACCES: /* This cannot occur: non root user attemted to lower
                    priority (since we did not call setpriority here */
      CCTK_INFO("getpriority received EACCES-error.");
      break;
    default: /* This should not occur, otherwise it is an
                undocumented errno */
      CCTK_VInfo(CCTK_THORNSTRING, "getpriority got undocumented errno %d",
                 errno);
    }
    CCTK_WARN(0, "This should never occur.");
  }

  errno = 0;
  /* try to set the new priority */
  nice_return_value = setpriority(PRIO_PROCESS, 0, Nice_nice);
  /* check return value */
  if (nice_return_value != 0) {
    switch (errno) {
    case ESRCH: /* this should never occur, since the 0 in setpriority
                   means this process, which should be found
                */
      CCTK_INFO("setpriority received ESTCH-error.");
      CCTK_WARN(0, "This should never occur.");
      break;
    case EINVAL: /* this should definitly not occur */
      CCTK_INFO("setpriority received EINVAL-error.");
      CCTK_WARN(0, "This should never occur.");
      break;
    case EPERM: /* this should never occur
                   It means, that the user of process did not match the
                   callers user */
      CCTK_INFO("setpriority received EPERM-error.");
      CCTK_WARN(0, "This should never occur.");
      break;
    case EACCES: /* This can occur: non root user attemted to lower
                    priority
                 */
      CCTK_WARN(0, "Only root can lower priorities.");
      break;
    default: /* This should not occur, otherwise it is an
                undocumented errno */
      CCTK_VInfo(CCTK_THORNSTRING, "setpriority got undocumented errno %d",
                 errno);
    }
    CCTK_WARN(0, "This should never occur.");
  }

  errno = 0;
  /* It should ok, but check the new priority */
  new_nice = getpriority(PRIO_PROCESS, 0);
  /* check errno */
  if (errno) {
    switch (errno) {
    case ESRCH: /* this should never occur, since the 0 in getpriority
                   means this process, which should be found */
      CCTK_INFO("getpriority received ESTCH-error.");
      break;
    case EINVAL: /* this should definitly not occur */
      CCTK_INFO("getpriority received EINVAL-error.");
      break;
    case EPERM: /* this should never occur
                   It means, that the user of process did not match the
                   callers user */
      CCTK_INFO("getpriority received EPERM-error.");
      break;
    case EACCES: /* This cannot occur: non root user attemted to lower
                    priority (since we did not call setpriority here
                 */
      CCTK_INFO("getpriority received EACCES-error.");
      break;
    default: /* This should not occur, otherwise it is an
                undocumented errno */
      CCTK_VInfo(CCTK_THORNSTRING, "getpriority got undocumented errno %d",
                 errno);
    }
    CCTK_WARN(0, "This should never occur.");
  }

  if (new_nice == Nice_nice) {
    CCTK_VInfo(CCTK_THORNSTRING, "Process reniced from nice level %d to %d.",
               old_nice, new_nice);
  } else {
    /* this should never occur because of the error handling before */
    CCTK_VInfo(CCTK_THORNSTRING,
               "Process reniced to nice level %d instead of %d.", new_nice,
               (int)Nice_nice);
  }

  return 0;
}
