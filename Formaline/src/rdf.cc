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
#include <limits>
#include <list>
#include <string>
#include <sstream>

#include <unistd.h>

#include "cctk.h"
#include "cctk_Parameters.h"
#include "cctk_Version.h"
#include "util_String.h"
#include "util_Network.h"

#include "rdf.hh"


// number of space chars for indentation
#define NUM_INDENT_SPACES 2

namespace Formaline
{

  using namespace std;


  static bool
  is_clean_for_shell (char const * str);

#if 0
  static list<string>
  parse (char const * const key, string& node);
#endif


  rdf::
  rdf (char const * const id,
       enum state const st,
       CCTK_ARGUMENTS)
    : storage (st)
  {
    //
    // RDF/XML document header with some namespace definitions
    //
    msgbuf <<
"<?xml version=\"1.0\" encoding=\"utf-8\"?>
<!DOCTYPE owl [
\t<!ENTITY rdf  'http://www.w3.org/1999/02/22-rdf-syntax-ns#'>
\t<!ENTITY xsd  'http://www.w3.org/2001/XMLSchema#'>
\t<!ENTITY cctk 'http://www.cct.lsu.edu/~dstark/cctk/0.1/'>
<!--
\t<!ENTITY dc   'http://purl.org/dc/elements/1.1/'>
\t<!ENTITY doap 'http://usefulinc.com/ns/doap#'>
\t<!ENTITY foaf 'http://xmlns.com/foaf/0.1/'>
\t<!ENTITY rdfs 'http://www.w3.org/2000/01/rdf-schema#'>
\t<!ENTITY form 'http://www.aei.mpg.de/form#'>
-->
]>
<rdf:RDF xmlns:rdf=\"&rdf;\"
\txmlns:xsd=\"&xsd;\"
\txmlns:cctk=\"&cctk;\"
>
<!--
\txmlns:dc=\"&dc;\"
\txmlns:doap=\"&doap;\"
\txmlns:foaf=\"&foaf;\"
\txmlns:rdfs=\"&rdfs;\"
\txmlns:form=\"&form;\"
-->

<!-- lessons learned so far
  
     * although it would be better to use rdf:ID as relative URIs for objects
       (because they are enforced to be unique), rdf:about must be used instead
       because the value of a relative rdf:about URI may contain special
       characters (such as '/') whereas rdf:ID must not
-->
" << endl << endl;

    // set the unique ID for this simulation
    jobID = clean (string (id));

    //
    // document contents
    //
    switch (get_state()) {
      case initial: Initial (); break;
      case update: 
      case final:   Update (cctkGH); break;
      default:      assert (0 && "invalid state");
    }

    //
    // close the RDF/XML document
    //
    msgbuf << endl << "</rdf:RDF>" << endl;
  }

