#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <list>
#include <set>
#include <sstream>
#include <string>

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cctk.h"
#include "cctk_Parameters.h"
#include "util_Network.h"

#include "senddata.hh"

namespace Formaline {

using namespace std;

static bool is_clean_for_shell(char const *const str);

int SendData(string const hostname, int const port, string const data) {
  DECLARE_CCTK_PARAMETERS;

  if (verbose) {
    CCTK_VInfo(CCTK_THORNSTRING, "Announcing to %s:%d", hostname.c_str(), port);
  }

#if 0
    // pair<,> is not a standard STL class
    typedef pair <string, int> destination_t;
    destination_t const destination (hostname, port);
#endif
  typedef string destination_t;
  ostringstream dest_buffer;
  dest_buffer << hostname << ":" << port;
  destination_t const destination = dest_buffer.str();

  string const socket_script = "socket-client.pl";
  string const socket_data = "socket-data";

  // Write the data to a file
  ostringstream datafilenamebuf;
  datafilenamebuf << out_dir << "/" << socket_data;
  string const datafilenamestr = datafilenamebuf.str();
  char const *const datafilename = datafilenamestr.c_str();

  ofstream datafile;
  datafile.open(datafilename, ios::out);
  datafile << data;
  datafile.close();

  // Create a script that sends the data
  ostringstream scriptbuf;
  scriptbuf
      << "#! /usr/bin/perl -w\n"
      << "\n"
      << "use strict;\n"
      << "use Socket;\n"
      << "use POSIX;\n"
      << "\n"
      << "my $verbose = " << verbose << ";\n"
      << "my $input = '" << datafilename << "';\n"
      << "my $host = '" << hostname << "';\n"
      << "my $port = " << port << ";\n"
      << "\n"
      << "# Set a timeout for the entire interaction with this server\n"
      //
      // Use POSIX::sigaction to bypass the Perl interpreter's signal
      // handling which uses deferred signals, effectively ignoring
      // user-defined timeouts for some I/O functions (see 'man perlipc' and
      // then look for 'Interrupting IO')
      //
      // Using { die 'timeout' } or { exit 1 } is not sufficient to exit
      // reliably after a timeout.  Using { POSIX::_exit 1 } seems to work.
      // The difference is that the POSIX function does not clean up, but
      // that should be fine.
      //
      << "POSIX::sigaction (SIGALRM, POSIX::SigAction->new (sub { POSIX::_exit "
         "1; }))\n"
      << "    or die \"Error setting SIGALRM handler: $!\";\n"
      << "alarm " << timeout << ";\n"
      << "\n"
      << "if ($verbose) { print STDERR \"Getting IP address\\n\"; }\n"
      << "my $iaddr = inet_aton ($host);\n"
      << "$iaddr or die \"Couldn't get IP address for '$host'\";\n"
      << "if ($verbose) { print STDERR \"Creating sockaddr_in\\n\"; }\n"
      << "my $sin = sockaddr_in ($port, $iaddr);\n"
      << "if ($verbose) { print STDERR \"Opening socket\\n\"; }\n"
      << "socket (my $SH, PF_INET, SOCK_STREAM, getprotobyname ('tcp'));\n"
      << "defined $SH or die \"Couldn't open TCP socket\";\n"
      << "\n"
      << "# Connect and send off the data\n"
      << "if ($verbose) { print STDERR \"Connecting\\n\"; }\n"
      << "connect ($SH, $sin) or die \"Couldn't connect to '$host:$port'\";\n"
      << "\n"
      << "if ($verbose) { print STDERR \"Opening local data file\\n\"; }\n"
      << "open (my $FH, \"< $input\");\n"
      << "if ($verbose) { print STDERR \"Sending data\\n\"; }\n"
      << "send ($SH, $_, 0) while (<$FH>);\n"
      << "if ($verbose) { print STDERR \"Closing local data file\\n\"; }\n"
      << "close $FH;\n"
      << "if ($verbose) { print STDERR \"Receiving acknowledgement\\n\"; }\n"
      << "recv ($SH, $_, 1, 0);\n"
      << "if ($verbose) { print STDERR \"Shutting down connection\\n\"; }\n"
      << "close $SH;\n"
      << "if ($verbose) { print STDERR \"Done.\\n\"; }\n";
  string const scriptstr = scriptbuf.str();

  // Write the script to a file
  ostringstream scriptfilenamebuf;
  scriptfilenamebuf << out_dir << "/" << socket_script;
  string const scriptfilenamestr = scriptfilenamebuf.str();
  char const *const scriptfilename = scriptfilenamestr.c_str();

  ofstream scriptfile;
  scriptfile.open(scriptfilename, ios::out);
  scriptfile << scriptstr;
  scriptfile.close();

  // Check that the file name is sane
  if (!is_clean_for_shell(scriptfilename)) {
    static bool did_complain = false;
    if (!did_complain) {
      did_complain = true;
      CCTK_WARN(1, "Strange character in file name -- not calling system()");
      return -1;
    }
  }

  // Make the script executable
  ostringstream chmodbuf;
  chmodbuf << "/bin/sh -c 'chmod a+x " << scriptfilenamestr << " < /dev/null'";
  string const chmodstr = chmodbuf.str();
  char const *const chmod = chmodstr.c_str();
  system(chmod);

  // Determine the relay host, if any
  bool my_use_relay_host = use_relay_host;
  char const *my_relay_host = 0;
  if (my_use_relay_host) {
    my_relay_host = relay_host;
    if (strcmp(my_relay_host, "") == 0) {
      // Determine a good relay host
      char run_host[1000];
      Util_GetHostName(run_host, sizeof run_host);
      CCTK_VInfo(CCTK_THORNSTRING, "Local host name is %s", run_host);
      if (strncmp(run_host, "ic", 2) == 0 && strlen(run_host) == 6) {
        // Peyote (AEI)
        my_relay_host = "peyote";
      } else if (strncmp(run_host, "mike", 4) == 0 && strlen(run_host) == 7) {
        // Supermike (LSU)
        my_use_relay_host = false;
      } else if (strlen(run_host) == 21 && strncmp(run_host, "node", 4) == 0 &&
                 strncmp(run_host + 7, ".damiana.admin", 13) == 0) {
        // Damiana (AEI)
        my_relay_host = "damiana";
      } else if (strlen(run_host) == 14 && strncmp(run_host, "node", 4) == 0 &&
                 strncmp(run_host + 8, ".admin", 6) == 0) {
        // Belladonna (AEI)
        my_relay_host = "belladonna";
      } else if (strncmp(run_host, "i", 1) == 0 &&
                 strncmp(run_host + 8, ".ranger", 7) == 0) {
        // Ranger (TACC)
        my_relay_host = "login3";
      } else {
        // Don't know a good relay host; try without
        my_use_relay_host = false;
      }

      if (verbose) {
        if (my_use_relay_host) {
          CCTK_VInfo(CCTK_THORNSTRING,
                     "Using %s as relay host to announce to %s:%d",
                     my_relay_host, hostname.c_str(), port);
        } else {
          CCTK_VInfo(CCTK_THORNSTRING, "Announcing to %s:%d without relay host",
                     hostname.c_str(), port);
        }
      }
    }
  }

  if (my_use_relay_host) {
    // Check that the relay host name is sane
    if (!is_clean_for_shell(my_relay_host)) {
      static set<destination_t> did_complain;
      if (did_complain.count(destination) == 0) {
        did_complain.insert(destination);
        CCTK_VWarn(CCTK_WARN_ALERT, __LINE__, __FILE__, CCTK_THORNSTRING,
                   "Strange character in relay host name \"%s\" -- not calling "
                   "system()",
                   hostname.c_str());
        return -2;
      }
    }
  }

  char cwd[10000];
  if (my_use_relay_host) {
    // Get the current directory
    char *const cwderr = getcwd(cwd, sizeof cwd);
    if (cwderr == NULL) {
      static bool did_complain = false;
      if (!did_complain) {
        did_complain = true;
        CCTK_WARN(1, "Cannot determine current working directory");
        return -3;
      }
    }

    // Check that the current directory name is sane
    if (!is_clean_for_shell(cwd)) {
      static bool did_complain = false;
      if (!did_complain) {
        did_complain = true;
        CCTK_WARN(
            1,
            "Strange character in current directory -- not calling system()");
        return -4;
      }
    }
  } else {
    // cwd is not used below
    strcpy(cwd, "");
  }

  // Call the script the data
  ostringstream cmdbuf;
  if (my_use_relay_host) {
    cmdbuf << "env DISPLAY= ssh -x " << my_relay_host << " \"/bin/sh -c '"
           << "cd " << cwd << " && " << scriptfilenamestr << " < /dev/null"
           << "'\"";
  } else {
    cmdbuf << "/bin/sh -c '" << scriptfilenamestr << " < /dev/null'";
  }
  string const cmdstr = cmdbuf.str();
  char const *const cmd = cmdstr.c_str();

  int const ierr = system(cmd);
  if (ierr != 0) {
    // system(3) blocks SIGINT which otherwise would interrupt the
    // simulation.  Make sure that this is still so if a user types
    // CTRL-C.
    if (WIFSIGNALED(ierr) and WTERMSIG(ierr) == SIGINT) {
      raise(SIGINT);
    }

    static set<destination_t> did_complain;
    if (did_complain.count(destination) == 0) {
      did_complain.insert(destination);
      CCTK_VWarn(CCTK_WARN_ALERT, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "Failed to send data to %s:%d", hostname.c_str(), port);
      return -5;
    }
  }

  // Do not remove the files; leave them around for debugging
  // remove (datafilename);
  // remove (scriptfilename);

  return 0; // Success
}

static bool is_clean_for_shell(char const *const str) {
  for (char const *p = str; *p; ++p) {
    if (!isalnum(*p)) {
      // Allow only certain characters
      switch (*p) {
      case '+':
      case ',':
      case '-':
      case '.':
      case '/':
      case ':':
      case '_':
      case '~':
        break;
      default:
        // We don't any other character
        return false;
      }
    }
  }
  return true;
}

} // namespace Formaline
