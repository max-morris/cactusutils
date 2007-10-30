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
#include "util_Network.h"
#include "util_String.h"

#include "Publish.h"

#include "rdf.hh"
#include "senddata.hh"



namespace Formaline
{
  using namespace std;

  // the jobID is shared between this source file and PublishAsRDF.cc
  string jobID;



  // NUM_RDF_ENTRIES must match the size of the
  // Formaline::rdf_hostname and Formaline::rdf_port parameter arrays
  int const NUM_RDF_ENTRIES = 5;

  // Number of space chars for indentation
  // int const NUM_INDENT_SPACES = 2;



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
    // set the unique ID for this simulation
    jobID = clean (string (id));

    //
    // document contents
    //
    switch (get_state()) {
      case initial: Initial (); break;
      case update:
      case final:   Update (cctkGH); break;
      default:      assert (0); // invalid state
    }
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
           << "\tcctk:user=\""          << user        << "\"" << endl
           << "\tcctk:executable=\""    << executable  << "\"" << endl
           << "\tcctk:version=\""       << version     << "\"" << endl;
    const char* const pbsJobID = getenv ("PBS_JOBID");
    if (pbsJobID) {
      msgbuf << "\tcctk:pbsJobID=\"" << clean (pbsJobID) << "\"" << endl;
    }
    const char* const pbsJobname = getenv ("PBS_JOBNAME");
    if (pbsJobname) {
      msgbuf << "\tcctk:pbsJobname=\"" << clean (pbsJobname) << "\"" << endl;
    }
    const char* pbsHost = getenv ("PBS_O_HOST");
    if (not pbsHost) {
      // check whether we are running on damiana where the MPI runtime system
      // doesn't pass on PBS environment settings
      if (strlen(hostbuf) == 21 &&
          strncmp(hostbuf, "node", 4) == 0 &&
          strncmp(hostbuf + 7, ".damiana.admin", 13) == 0) {
        pbsHost = "damiana.damiana.admin";
      }
    }
    if (pbsHost) {
      // fix incomplete and/or strange PBS headnode hostnames
      if (strncmp(pbsHost, "peyote", 6) == 0) {
        pbsHost = "peyote.aei.mpg.de";
      } else if (strcmp(pbsHost, "master.ic") == 0) {
        pbsHost = "belladonna.aei.mpg.de";
      } else if (strcmp(pbsHost, "damiana.damiana.admin") == 0) {
        pbsHost = "damiana.aei.mpg.de";
      }
      msgbuf << "\tcctk:pbsHost=\"" << clean (pbsHost) << "\"" << endl;
    }
    msgbuf << "\tcctk:cwd=\"" << cwd         << "\">" << endl
           << "\t<cctk:nProcs rdf:datatype=\"&xsd;integer\">"
           << nprocs << "</cctk:nProcs>" << endl
           << "\t<cctk:compiledAt rdf:datatype=\"&xsd;dateTime\">"
           << compiled_at << "</cctk:compiledAt>" << endl
           << "\t<cctk:startedAt rdf:datatype=\"&xsd;dateTime\">"
           << started_at << "</cctk:startedAt>" << endl
           << "\t<cctk:lastModified rdf:datatype=\"&xsd;dateTime\">"
           << started_at << "</cctk:lastModified>" << endl;

    //
    // metadata as references to other nodes
    //
    msgbuf << "\t<cctk:thornList rdf:resource=\"#ThornList\"/>" << endl
           << "\t<cctk:parameterFile rdf:resource=\"#ParameterFile\"/>" << endl
           << "</cctk:Simulation>" << endl << endl;

    // store thorn list
    msgbuf << "<cctk:ThornList rdf:about=\"#ThornList\">" << endl;
    const int numthorns = CCTK_NumCompiledThorns ();
    for (int thorn = 0; thorn < numthorns; ++ thorn) {
      const char* const thornname = CCTK_CompiledThorn (thorn);

      msgbuf << "\t<cctk:thorn rdf:resource=\"#Thorns/"
             << thornname << "\"/>" << endl;
    }
    msgbuf << "</cctk:ThornList>" << endl << endl;

    // store parameter file name and contents
    char parfilebuf[512] = "";
    CCTK_ParameterFilename (sizeof (parfilebuf), parfilebuf);
    const string parfile = clean (parfilebuf);
    msgbuf << "<cctk:ParameterFile rdf:about=\"#ParameterFile\"" << endl
           << "\tcctk:name=\"" << parfile     << "\">" << endl
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
      msgbuf << "\tcctk:name=\"" << thornname << "\">" << endl;

      // skip parameters that belong to inactive thorns
      const bool is_active = CCTK_IsThornActive (thornname);
      msgbuf << "\t<cctk:active rdf:datatype=\"&xsd;boolean\">"
             << (is_active ? "true" : "false") << "</cctk:active>" << endl;