  void rdf::Initial (void)
  {
    //
    // This code was copied over from function Formaline_AnnounceInitial()
    // in announce.cc and modified here to create a valid RDF/XML (rather then
    // an unstructured plain XML document).
    //
    DECLARE_CCTK_PARAMETERS;

    if (verbose) CCTK_INFO ("Announcing initial RDF metadata information");

    //
    // general metadata with inlined attribute values
    //
    char hostbuf[512] = "";
    Util_GetHostName (hostbuf, sizeof (hostbuf));
    const string host = clean (hostbuf);
    const cGH* const cctkGH = NULL;
    const int nprocs = CCTK_nProcs (cctkGH);
#if 0
    const string user = clean (CCTK_RunUser());
#else
    const string user = clean (getenv ("USER"));
#endif
    char** argv;
    CCTK_CommandLine (&argv);
    const string executable = clean (argv[0]);
    char parfilebuf[512] = "";
    CCTK_ParameterFilename (sizeof (parfilebuf), parfilebuf);
    const string parfile = clean (parfilebuf);
    const string version = clean (CCTK_FullVersion ());
    ostringstream compiled_at_buf;
    compiled_at_buf << CCTK_CompileDate () << " " << CCTK_CompileTime ();
    const string compiled_at (clean (compiled_at_buf.str()));
    char rundatebuf[32] = "";
    char runtimebuf[32] = "";
    Util_CurrentDate (sizeof (rundatebuf), rundatebuf);
    Util_CurrentTime (sizeof (runtimebuf), runtimebuf);
    ostringstream started_at_buf;
    started_at_buf << rundatebuf << " " << runtimebuf;
    const string started_at (clean (started_at_buf.str()));
    char cwdbuf[512];
    getcwd (cwdbuf, sizeof (cwdbuf));
    const string cwd (clean (cwdbuf));


    msgbuf
<< "<cctk:Simulation rdf:about=\"#" << jobID << "\">" << endl
<< "\t<cctk:simulationID>"  << jobID       << "</cctk:simulationID>" << endl
<< "\t<cctk:host>"          << host        << "</cctk:host>" << endl
<< "\t<cctk:nProcs>"        << nprocs      << "</cctk:nProcs>" << endl
<< "\t<cctk:user>"          << user        << "</cctk:user>" << endl
<< "\t<cctk:executable>"    << executable  << "</cctk:executable>" << endl
<< "\t<cctk:parameterFile>" << parfile     << "</cctk:parameterFile>" << endl
<< "\t<cctk:hasVersion>"    << version     << "</cctk:hasVersion>" << endl
<< "\t<cctk:compiledAt>"    << compiled_at << "</cctk:compiledAt>" << endl
<< "\t<cctk:startedAt>"     << started_at  << "</cctk:startedAt>" << endl
<< "\t<cctk:cwd>"           << cwd         << "</cctk:cwd>" << endl
<< "</cctk:Simulation>" << endl << endl;


    //
    // metadata as references to other nodes
    //
    msgbuf << "<cctk:Simulation rdf:about=\"#" << jobID << "\">" << endl
           << "\t<cctk:hasThornList rdf:resource=\"#ThornList\"/>" << endl
           << "\t<cctk:hasParameterFile rdf:resource=\"#ParameterFile\"/>" << endl
           << "</cctk:Simulation>" << endl << endl;

    // store thorn list
    msgbuf << "<cctk:ThornList rdf:about=\"#ThornList\">" << endl;
    const int numthorns = CCTK_NumCompiledThorns ();
    for (int thorn = 0; thorn < numthorns; ++ thorn) {
      const char* const thornname = CCTK_CompiledThorn (thorn);

      msgbuf << "\t<cctk:containsThorn rdf:resource=\"#Thorns/"
             << thornname << "\"/>" << endl;
    }
    msgbuf << "</cctk:ThornList>" << endl << endl;

    // store parameter file contents
    msgbuf << "<cctk:ParameterFile rdf:about=\"#ParameterFile\">" << endl
           << "\t<rdf:value>";
    ifstream file (parfile.c_str());
    char c;
    ostringstream filebuf;
    while (filebuf and file.get (c)) filebuf.put (c);
    file.close();
    msgbuf << clean (filebuf.str());
    filebuf.clear();
    msgbuf << "</rdf:value>" << endl
           << "</cctk:ParameterFile>" << endl << endl;

    // store all parameters which have been set in the parfile
    msgbuf
<< "<!-- ============================================================ -->" << endl
<< "<!-- thorn graphs with their parameters                           -->" << endl
<< "<!-- ============================================================ -->" << endl;
    ostringstream parambuf;
    parambuf
<< "<!-- ============================================================ -->" << endl
<< "<!-- list of parameters and their values                          -->" << endl
<< "<!-- ============================================================ -->" << endl;

    const bool list_all_parameters = CCTK_Equals (out_save_parameters, "all");

    for (int thorn = 0; thorn < numthorns; ++ thorn) {
      const char* const thornname = CCTK_CompiledThorn (thorn);

      msgbuf << "<cctk:Thorn rdf:about=\"#Thorns/"
             << thornname << "\">" << endl;
      msgbuf << "\t<cctk:hasName>" << thornname << "</cctk:hasName>" << endl;

      // skip parameters that belong to inactive thorns
      const bool is_active = CCTK_IsThornActive (thornname);
      msgbuf << "\t<cctk:isActive>" << (is_active ? "true" : "false")
             << "</cctk:isActive>" << endl;

      // loop over all parameters of this thorn (if it is active)
      if (is_active) {
        for (int first = 1; ; first = 0) {
          char* fullname = NULL;
          const cParamData* pdata = NULL;

          // get the first/next parameter
          const int ierr = CCTK_ParameterWalk (first, thornname,&fullname,&pdata);
          assert (ierr >= 0);
          if (ierr > 0) break;

          msgbuf << "\t<cctk:hasParameter rdf:resource=\"#Parameters/"
                 << pdata->thorn << "/" << pdata->name << "\"/>" << endl;

          // get its value
          const void* const pvalue
            = CCTK_ParameterGet (pdata->name, pdata->thorn, NULL);
          assert (pvalue);

          if (pdata->n_set or list_all_parameters) {
            const char* paramtype;
            ostringstream paramvaluebuf;

            switch (pdata->type) {
              case PARAMETER_BOOLEAN:
              {
                paramtype = "BooleanParameter";
                const CCTK_INT v = *static_cast<const CCTK_INT*> (pvalue);
                paramvaluebuf << (v ? "true" : "false");
              }
              break;

              case PARAMETER_INT:
              {
                paramtype = "IntegerParameter";
                const CCTK_INT v = *static_cast<const CCTK_INT*> (pvalue);
                paramvaluebuf << v;
              }
              break;

              case PARAMETER_REAL:
              {
                paramtype = "RealParameter";
                CCTK_REAL const v = *static_cast<const CCTK_REAL*> (pvalue);
                paramvaluebuf << v;
              }
              break;

              case PARAMETER_KEYWORD:
              {
                paramtype = "KeywordParameter";
                const char* const v = *static_cast<const char* const*> (pvalue);
                paramvaluebuf << clean (v);
              }
              break;

              case PARAMETER_STRING:
              {
                paramtype = "StringParameter";
                const char* const v = *static_cast<const char* const*> (pvalue);
                paramvaluebuf << clean (v);
              }
              break;

              default: assert (0 and "invalid parameter type");

            } // switch (pdata->type)

            parambuf << "<cctk:" << paramtype << " rdf:about=\"#Parameters/"
                     << pdata->thorn << "/" << pdata->name << "\">" << endl
                     << "\t<cctk:hasName>" << fullname
                     << "</cctk:hasName>" << endl
                     << "\t<cctk:hasValue>" << paramvaluebuf.str()
                     << "</cctk:hasValue>" << endl
                     << "</cctk:" << paramtype << ">" << endl;

          } // if (pdata->n_set or list_all_parameters)

          free (fullname);

        } // loop over all parameters of this thorn
      } // if (is_active)

      msgbuf << "</cctk:Thorn>" << endl;

    } // loop over all thorns

    // FIXME: is there some better method for concatenation ??
    msgbuf << endl << parambuf.str();
  }


