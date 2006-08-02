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

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cctk.h"
#include "cctk_Parameters.h"
#include "cctk_Version.h"
#include "util_String.h"
#include "util_Network.h"

#include "rdf.hh"
#include "senddata.hh"



namespace Formaline
{

  using namespace std;

  

  // NUM_RDF_ENTRIES must match the size of the
  // Formaline::rdf_hostname and Formaline::rdf_port parameter arrays
  int const NUM_RDF_ENTRIES = 5;

  // Number of space chars for indentation
  int const NUM_INDENT_SPACES = 2;



#if 0
  static list<string>
  parse (char const * const key, string& node);
#endif


  rdf::
  rdf (char const * const id,
       enum state const st,
       cGH const * const cctkGH)
    : storage (st)
  {
    //
    // RDF/XML document header with some namespace definitions
    //
    msgbuf
      << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << endl
      << "<!DOCTYPE owl [" << endl
      << "\t<!ENTITY rdf  'http://www.w3.org/1999/02/22-rdf-syntax-ns#'>" << endl
      << "\t<!ENTITY xsd  'http://www.w3.org/2001/XMLSchema#'>" << endl
      << "\t<!ENTITY cctk 'http://www.cct.lsu.edu/~dstark/cctk/0.1/'>" << endl
#if 0
      << "<!--" << endl
      << "\t<!ENTITY dc   'http://purl.org/dc/elements/1.1/'>" << endl
      << "\t<!ENTITY doap 'http://usefulinc.com/ns/doap#'>" << endl
      << "\t<!ENTITY foaf 'http://xmlns.com/foaf/0.1/'>" << endl
      << "\t<!ENTITY rdfs 'http://www.w3.org/2000/01/rdf-schema#'>" << endl
      << "\t<!ENTITY form 'http://www.aei.mpg.de/form#'>" << endl
      << "-->" << endl
#endif
      << "]>" << endl
      << "<rdf:RDF xmlns:rdf=\"&rdf;\"" << endl
      << "\txmlns:xsd=\"&xsd;\"" << endl
      << "\txmlns:cctk=\"&cctk;\"" << endl
      << ">" << endl
#if 0
      << "<!--" << endl
      << "\txmlns:dc=\"&dc;\"" << endl
      << "\txmlns:doap=\"&doap;\"" << endl
      << "\txmlns:foaf=\"&foaf;\"" << endl
      << "\txmlns:rdfs=\"&rdfs;\"" << endl
      << "\txmlns:form=\"&form;\"" << endl
      << "-->" << endl
      << "" << endl
      << "<!-- lessons learned so far" << endl
      << "  " << endl
      << "     * although it would be better to use rdf:ID as relative URIs for objects" << endl
      << "       (because they are enforced to be unique), rdf:about must be used instead" << endl
      << "       because the value of a relative rdf:about URI may contain special" << endl
      << "       characters (such as '/') whereas rdf:ID must not" << endl
      << "-->" << endl
#endif
      << endl
      << endl;


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
    const string compiled_at (clean (CCTK_CompileDateTime ()));
    char* rundatebuf = Util_CurrentDateTime ();
    const string started_at (clean (rundatebuf));
    free (rundatebuf);
    char cwdbuf[512];
    getcwd (cwdbuf, sizeof (cwdbuf));
    const string cwd (clean (cwdbuf));


    msgbuf << "<cctk:Simulation rdf:about=\"#" << jobID << "\"" << endl
           << "\tcctk:simulationID=\""  << jobID       << "\"" << endl
           << "\tcctk:host=\""          << host        << "\"" << endl
           << "\tcctk:nProcs=\""        << nprocs      << "\"" << endl
           << "\tcctk:user=\""          << user        << "\"" << endl
           << "\tcctk:executable=\""    << executable  << "\"" << endl
           << "\tcctk:parameterFile=\"" << parfile     << "\"" << endl
           << "\tcctk:hasVersion=\""    << version     << "\"" << endl
           << "\tcctk:compiledAt=\""    << compiled_at << "\"" << endl
           << "\tcctk:startedAt=\""     << started_at  << "\"" << endl;
    const char* const pbsJobID = getenv ("PBS_JOBID");
    if (pbsJobID) {
      msgbuf << "\tcctk:pbsJobID=\"" << clean (pbsJobID) << "\"" << endl;
    }
    msgbuf << "\tcctk:cwd=\"" << cwd         << "\">" << endl;

    //
    // metadata as references to other nodes
    //
    msgbuf << "\t<cctk:hasThornList rdf:resource=\"#ThornList\"/>" << endl
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
             << thornname << "\"" << endl;
      msgbuf << "\tcctk:hasName=\"" << thornname << "\"" << endl;

      // skip parameters that belong to inactive thorns
      const bool is_active = CCTK_IsThornActive (thornname);
      msgbuf << "\tcctk:isActive=\"" << (is_active ? "true" : "false") << "\"";

      // loop over all parameters of this thorn (if it is active)
      bool node_contains_urirefs = false;
      if (is_active) {
        for (int first = 1; ; first = 0) {
          char* fullname = NULL;
          const cParamData* pdata = NULL;

          // get the first/next parameter
          const int ierr = CCTK_ParameterWalk (first, thornname,&fullname,&pdata);
          assert (ierr >= 0);
          if (ierr > 0) break;

          if (not node_contains_urirefs) msgbuf << ">" << endl;
          node_contains_urirefs = true;

          // brackets in array parameter names have to be escaped
          msgbuf << "\t<cctk:hasParameter rdf:resource=\"#Parameters/"
                 << pdata->thorn << "/" << cleanURI (pdata->name) << "\"/>"
                 << endl;

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

            // brackets in array parameter names have to be escaped
            parambuf << "<cctk:" << paramtype << " rdf:about=\"#Parameters/"
                     << pdata->thorn << "/" << cleanURI (pdata->name) << "\""
                     << endl
                     << "\tcctk:hasName=\"" << fullname <<"\"" << endl
                     << "\tcctk:hasValue=\"" << paramvaluebuf.str() << "\"/>"
                     << endl;
          } // if (pdata->n_set or list_all_parameters)

          free (fullname);

        } // loop over all parameters of this thorn
      } // if (is_active)

