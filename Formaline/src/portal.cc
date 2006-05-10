// $Header$

#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>

#include <unistd.h>

#include "cctk.h"
#include "cctk_Parameters.h"
#include "util_Network.h"

#include "portal.hh"



namespace Formaline
{

  using namespace std;
  
  

  static bool
  is_clean_for_shell (char const * str);
  
  
  
  portal::
  portal (char const * const id,
          enum state const st)
    : storage (st)
  {
    DECLARE_CCTK_PARAMETERS;
    
    msgbuf << "<?xml version='1.0' ?>"
           << "<methodCall><methodName>";
    switch (get_state())
    {
    case initial:
      msgbuf << "cactus.registerApplication";
      break;
    case update:
      msgbuf << "cactus.updateApplication";
      break;
    case final:
      msgbuf << "cactus.deregisterApplication";
      break;
    default:
      assert (0);
    }
    msgbuf << "</methodName>"
           << "<params><param><value><struct>"
           << "<member>"
           << "<name>jobid</name>"
           << "<value><string>" << clean (id) << "</string></value>"
           << "</member>";
  }



  portal::
  ~ portal ()
  {
    DECLARE_CCTK_PARAMETERS;
    
    string const socket_script = "socket-client.pl";
    string const socket_data = "socket-data";
    
    
    
    // Write the data
    msgbuf << "</struct></value></param></params>";
    msgbuf << "</methodCall>";
    string const msgstr = msgbuf.str();
    
    ostringstream databuf;
    databuf << "POST HTTP/1.0 200\r\n"
            << "Content-Type: text/xml\r\n"
            << "Content-Length: " << msgstr.length() << "\r\n"
            << "\r\n"
            << msgstr
            << "\r\n"
            << "\r\n";
    string const datastr = databuf.str();
    
    ostringstream datafilenamebuf;
    datafilenamebuf << out_dir << "/" << socket_data;
    string const datafilenamestr = datafilenamebuf.str();
    char const * const datafilename = datafilenamestr.c_str();
    
    ofstream datafile;
    datafile.open (datafilename, ios::out);
    datafile << datastr;
    datafile.close ();
    
    
    
    // Write the script
    ostringstream scriptbuf;
    scriptbuf
<< "#! /usr/bin/perl -w" << endl
<< endl
<< "use strict;" << endl
<< "use Socket;" << endl
<< endl
<< "my $input = '" << datafilename << "';" << endl
<< "my @hostlist = (";

    // NUM_PORTAL_ENTRIES must match the size of the
    // Formaline::portal_hostname and Formaline::portal_port parameter arrays
#define NUM_PORTAL_ENTRIES 5

    // add all array parameters which have been set
    for (int i = 0; i < NUM_PORTAL_ENTRIES; i++) {
      if (*portal_hostname[i]) {
        if (i) scriptbuf << "," << endl << "                ";
        scriptbuf << "'" << portal_hostname[i] << ":" << portal_port[i] << "'";
      }
    }
    scriptbuf
<< ");" << endl
<< endl
<< "foreach my $entry (@hostlist) {" << endl
<< "  next if ($entry !~ /^(.+):(\\d+)$/);" << endl
<< endl
<< "  my $host = $1;" << endl
<< "  my $port = $2;" << endl
<< endl
<< "  my $SH;" << endl
<< endl
<< "  # try to use IO::Socket::INET if the module exists;" << endl
<< "  # it accepts a timeout for its internal connect call" << endl
<< "  eval 'use IO::Socket::INET;" << endl
<< endl
<< "        $SH = IO::Socket::INET->new (PeerAddr => $host," << endl
<< "                                     PeerPort => $port," << endl
<< "                                     Proto    => \\'tcp\\'," << endl
<< "                                     Type     => SOCK_STREAM," << endl
<< "                                     Timeout  => 0.2);';" << endl
<< "  # if that failed, fall back to making the standard socket/connect calls" << endl
<< "  # (with their built-in fixed timeout)" << endl
<< "  if ($@) {" << endl
<< "    my $iaddr = inet_aton ($host);" << endl
<< "    next if (not $iaddr);" << endl
<< "" << endl
<< "    socket ($SH, PF_INET, SOCK_STREAM, getprotobyname ('tcp'));" << endl
<< "    my $sin = sockaddr_in ($port, $iaddr);" << endl
<< "    connect ($SH, $sin) || next;" << endl
<< "  }" << endl
<< endl
<< "  # send off the data" << endl
<< "  if (defined $SH) {" << endl
<< "    open (my $FH, '<' . $input);" << endl
<< "    print $SH $_ while (<$FH>);" << endl
<< "    close $FH;" << endl
<< "    close $SH;" << endl
<< "  }" << endl
<< "}" << endl
<< endl;
    string const scriptstr = scriptbuf.str();
    
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
        return;
      }
    }
    
    
    
    // Make the script executable
    ostringstream chmodbuf;
    chmodbuf << "chmod a+x " << scriptfilenamestr
             << " < /dev/null > /dev/null 2> /dev/null";
    string const chmodstr = chmodbuf.str();
    char const * const chmod = chmodstr.c_str();
    system (chmod);
    
    
    
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
          return;
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
          return;
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
          return;
        }
      }
    }
    else
    {
      cwd[0] = '\0';
    }
    
    
    
    // Send the data
    ostringstream cmdbuf;
    if (my_use_relay_host)
    {
      cmdbuf << "ssh " << my_relay_host << " '"
             << "cd " << cwd << " && ";
    }
    cmdbuf << scriptfilenamestr << " < /dev/null > /dev/null 2> /dev/null";
    if (my_use_relay_host)
    {
      cmdbuf << "'";
    }
    string const cmdstr = cmdbuf.str();
    char const * const cmd = cmdstr.c_str();
    
    int const ierr = system (cmd);
    if (ierr != 0)
    {
      static bool did_complain = false;
      if (! did_complain)
      {
        did_complain = true;
        CCTK_WARN (1, "Failed to send data to the portal");
      }
    }
    
    remove (datafilename);
    remove (scriptfilename);
  }



  void portal::
  store (char const * const key,
         bool const value)
  {
    assert (key);
  
    ostringstream keybuf;
    keybuf << key;
    ostringstream valuebuf;
    valuebuf << (value ? "true" : "false");
  
    msgbuf << "<member>"
           << "<name>" << clean (keybuf.str()) << "</name>"
           << "<value><boolean>" << clean (valuebuf.str()) << "</boolean></value>"
           << "</member>";
  }



  void portal::
  store (char const * const key,
         int const value)
  {
    assert (key);
  
    ostringstream keybuf;
    keybuf << key;
    ostringstream valuebuf;
    valuebuf << value;
  
    msgbuf << "<member>"
           << "<name>" << clean (keybuf.str()) << "</name>"
           << "<value><int>" << clean (valuebuf.str()) << "</int></value>"
           << "</member>";
  }



  void portal::
  store (char const * const key,
         double const value)
  {
    assert (key);
  
    ostringstream keybuf;
    keybuf << key;
    ostringstream valuebuf;
    valuebuf << setprecision(15) << value;
  
    msgbuf << "<member>"
           << "<name>" << clean (keybuf.str()) << "</name>"
           << "<value><double>" << clean (valuebuf.str()) << "</double></value>"
           << "</member>";
  }



  void portal::
  store (char const * const key,
         char const * const value)
  {
    assert (key);
  
    ostringstream keybuf;
    keybuf << key;
    ostringstream valuebuf;
    valuebuf << value;
  
    msgbuf << "<member>"
           << "<name>" << clean (keybuf.str()) << "</name>"
           << "<value><string>" << clean (valuebuf.str()) << "</string></value>"
           << "</member>";
  }



  string portal::
  clean (string const & txt)
    const
  {
    ostringstream buf;
  
    for (string::const_iterator p = txt.begin(); p != txt.end(); ++ p)
    {
      switch (* p)
      {
      case '<': buf << "&lt;"; break;
      case '&': buf << "&amp;"; break;
      default: buf << * p;
      }
    }
  
    return buf.str();
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
          // We don't like this character
          return false;
        }
      }
    }
    return true;
  }



} // namespace Formaline