  void rdf::Update (CCTK_ARGUMENTS)
  {
    DECLARE_CCTK_ARGUMENTS;
    DECLARE_CCTK_PARAMETERS;

    if (verbose) {
      if (get_state() == update) {
        CCTK_INFO ("Announcing RDF metadata information update");
      } else {
        CCTK_INFO ("Announcing final RDF metadata information");
      }
    }

    char currentdatebuf[32] = "";
    char currenttimebuf[32] = "";
    Util_CurrentDate (sizeof (currentdatebuf), currentdatebuf);
    Util_CurrentTime (sizeof (currenttimebuf), currenttimebuf);
    ostringstream currentdate;
    currentdate << currentdatebuf << " " << currenttimebuf;

    // update counter which is incremented for each Update() call
    static int update_counter = 0;
    update_counter++;

    msgbuf
<< "<cctk:Simulation rdf:about=\"#" << jobID << "\">" << endl
<< "\t<cctk:updatedInfo rdf:resource=\"#UpdateInfo/" << update_counter << "\"/>" << endl
<< "</cctk:Simulation>" << endl << endl
<< "<cctk:UpdateInfo rdf:about=\"#UpdateInfo/" << update_counter << "\">" << endl
<< "\t<cctk:iteration>" << cctk_iteration << "</cctk:iteration>" << endl
<< "\t<cctk:time>" << cctk_time << "</cctk:time>" << endl
<< "\t<cctk:datetime>" << clean (currentdate.str()) << "</cctk:datetime>" << endl;

    if (get_state() == final) {
      msgbuf << "\t<cctk:terminated>yes</cctk:terminated>" << endl;
    }

    msgbuf << "</cctk:UpdateInfo>" << endl;
  }


