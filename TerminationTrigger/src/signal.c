#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"

#include "terminationtrigger.h"

/*************************************************************************
 ********************** Local function prototypes ************************
 ************************************************************************/
static void set_sighandler(const int which, const char *signame,
                           const int signum);
static void sighandler(int signum);
static void signal_name_callback(CCTK_ATTRIBUTE_UNUSED void *data,
                                 CCTK_ATTRIBUTE_UNUSED const char *thorn,
                                 const char *parameter,
                                 const char *new_value);
static void signal_number_callback(CCTK_ATTRIBUTE_UNUSED void *data,
                                   CCTK_ATTRIBUTE_UNUSED const char *thorn,
                                   const char *parameter,
                                   const char *new_value);

/*************************************************************************
 ********************** Local variable definitionsi **********************
 ************************************************************************/
#define MAX_NUM_SIGNALS 10 /* must match param.ccl */
/* set to the signal number by signal handler */
static int signal_caught = 0;
static int current_signals[MAX_NUM_SIGNALS] = {0};
static void (*old_handlers[MAX_NUM_SIGNALS])(int) = {0};

/*************************************************************************
 ********************** Scheduled function definitons ********************
 ************************************************************************/

int TerminationTrigger_StartSignalHandler(void) {
  DECLARE_CCTK_PARAMETERS;

  for(int i = 0 ; i < MAX_NUM_SIGNALS ; i++) {
    set_sighandler(i, signal_names[i], signal_numbers[i]);
  }

  /* actively listen to parameter changes so that the signal can be changed eg
   * via the http thorn and is active right away */
  int ierr;
  ierr =
    CCTK_ParameterSetNotifyRegister(signal_name_callback, NULL,
                                    CCTK_THORNSTRING "WATCH_SIGNAL_NAME_CHANGE",
                                    CCTK_THORNSTRING, "signal_names");
  if(ierr) {
    CCTK_VWarn(CCTK_WARN_PICKY, __LINE__, __FILE__, CCTK_THORNSTRING,
               "Could not register parameter change monitor for "
               "'signal_names': %d", ierr);
  }
  ierr =
    CCTK_ParameterSetNotifyRegister(signal_number_callback, NULL,
                                    CCTK_THORNSTRING
                                    "WATCH_SIGNAL_NUMBER_CHANGE",
                                    CCTK_THORNSTRING, "signal_numbers");
  if(ierr) {
    CCTK_VWarn(CCTK_WARN_PICKY, __LINE__, __FILE__, CCTK_THORNSTRING,
               "Could not register parameter change monitor for "
               "'signal_numbers': %d", ierr);
  }

  return 1;
}

void TerminationTrigger_CheckSignal(CCTK_ARGUMENTS) {
  DECLARE_CCTK_ARGUMENTS;
  DECLARE_CCTK_PARAMETERS;

  if (testsuite && current_signals[0] && cctk_iteration == 1) {
    raise(current_signals[0]);
  }

  if(signal_caught) {
    CCTK_VInfo(CCTK_THORNSTRING,
               "Received signal '%d'. Triggering termination...", signal_caught);
    TerminationTrigger_TriggerTermination(CCTK_PASS_CTOC);
  }

  /* reset signal handler in case the signal we are listening to changed */
  for(int i = 0 ; i < MAX_NUM_SIGNALS ; i++) {
    set_sighandler(i, signal_names[i], signal_numbers[i]);
  }
}

/*************************************************************************
 ********************** Local function definitons ************************
 ************************************************************************/

static void set_sighandler(const int which, const char *signame,
                           const int signum) {
  int my_signum = 0;

  assert(which >= 0 && which < MAX_NUM_SIGNALS);

  if(CCTK_EQUALS(signame, "SIGHUP")) {
    my_signum = SIGHUP;
    assert(my_signum > 0);
  } else if(CCTK_EQUALS(signame, "SIGINT")) {
    my_signum = SIGINT;
    assert(my_signum > 0);
  } else if(CCTK_EQUALS(signame, "SIGTERM")) {
    my_signum = SIGTERM;
    assert(my_signum > 0);
  } else if(CCTK_EQUALS(signame, "SIGUSR1")) {
    my_signum = SIGUSR1;
    assert(my_signum > 0);
  } else if(CCTK_EQUALS(signame, "SIGUSR2")) {
    my_signum = SIGUSR2;
    assert(my_signum > 0);
  } else if(CCTK_EQUALS(signame, "")) {
    my_signum = signum;
  } else {
    CCTK_VWarn(CCTK_WARN_PICKY, __LINE__, __FILE__, CCTK_THORNSTRING,
               "Internal error: unknown signal '%s', continuing without",
               signame);
  }
  assert(my_signum >= 0);

  if(my_signum != current_signals[which]) {
    if(my_signum > 0) {
      CCTK_VInfo(CCTK_THORNSTRING, "Listening for signal '%s'.", signame);
    } else {
      CCTK_VInfo(CCTK_THORNSTRING, "Stopped listening for signals.");
    }
  }

  if(old_handlers[which] != NULL) {
    assert(current_signals[which] > 0);
    signal(current_signals[which], old_handlers[which]);
  }

  if(my_signum > 0) {
    old_handlers[which] = signal(my_signum, sighandler);
    current_signals[which] = my_signum;
  }
}

static void sighandler(int signum) {
  signal_caught = signum;
  /* ignore further identical signals just in case a user kills us more than
   * once and we don't want to just abort */
  signal(signum, SIG_IGN);
}

static void signal_name_callback(CCTK_ATTRIBUTE_UNUSED void *data,
                                 CCTK_ATTRIBUTE_UNUSED const char *thorn,
                                 const char *parameter,
                                 const char *new_value) {
  int which;
  const int converted = sscanf("%*[a-zA-Z_0-9][%d]", parameter, &which);
  assert(converted == 1);
  set_sighandler(which, new_value, 0);
}

static void signal_number_callback(CCTK_ATTRIBUTE_UNUSED void *data,
                                   CCTK_ATTRIBUTE_UNUSED const char *thorn,
                                   const char *parameter,
                                   const char *new_value) {
  int which;
  const int converted = sscanf("%*[a-zA-Z_0-9][%d]", parameter, &which);
  assert(converted == 1);
  set_sighandler(which, "", atoi(new_value));
}