      // loop over all parameters of this thorn (if it is active)
      if (is_active) {
        for (int first = 1; ; first = 0) {
          char* fullname = NULL;
          const cParamData* pdata = NULL;

          // get the first/next parameter
          const int ierr = CCTK_ParameterWalk (first, thornname,
                                               &fullname, &pdata);
          assert (ierr >= 0);
          if (ierr > 0) break;

          // brackets in array parameter names have to be escaped
          msgbuf << "\t<cctk:parameter rdf:resource=\"#Parameters/"
                 << pdata->thorn << "/" << cleanURI (pdata->name) << "\"/>"
                 << endl;

          // get its value
          const void* const pvalue
            = CCTK_ParameterGet (pdata->name, pdata->thorn, NULL);
          assert (pvalue);

          if (pdata->n_set or list_all_parameters) {
            const char* paramtype;
            const char* paramdatatype;
            ostringstream paramvaluebuf;

            switch (pdata->type) {
              case PARAMETER_BOOLEAN:
              {
                paramtype = "BooleanParameter";
                paramdatatype = "boolean";
                const CCTK_INT v = *static_cast<const CCTK_INT*> (pvalue);
                paramvaluebuf << (v ? "true" : "false");
              }
              break;

              case PARAMETER_INT:
              {
                paramtype = "IntegerParameter";
                paramdatatype = "integer";
                const CCTK_INT v = *static_cast<const CCTK_INT*> (pvalue);
                paramvaluebuf << v;
              }
              break;

              case PARAMETER_REAL:
              {
                paramtype = "RealParameter";
                paramdatatype = "double";
                CCTK_REAL const v = *static_cast<const CCTK_REAL*> (pvalue);
                paramvaluebuf << v;
              }
              break;

              case PARAMETER_KEYWORD:
              {
                paramtype = "KeywordParameter";
                paramdatatype = "string";
                const char* const v = *static_cast<const char* const*> (pvalue);
                paramvaluebuf << clean (v);
              }
              break;

              case PARAMETER_STRING:
              {
                paramtype = "StringParameter";
                paramdatatype = "string";
                const char* const v = *static_cast<const char* const*> (pvalue);
                paramvaluebuf << clean (v);
              }
              break;

              default: assert (0); // invalid parameter type

            } // switch (pdata->type)

            // brackets in array parameter names have to be escaped
            parambuf << "<cctk:" << paramtype << " rdf:about=\"#Parameters/"
                     << pdata->thorn << "/" << cleanURI (pdata->name) << "\""
                     << endl
                     << "\tcctk:name=\"" << fullname <<"\">" << endl
                     << "\t<cctk:value rdf:datatype=\"&xsd;" << paramdatatype
                     << "\">" << paramvaluebuf.str() << "</cctk:value>" << endl
                     << "</cctk:" << paramtype << ">" << endl;
          } // if (pdata->n_set or list_all_parameters)

          free (fullname);

        } // loop over all parameters of this thorn
      } // if (is_active)

      msgbuf << "</cctk:Thorn>" << endl;

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

    if (CCTK_IsFunctionAliased ("PublishBoolean")) {
#ifndef PUBLISH_LEVEL_NOTICE
#define PUBLISH_LEVEL_NOTICE 2
#endif
      const int retval = PublishBoolean (cctkGH, PUBLISH_LEVEL_NOTICE,
                                         get_state() == final ? 1 : 0,
                                         "Finished", "Runtime Info");
      if (retval < 0) {
        CCTK_VWarn (1, __LINE__, __FILE__, CCTK_THORNSTRING,
                    "Failed to publish runtime information (error code %d)",
                    retval);
      }
    }

    // check if there was anything published
    if (rdfPublishList.empty()) return;

    char* rundatebuf = Util_CurrentDateTime ();
    const string started_at (clean (rundatebuf));