  rdf::
  ~ rdf ()
  {
    DECLARE_CCTK_PARAMETERS;

    string const socket_script = "socket-client.pl";
    string const socket_data = "socket-data";



    // Write the data
    string const msgstr = msgbuf.str();

    ostringstream databuf;
    databuf << msgstr;
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
<< "  my $host = $1;" << endl
<< "  my $port = $2;" << endl
<< endl
<< "  my $iaddr = inet_aton ($host);" << endl
<< "  next if (not $iaddr);" << endl
<< "  my $sin = sockaddr_in ($port, $iaddr);" << endl
<< "  socket (my $SH, PF_INET, SOCK_STREAM, getprotobyname ('tcp'));" << endl
<< "  next if (not defined $SH);" << endl
<< endl
<< "  # set a timeout for the entire interaction with this server" << endl
<< "  eval {" << endl
<< "    local $SIG{ALRM} = sub { die 'timeout' };" << endl
<< "    alarm " << timeout << ";" << endl
<< endl
<< "    # connect and send off the data" << endl
<< "    if (connect ($SH, $sin)) {" << endl
// currently the RDF server only understands HTTP PUT
// << "POST HTTP/1.0 200\r\n"
<< "      my $header = \"PUT /context/CactusSimulations/" << jobID << " HTTP/1.0\\r\\n\" ." << endl
<< "                   \"Host: $entry\\r\\n\" ." << endl
<< "                   \"Content-Type: application/rdf+xml\\r\\n\" ." << endl
<< "                   \"Content-Length: " << msgstr.length() << "\\r\\n\" ." << endl
<< "                   \"\\r\\n\";" << endl
<< "      my $footer = \"\\r\\n\\r\\n\";" << endl
<< endl
<< "      open (my $FH, '<' . $input);" << endl
<< "      print $SH $header;" << endl
<< "      print $SH $_ while (<$FH>);" << endl
<< "      print $SH $footer;" << endl
<< "      # print $_ while (<$SH>);" << endl
<< "      close $FH;" << endl
<< "    }" << endl
<< "  };" << endl
<< "  close $SH;" << endl
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
#if 0
    ostringstream valuebuf;
    valuebuf << (value ? "true" : "false");

    string node;
    list<string> const keys = parse (key, node);
    string indent_string (NUM_INDENT_SPACES, ' ');
    for (list<string>::const_iterator lsi = keys.begin();
         lsi != keys.end(); ++ lsi)
    {
      msgbuf << indent_string << "<form:" << * lsi << ">" << endl;
      indent_string.append (NUM_INDENT_SPACES, ' ');
    }

    msgbuf << indent_string
           << "<form:" << node << " rdf:datatype=\"&xsd;boolean\">"
           << clean (valuebuf.str())
           << "</form:" << node << ">" << endl;

    for (list<string>::const_reverse_iterator lsi = keys.rbegin();
         lsi != keys.rend(); ++ lsi)
    {
      indent_string.erase(0, NUM_INDENT_SPACES);
      msgbuf << indent_string << "</form:" << * lsi << ">" << endl;
    }
    msgbuf << endl;
#endif
  }



