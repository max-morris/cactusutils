#include <assert.h>
#include <signal.h>

#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"

/*************************************************************************
 ********************** Local function prototypes ************************
 ************************************************************************/
static void set_sighandler(const char *signame);
static void sighandler(int signum);
static void signal_name_callback(CCTK_ATTRIBUTE_UNUSED void *data,
                                 CCTK_ATTRIBUTE_UNUSED const char *thorn,
                                 CCTK_ATTRIBUTE_UNUSED const char *parameter,
                                 const char *new_value);

/*************************************************************************
 ********************** Local variable definitionsi **********************
 ************************************************************************/
static int signal_caught = 0;   /* set to 1 by signal handler */
static int current_signal = -1;
static void (*old_handler)(int) = NULL;

/*************************************************************************
 ********************** Scheduled function definitons ********************
 ************************************************************************/

int TerminationTrigger_StartSignalHandler(void) {
  DECLARE_CCTK_PARAMETERS;

  set_sighandler(signal_name);

  /* actively listen to parameter changes so that the signal can be changed eg
   * via the http thorn and is active right away */
  CCTK_ParameterSetNotifyRegister(signal_name_callback, NULL,
                                  CCTK_THORNSTRING "WATCH_SIGNAL_NAME_CHANGE",
                                  CCTK_THORNSTRING, "signal_name");

  return 1;
}

void TerminationTrigger_CheckSignal(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  if(signal_caught) {
    CCTK_VInfo(CCTK_THORNSTRING,
               "Received signal '%s'. Triggering termination...", signal_name);
    CCTK_TerminateNext(cctkGH);
  }

  /* reset signal handler in case the signal we are listening too changed */
  set_sighandler(signal_name);
}

/*************************************************************************
 ********************** Local function definitons ************************
 ************************************************************************/

static void set_sighandler(const char *signame) {
  int signum = -1;

  if(CCTK_EQUALS(signame, "SIGHUP")) {
    signum = SIGHUP;
  } else if(CCTK_EQUALS(signame, "SIGINT")) {
    signum = SIGINT;
  } else if(CCTK_EQUALS(signame, "SIGTERM")) {
    signum = SIGTERM;
  } else if(CCTK_EQUALS(signame, "SIGUSR1")) {
    signum = SIGUSR1;
  } else if(CCTK_EQUALS(signame, "SIGUSR2")) {
    signum = SIGUSR2;
  } else if(CCTK_EQUALS(signame, "")) {
    signum = -1;
  } else {
    CCTK_VWarn(CCTK_WARN_PICKY, __LINE__, __FILE__, CCTK_THORNSTRING,
               "Internal error: unknown signal '%s', continuing without",
               signame);
  }

  if(signum != current_signal) {
    if(signum > 0) {
      CCTK_VInfo(CCTK_THORNSTRING, "Listening for signal '%s'.", signame);
    } else {
      CCTK_VInfo(CCTK_THORNSTRING, "Stopped listening for signals.");
    }
  }

  if(old_handler != NULL) {
    assert(current_signal >= 0);
    signal(current_signal, old_handler);
  }

  if(signum >= 0) {
    old_handler = signal(signum, sighandler);
    current_signal = signum;
  }
}

static void sighandler(int signum) {
  signal_caught = 1;
  /* ignore further identical signals just in case a user kills us more than
   * once and we don't want to just abort */
  signal(signum, SIG_IGN);
}

static void signal_name_callback(CCTK_ATTRIBUTE_UNUSED void *data,
                                 CCTK_ATTRIBUTE_UNUSED const char *thorn,
                                 CCTK_ATTRIBUTE_UNUSED const char *parameter,
                                 const char *new_value) {
  set_sighandler(new_value);
}