    static int publishedItems = 0;
    msgbuf << "<cctk:Simulation rdf:about=\"#" << jobID << "\">" << endl
           << "\t<cctk:lastModified rdf:datatype=\"&xsd;dateTime\">"
           << started_at << "</cctk:lastModified>" << endl;
    for (size_t i = 0; i < rdfPublishList.size(); i++) {
      msgbuf << "\t<cctk:publish rdf:resource=\"#Publish/"
             << (publishedItems + i) << "\"/>" << endl;
    }
    msgbuf << "</cctk:Simulation>" << endl << endl;
    for (size_t i = 0; i < rdfPublishList.size(); i++, publishedItems++) {
      const rdfPublishItem& item = rdfPublishList[i];
      msgbuf << "<cctk:Publish rdf:about=\"#Publish/"
             << publishedItems << "\">" << endl
             << "\t<cctk:datetime rdf:datatype=\"&xsd;dateTime\">"
             << item.datetime << "</cctk:datetime>" << endl
             << "\t<cctk:key>" << item.key << "</cctk:key>" << endl;
      if (not item.name.empty()) {
        msgbuf << "\t<cctk:name>" << item.name << "</cctk:name>" << endl;
      }
      if (item.hasCCTKinfo) {
        msgbuf << "\t<cctk:time>" << item.cctk_time << "</cctk:time>" << endl;
        msgbuf << "\t<cctk:iteration>" << item.cctk_iteration
               << "</cctk:iteration>" << endl;
      }
      ostringstream tablebuf;
      if (item.isTable) {
        for (size_t j = 0; j < item.table.size(); j++) {
          const rdfTableEntry& entry = item.table[j];
          msgbuf << "\t<cctk:tableEntry rdf:resource=\"#Publish/"
                 << publishedItems << "/" << j << "\"/>" << endl;
          tablebuf << "<cctk:TableEntry rdf:about=\"#Publish/"
                   << publishedItems << "/" << j << "\">" << endl
                   << "\t<cctk:key>" << entry.key << "</cctk:key>" << endl
                   << "\t<cctk:value";
          if (not entry.value.type.empty()) {
            tablebuf << " rdf:datatype=\"&xsd;" << entry.value.type << "\"";
          }
          tablebuf << ">" << entry.value.value << "</cctk:value>" << endl
                   << "</cctk:TableEntry>" << endl;
        }
      } else {
        msgbuf << "\t<cctk:value";
        if (not item.scalar.type.empty()) {
          msgbuf << " rdf:datatype=\"&xsd;" << item.scalar.type << "\"";
        }
        msgbuf << ">" << item.scalar.value << "</cctk:value>" << endl;
      }
      msgbuf << "</cctk:Publish>" << endl;
      msgbuf << tablebuf.str();
    }

    rdfPublishList.clear();
  }


  rdf::
  ~ rdf ()
  {
    DECLARE_CCTK_PARAMETERS;

    // check if anything needs to be done
    if (msgbuf.str().empty()) return;

    // RDF/XML document header with some namespace definitions
    const string header =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
"<!DOCTYPE owl [\n"
"\t<!ENTITY rdf  'http://www.w3.org/1999/02/22-rdf-syntax-ns#'>\n"
"\t<!ENTITY xsd  'http://www.w3.org/2001/XMLSchema#'>\n"
"\t<!ENTITY cctk 'http://www.gac-grid.org/project-products/Software/InformationService/InformationProducer/CactusRDFProducer/2006/08/cctk-schema#'>\n"

#if 0
      << "\t<!ENTITY cctk 'http://www.aei.mpg.de/~tradke/cctk-schema#'>" << endl
      << "\t<!ENTITY cctk 'http://www.cct.lsu.edu/~dstark/cctk/0.1/'>" << endl
      << "<!--" << endl
      << "\t<!ENTITY dc   'http://purl.org/dc/elements/1.1/'>" << endl
      << "\t<!ENTITY doap 'http://usefulinc.com/ns/doap#'>" << endl
      << "\t<!ENTITY foaf 'http://xmlns.com/foaf/0.1/'>" << endl
      << "\t<!ENTITY rdfs 'http://www.w3.org/2000/01/rdf-schema#'>" << endl
      << "\t<!ENTITY form 'http://www.aei.mpg.de/form#'>" << endl
      << "-->" << endl
#endif
"]>\n"
"<rdf:RDF xmlns:rdf=\"&rdf;\"\n"
"\txmlns:xsd=\"&xsd;\"\n"
"\txmlns:cctk=\"&cctk;\"\n"
">\n"
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
"\n\n";

    // RDF/XML document footer
    const string footer = "\n</rdf:RDF>\n";

    const int len = header.length() + msgbuf.str().length() + footer.length();

    // Loop over all destinations
    for (int i = 0; i < NUM_RDF_ENTRIES; i++) {
      if (*rdf_hostname[i]) {

        // Create the data
        // use PUT to create a new context and
        // POST to add metadata to an existing one
        ostringstream databuf;
        databuf
<< (get_state() == initial ? "PUT" : "POST")
<< " /context/CactusSimulations/" << jobID;
//        if (get_state() != initial) databuf << "?action=update";
        databuf << " HTTP/1.0\r\n"
<< "Host: " << rdf_hostname[i] << "\r\n"
<< "Content-Type: application/rdf+xml\r\n"
<< "Content-Length: " << len << "\r\n\r\n"
<< header << msgbuf.str() << footer << "\r\n\r\n";

        // Send the data
        SendData (rdf_hostname[i], rdf_port[i], databuf.str());

      }
    } // loop over all destinations
  }



  void rdf::
  store (char const * const key,
         bool const value)
  {
    const void* dummy = &dummy;
    dummy = &key; dummy = &value;
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
    const void* dummy = &dummy;
    dummy = &key; dummy = &value;
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
    const void* dummy = &dummy;
    dummy = &key; dummy = &value;
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
    const void* dummy = &dummy;
    dummy = &key; dummy = &value;
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


  string
  clean (string const & txt)
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


  string
  cleanURI (string const & uri)
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