  void rdf::
  store (char const * const key,
         CCTK_INT const value)
  {
#if 0
    ostringstream valuebuf;
    valuebuf << value;

    string node;
    list<string> const keys = parse (key, node);
    string indent_string (NUM_INDENT_SPACES, ' ');
    for (list<string>::const_iterator lsi = keys.begin();
         lsi != keys.end(); ++ lsi)
    {
      msgbuf << indent_string << "<form:" << * lsi << ">" << endl;
      indent_string.append (NUM_INDENT_SPACES, ' ');
    }

    msgbuf << indent_string
           << "<form:" << node << " rdf:datatype=\"&xsd;integer\">"
           << clean (valuebuf.str())
           << "</form:" << node << ">" << endl;

    for (list<string>::const_reverse_iterator lsi = keys.rbegin();
         lsi != keys.rend(); ++ lsi)
    {
      indent_string.erase(0, NUM_INDENT_SPACES);
      msgbuf << indent_string << "</form:" << * lsi << ">" << endl;
    }
    msgbuf << endl;
#endif
  }



  void rdf::
  store (char const * const key,
         CCTK_REAL const value)
  {
#if 0
    int const prec = numeric_limits<CCTK_REAL>::digits10;
    ostringstream valuebuf;
    valuebuf << setprecision(prec) << value;

    string node;
    list<string> const keys = parse (key, node);
    string indent_string (NUM_INDENT_SPACES, ' ');
    for (list<string>::const_iterator lsi = keys.begin();
         lsi != keys.end(); ++ lsi)
    {
      msgbuf << indent_string << "<form:" << * lsi << ">" << endl;
      indent_string.append (NUM_INDENT_SPACES, ' ');
    }

    msgbuf << indent_string
           << "<form:" << node << " rdf:datatype=\"&xsd;double\">"
           << clean (valuebuf.str())
           << "</form:" << node << ">" << endl;

    for (list<string>::const_reverse_iterator lsi = keys.rbegin();
         lsi != keys.rend(); ++ lsi)
    {
      indent_string.erase(0, NUM_INDENT_SPACES);
      msgbuf << indent_string << "</form:" << * lsi << ">" << endl;
    }
    msgbuf << endl;
#endif
  }



  void rdf::
  store (char const * const key,
         char const * const value)
  {
#if 0
    // don't store keys with empty string values
    if (not *value) return;

    ostringstream valuebuf;
    valuebuf << value;

    string node;
    list<string> const keys = parse (key, node);
    string indent_string (NUM_INDENT_SPACES, ' ');
    for (list<string>::const_iterator lsi = keys.begin();
         lsi != keys.end(); ++ lsi)
    {
      msgbuf << indent_string << "<form:" << * lsi << ">" << endl;
      indent_string.append (NUM_INDENT_SPACES, ' ');
    }

    msgbuf << indent_string
           // FIXME: is <string> the default datatype for RDF objects ??
           << "<form:" << node << ">" // " rdf:datatype=\"&xsd;string\">"
           << clean (valuebuf.str())
           << "</form:" << node << ">" << endl;

    for (list<string>::const_reverse_iterator lsi = keys.rbegin();
         lsi != keys.rend(); ++ lsi)
    {
      indent_string.erase(0, NUM_INDENT_SPACES);
      msgbuf << indent_string << "</form:" << * lsi << ">" << endl;
    }
    msgbuf << endl;
#endif
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


#if 0
  static list<string>
  parse (char const * const key, string& node)
  {
    assert (key);
    string str(key);
    list<string> strs;
    size_t p = 0;
    for (;;) {
      size_t const s = str.find ("/", p);
      if (s == string::npos) break;
      strs.push_back (str.substr (p, s - p));
      p = s + 1;
    }
    node = str.substr (p);
    return strs;
  }
#endif


} // namespace Formaline