      msgbuf << (node_contains_urirefs ? "</cctk:Thorn>" : "/>") << endl;

    } // loop over all thorns

    msgbuf << endl << parambuf.str();
  }


  void rdf::Update (cGH const * const cctkGH)
  {
    DECLARE_CCTK_PARAMETERS;

    if (verbose) {
      if (get_state() == update) {
        CCTK_INFO ("Announcing RDF metadata information update");
      } else {
        CCTK_INFO ("Announcing final RDF metadata information");
      }
    }

    char* currentdatebuf = Util_CurrentDateTime ();
    const string currentdate (clean (currentdatebuf));
    free (currentdatebuf);

    // update counter which is incremented for each Update() call
    static int update_counter = 0;
    update_counter++;

    msgbuf
<< "<cctk:Simulation rdf:about=\"#" << jobID << "\">" << endl
<< "\t<cctk:updatedInfo rdf:resource=\"#UpdateInfo/" << update_counter << "\"/>" << endl
<< "</cctk:Simulation>" << endl << endl
<< "<cctk:UpdateInfo rdf:about=\"#UpdateInfo/" << update_counter << "\"" << endl
<< "\tcctk:iteration=\"" << cctkGH->cctk_iteration << "\"" << endl
<< "\tcctk:time=\"" << cctkGH->cctk_time << "\"" << endl
<< "\tcctk:datetime=\"" << currentdate << "\"";

    if (get_state() == final) {
      msgbuf << endl << "\tcctk:terminated=\"yes\"";
    }
    msgbuf << "/>" << endl;
  }


  rdf::
  ~ rdf ()
  {
    DECLARE_CCTK_PARAMETERS;
    
    // Create the data string
    string const msgstr = msgbuf.str();

    // Loop over all destinations
    for (int i = 0; i < NUM_RDF_ENTRIES; i++) {
      if (*rdf_hostname[i]) {

        // Create the data
        ostringstream databuf;
        databuf
          // currently the RDF server only understands HTTP PUT
          // << "POST HTTP/1.0 200\r\n"
<< "PUT /context/CactusSimulations/" << jobID << " HTTP/1.0\r\n"
<< "Host: " << rdf_hostname[i] << "\r\n"
<< "Content-Type: application/rdf+xml\r\n"
<< "Content-Length: " << msgstr.length() << "\r\n"
<< "\r\n"
<< msgstr
<< "\r\n"
<< "\r\n";
        string const datastr = databuf.str();

        // Send the data
        SendData (rdf_hostname[i], rdf_port[i], datastr);
        
      }
    } // loop over all destinations
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


  string rdf::
  cleanURI (string const & uri)
    const
  {
    const string allowed_charset ("-_.!~*'()/");
    ostringstream buf;

    for (string::const_iterator p = uri.begin(); p != uri.end(); ++ p) {
      if (isalnum (*p) or allowed_charset.find (*p, 0) != string::npos) {
        buf << *p;
      } else if (*p == ' ') {
        buf << '+';
      } else {
        buf << '%' << hex << int (*p);
      }
    }

    return buf.str();
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
