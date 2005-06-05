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

#include "cctk.h"
#include "cctk_Parameters.h"

#include "portal.hh"



namespace Formaline
{

  using namespace std;



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
<< "my $host = '" << portal_hostname << "';" << endl
<< "my $port = '" << portal_port << "';" << endl
<< endl
<< "open (my $FH, '<' . $input);" << endl
<< endl
<< "socket (my $SH, PF_INET, SOCK_STREAM, getprotobyname ('tcp'));" << endl
<< "my $sin = sockaddr_in ($port, inet_aton ($host));" << endl
<< "connect ($SH, $sin) || exit -1;" << endl
<< endl
<< "while (my $line = <$FH>)" << endl
<< "{" << endl
<< "  print $SH $line;" << endl
<< "}" << endl
<< endl
<< "close $SH;" << endl;
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
    for (char const * p = scriptfilename; * p; ++ p)
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
          {
            static bool did_complain = false;
            if (! did_complain)
            {
              did_complain = true;
              CCTK_WARN (1, "Strange character in file name -- not calling system()");
              return;
            }
          }
        }
      }
    }
    
    
    
    // Make the script executable
    ostringstream chmodbuf;
    chmodbuf << "chmod a+x " << scriptfilenamestr
             << " < /dev/null > /dev/null 2> /dev/null";
    string const chmodstr = chmodbuf.str();
    char const * const chmod = chmodstr.c_str();
    system (chmod);
    
    
    
    // Send the data
    ostringstream cmdbuf;
    cmdbuf << scriptfilenamestr << " < /dev/null > /dev/null 2> /dev/null";
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



} // namespace Formaline
