#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <list>
#include <sstream>
#include <string>

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cctk.h"
#include "cctk_Parameters.h"
#include "util_Network.h"

#include "senddata.hh"



namespace Formaline
{
  
  using namespace std;
  
  
  
  static bool
  is_clean_for_shell (char const * const str);
  
  
  
  int
  SendData (string const hostname, int const port, string const data)
  {
    DECLARE_CCTK_PARAMETERS;
    
    string const socket_script = "socket-client.pl";
    string const socket_data = "socket-data";
    
    
    
    // Write the data to a file
    ostringstream datafilenamebuf;
    datafilenamebuf << out_dir << "/" << socket_data;
    string const datafilenamestr = datafilenamebuf.str();
    char const * const datafilename = datafilenamestr.c_str();
    
    ofstream datafile;
    datafile.open (datafilename, ios::out);
    datafile << data;
    datafile.close ();
    
    
    
    // Create a script that sends the data
    ostringstream scriptbuf;
    scriptbuf
<< "#! /usr/bin/perl -w" << endl
<< endl
<< "use strict;" << endl
<< "use Socket;" << endl
<< "use POSIX;"  << endl
<< endl
<< "my $input = '" << datafilename << "';" << endl
<< "my $host = '" << hostname << "';" << endl
<< "my $port = " << port << ";" << endl
<< endl
<< "# Set a timeout for the entire interaction with this server" << endl
<< "eval {" << endl
//
//    Use POSIX::sigaction to bypass the Perl interpreter's signal
//    handling which uses defered signals effectively ignoring
//    user-defined timeouts for some I/O functions (see 'man
//    perlipc' and then grep for 'Interrupting IO')
//
<< "  POSIX::sigaction (SIGALRM, POSIX::SigAction->new (sub { die 'timeout' }))" << endl
<< "    or die \"Error setting SIGALRM handler: $!\";" << endl
<< "  alarm " << timeout << ";" << endl
<< endl
<< "  my $iaddr = inet_aton ($host);" << endl
<< "  die \"Couldn't get IP address for '$host'\" if (not $iaddr);" << endl
<< "  my $sin = sockaddr_in ($port, $iaddr);" << endl
<< "  socket (my $SH, PF_INET, SOCK_STREAM, getprotobyname ('tcp'));" << endl
<< "  die 'Couldn\\'t open TCP socket' if (not defined $SH);" << endl
<< endl
<< "  # Connect and send off the data" << endl
<< "  die \"Couldn't connect to '$host:$port'\" if ! connect ($SH, $sin);" << endl
<< endl
<< "  open (my $FH, '<' . $input);" << endl
<< "  print $SH $_ while (<$FH>);" << endl
<< "  close $FH;" << endl
<< "  close $SH;" << endl
<< "}" << endl;
    string const scriptstr = scriptbuf.str();
    
    // Write the script to a file
    ostringstream scriptfilenamebuf;
    scriptfilenamebuf << out_dir << "/" << socket_script;
    string const scriptfilenamestr = scriptfilenamebuf.str();
    char const * const scriptfilename = scriptfilenamestr.c_str();
    
    ofstream scriptfile;
    scriptfile.open (scriptfilename, ios::out);
    scriptfile << scriptstr;
    scriptfile.close ();
    
    
    
    // Check that the file name is sane
    if (! is_clean_for_shell (scriptfilename))
    {
      static bool did_complain = false;
      if (! did_complain)
      {
        did_complain = true;
        CCTK_WARN (1, "Strange character in file name -- not calling system()");
        return -1;
      }
    }
    
    
    
    // Make the script executable
    ostringstream chmodbuf;
    chmodbuf << "/bin/sh -c 'chmod a+x " << scriptfilenamestr
             << " < /dev/null > /dev/null 2> /dev/null'";
    string const chmodstr = chmodbuf.str();
    char const * const chmod = chmodstr.c_str();
    system (chmod);
    
    
    
    // Determine the relay host, if any
    bool my_use_relay_host = use_relay_host;
    char const * my_relay_host = 0;
    if (my_use_relay_host)
    {
      my_relay_host = relay_host;
      if (strcmp (my_relay_host, "") == 0)
      {
        // Determine a good relay host
        char run_host [1000];
        Util_GetHostName (run_host, sizeof run_host);
        if (strncmp (run_host, "ic", 2) == 0 && strlen (run_host) == 6)
        {
          // Peyote or Lagavulin
          int const node = atoi (run_host + 2);
          if (node < 192)
          {
            // Peyote
            my_relay_host = "peyote";
          }
          else
          {
            // Lagavulin
            my_relay_host = "lagavulin";
          }
        }
        else if (strncmp (run_host, "mike", 4) == 0 && strlen (run_host) == 7)
        {
          // Supermike
          my_use_relay_host = false;
        }
        else
        {
          // Don't know a good relay host; try without
          my_use_relay_host = false;
        }
        
        if (verbose)
        {
          if (my_use_relay_host)
          {
            CCTK_VInfo (CCTK_THORNSTRING,
                        "Using \"%s\" as relay host", my_relay_host);
          }
          else
          {
            CCTK_INFO ("Announcing without relay host");
          }
        }
      }
    }
    
    if (my_use_relay_host)
    {
      // Check that the relay host name is sane
      if (! is_clean_for_shell (my_relay_host))
      {
        static bool did_complain = false;
        if (! did_complain)
        {
          did_complain = true;
          CCTK_WARN (1, "Strange character in relay host name -- not calling system()");
          return -2;
        }
      }
    }
    
    
    
    char cwd[10000];
    if (my_use_relay_host)
    {
      // Get the current directory
      char * const cwderr = getcwd (cwd, sizeof cwd);
      if (cwderr == NULL) {
        static bool did_complain = false;
        if (! did_complain)
        {
          did_complain = true;
          CCTK_WARN (1, "Cannot determine current working directory");
          return -3;
        }
      }

      // Check that the current directory name is sane
      if (! is_clean_for_shell (cwd))
      {
        static bool did_complain = false;
        if (! did_complain)
        {
          did_complain = true;
          CCTK_WARN (1, "Strange character in current directory -- not calling system()");
          return -4;
        }
      }
    }
    else
    {
      strcpy (cwd, "");
    }
    
    
    
    // Call the script the data
    ostringstream cmdbuf;
    if (my_use_relay_host)
    {
      cmdbuf << "ssh -x " << my_relay_host << " \"/bin/sh -c '"
             << "cd " << cwd << " && ";
    } else {
      cmdbuf << "/bin/sh -c '";
    }
    cmdbuf << scriptfilenamestr << " < /dev/null > /dev/null 2> /dev/null'";
    if (my_use_relay_host)
    {
      cmdbuf << "\"";
    }
    string const cmdstr = cmdbuf.str();
    char const * const cmd = cmdstr.c_str();
    
    int const ierr = system (cmd);
    if (ierr != 0)
    {
      // system(3) blocks SIGINT which otherwise would interrupt the
      // simulation.  Make sure that this is still so if a user types
      // CTRL-C.
      if (WIFSIGNALED (ierr) and WTERMSIG (ierr) == SIGINT) {
        raise (SIGINT);
      }
      
      static bool did_complain = false;
      if (! did_complain)
      {
        did_complain = true;
        CCTK_WARN (1, "Failed to send data");
        return -5;
      }
    }
    
    
    
    // Do not remove the files; leave them around for debugging
#if 0
    remove (datafilename);
    remove (scriptfilename);
#endif
    
    return 0;                   // Success
  }
  
  
  
  static bool
  is_clean_for_shell (char const * const str)
  {
    for (char const * p = str; * p; ++ p)
    {
      if (! isalnum (* p))
      {
        // Allow only certain characters
        switch (* p)
        {
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
