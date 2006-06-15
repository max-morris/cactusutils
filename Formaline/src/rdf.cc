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
#include <list>
#include <string>
#include <sstream>

#include <unistd.h>

#include "cctk.h"
#include "cctk_Parameters.h"
#include "util_Network.h"

#include "rdf.hh"



namespace Formaline
{

  using namespace std;
  
  

  static bool
  is_clean_for_shell (char const * str);
  
  static list<string>
  parse (string const str);
  
  
  
  rdf::
  rdf (char const * const id,
       enum state const st)
    : storage (st)
  {
    msgbuf
<< "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << endl
<< "<!DOCTYPE owl [" << endl
<< "  <!ENTITY dc 'http://purl.org/dc/elements/1.1/'>" << endl
<< "  <!ENTITY doap 'http://usefulinc.com/ns/doap#'>" << endl
<< "  <!ENTITY foaf 'http://xmlns.com/foaf/0.1/'>" << endl
<< "  <!ENTITY rdf 'http://www.w3.org/1999/02/22-rdf-syntax-ns#'>" << endl
<< "  <!ENTITY rdfs 'http://www.w3.org/2000/01/rdf-schema#'>" << endl
<< "" << endl
<< "  <!ENTITY cctk 'http://www.cct.lsu.edu/~dstark/cctk/0.1/'>" << endl
<< "  <!ENTITY form 'http://www.aei.mpg.de/form#'>" << endl
<< "]>" << endl
<< "<rdf:RDF" << endl
<< "    xmlns:dc=\"&dc;\"" << endl
<< "    xmlns:doap=\"&doap;\"" << endl
<< "    xmlns:foaf=\"&foaf;\"" << endl
<< "    xmlns:rdf=\"&rdf;\"" << endl
<< "    xmlns:rdfs=\"&rdfs;\"" << endl
<< "    xmlns:cctk=\"&cctk;\"" << endl
<< "    xmlns:form=\"&form;\"" << endl
<< ">" << endl
<< endl
<< "<form:Simulation>" << endl
<< "  <form:jobid>" << clean (string (id)) << "</form:jobid>" << endl;
  }



  rdf::
  ~ rdf ()
  {
    DECLARE_CCTK_PARAMETERS;
    
    string const socket_script = "socket-client.pl";
    string const socket_data = "socket-data";
    
    
    
    // Write the data
    msgbuf
<< "</form:Simulation>" << endl
<< endl
<< "</rdf:RDF>" << endl;
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

    // NUM_RDF_ENTRIES must match the size of the
    // Formaline::rdf_hostname and Formaline::rdf_port parameter arrays
#define NUM_RDF_ENTRIES 5

    // add all array parameters which have been set
    for (int i = 0; i < NUM_RDF_ENTRIES; i++) {
      if (*rdf_hostname[i]) {
        if (i) scriptbuf << "," << endl << "                ";
        scriptbuf << "'" << rdf_hostname[i] << ":" << rdf_port[i] << "'";
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
        CCTK_WARN (1, "Failed to send data to the rdf");
      }
    }
    
    remove (datafilename);
    remove (scriptfilename);
  }



  void rdf::
  store (char const * const key,
         bool const value)
  {
    assert (key);
  
    ostringstream keybuf;
    keybuf << key;
    ostringstream valuebuf;
    valuebuf << (value ? "true" : "false");
    
    list<string> const keys = parse (keybuf.str());
    
    msgbuf << "  ";
    for (list<string>::const_iterator lsi = keys.begin();
         lsi != keys.end(); ++ lsi)
    {
      msgbuf << "<form:" << * lsi << ">";
    }
    msgbuf << clean (valuebuf.str());
    for (list<string>::const_reverse_iterator lsi = keys.rbegin();
         lsi != keys.rend(); ++ lsi)
    {
      msgbuf << "</form:" << * lsi << ">";
    }
    msgbuf << endl;
  }



  void rdf::
  store (char const * const key,
         CCTK_INT const value)
  {
    assert (key);
  
    ostringstream keybuf;
    keybuf << key;
    ostringstream valuebuf;
    valuebuf << value;
    
    list<string> const keys = parse (keybuf.str());
    
    msgbuf << "  ";
    for (list<string>::const_iterator lsi = keys.begin();
         lsi != keys.end(); ++ lsi)
    {
      msgbuf << "<form:" << * lsi << ">";
    }
    msgbuf << clean (valuebuf.str());
    for (list<string>::const_reverse_iterator lsi = keys.rbegin();
         lsi != keys.rend(); ++ lsi)
    {
      msgbuf << "</form:" << * lsi << ">";
    }
    msgbuf << endl;
  }



  void rdf::
  store (char const * const key,
         CCTK_REAL const value)
  {
    assert (key);
    
#if defined CCTK_REAL_PRECISION_4
    int const prec = 6;
#elif defined CCTK_REAL_PRECISION_8
    int const prec = 15;
#elif defined CCTK_REAL_PRECISION_16
    int const prec = 30;
#else
    int const prec = 15;
#endif
  
    ostringstream keybuf;
    keybuf << key;
    ostringstream valuebuf;
    valuebuf << setprecision(prec) << value;
    
    list<string> const keys = parse (keybuf.str());
    
    msgbuf << "  ";
    for (list<string>::const_iterator lsi = keys.begin();
         lsi != keys.end(); ++ lsi)
    {
      msgbuf << "<form:" << * lsi << ">";
    }
    msgbuf << clean (valuebuf.str());
    for (list<string>::const_reverse_iterator lsi = keys.rbegin();
         lsi != keys.rend(); ++ lsi)
    {
      msgbuf << "</form:" << * lsi << ">";
    }
    msgbuf << endl;
  }



  void rdf::
  store (char const * const key,
         char const * const value)
  {
    assert (key);
  
    ostringstream keybuf;
    keybuf << key;
    ostringstream valuebuf;
    valuebuf << value;
    
    list<string> const keys = parse (keybuf.str());
    
    msgbuf << "  ";
    for (list<string>::const_iterator lsi = keys.begin();
         lsi != keys.end(); ++ lsi)
    {
      msgbuf << "<form:" << * lsi << ">";
    }
    msgbuf << clean (valuebuf.str());
    for (list<string>::const_reverse_iterator lsi = keys.rbegin();
         lsi != keys.rend(); ++ lsi)
    {
      msgbuf << "</form:" << * lsi << ">";
    }
    msgbuf << endl;
  }



  string rdf::
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
  
  
  
  static list<string>
  parse (string const str)
  {
    list<string> strs;
    size_t p = 0;
    for (;;) {
      size_t const s = str.find ("/", p);
      if (s == string::npos) break;
      strs.push_back (str.substr (p, s - p));
      p = s + 1;
    }
    strs.push_back (str.substr (p));
    return strs;
  }
  
  
  
} // namespace Formaline
