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

namespace Formaline {
using namespace std;

// The jobID is shared between this source file and PublishAsRDF.cc
// ES 2008-04-29: Check this, I think this is wrong
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

rdf::rdf(char const *const id, enum state const st, cGH const *const cctkGH,
         char const *const n, rdf *const par)
    : storage(st), groupname(n), parent(par) {
  if (parent)
    return;

  // set the unique ID for this simulation
  jobID = clean(string(id));

  //
  // document contents
  //
  switch (get_state()) {
  case initial:
    Initial();
    break;
  case update:
  case final:
    Update(cctkGH);
    break;
  default:
    assert(0); // invalid state
  }
}

void rdf::Initial(void) {
  //
  // This code was copied over from function Formaline_AnnounceInitial()
  // in announce.cc and modified here to create a valid RDF/XML (rather then
  // an unstructured plain XML document).
  //
  DECLARE_CCTK_PARAMETERS;

  if (verbose)
    CCTK_INFO("Announcing initial RDF metadata information");

  //
  // general metadata with inlined attribute values
  //
  char hostbuf[512] = "";
  Util_GetHostName(hostbuf, sizeof(hostbuf));
  const string host = clean(hostbuf);
  const cGH *const cctkGH = NULL;
  const int nprocs = CCTK_nProcs(cctkGH);
#if 0
    const string user = clean (CCTK_RunUser());
#else
  const string user = clean(getenv("USER"));
#endif
  char **argv;
  const int argc = CCTK_CommandLine(&argv);
  const string executable = clean(argc >= 1 ? argv[0] : "");
  const string version = clean(CCTK_FullVersion());
  const string compiled_at(clean(CCTK_CompileDateTime()));
  char *rundatebuf = Util_CurrentDateTime();
  const string started_at(clean(rundatebuf));
  free(rundatebuf);
  char buf[512] = "";
  getcwd(buf, sizeof(buf));
  const string cwd(clean(buf));
  buf[0] = 0;
  CCTK_RunTitle(sizeof(buf) - 1, buf);
  const string title(clean(buf));
  const string configID(clean(static_cast<const char *>(UniqueConfigID(0))));
  const string buildID(clean(static_cast<const char *>(UniqueBuildID(0))));
  const string bbhID(clean(static_cast<const char *>(UniqueSimulationID(0))));
  const string runID(clean(static_cast<const char *>(UniqueRunID(0))));

  msgbuf << "<cctk:Simulation rdf:about=\"#" << jobID << "\"\n"
         << "\tcctk:simulationID=\"" << jobID << "\"\n"
         << "\tcctk:host=\"" << host << "\"\n"
         << "\tcctk:user=\"" << user << "\"\n"
         << "\tcctk:executable=\"" << executable << "\"\n"
         << "\tcctk:version=\"" << version << "\"\n"
         << "\tcctk:title=\"" << title << "\"\n"
         << "\tcctk:configID=\"" << configID << "\"\n"
         << "\tcctk:buildID=\"" << buildID << "\"\n"
         << "\tcctk:bbhID=\"" << bbhID << "\"\n"
         << "\tcctk:runID=\"" << runID << "\"\n";
  const char *const pbsJobID = getenv("PBS_JOBID");
  if (pbsJobID) {
    msgbuf << "\tcctk:pbsJobID=\"" << clean(pbsJobID) << "\"\n";
  }
  const char *const pbsJobname = getenv("PBS_JOBNAME");
  if (pbsJobname) {
    msgbuf << "\tcctk:pbsJobname=\"" << clean(pbsJobname) << "\"\n";
  }
#if 0
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
      msgbuf << "\tcctk:pbsHost=\"" << clean (pbsHost) << "\"\n";
    }
#endif
  msgbuf << "\tcctk:cwd=\"" << cwd << "\">\n"
         << "\t<cctk:nProcs rdf:datatype=\"&xsd;integer\">" << nprocs
         << "</cctk:nProcs>\n"
         << "\t<cctk:compiledAt rdf:datatype=\"&xsd;dateTime\">" << compiled_at
         << "</cctk:compiledAt>\n"
         << "\t<cctk:startedAt rdf:datatype=\"&xsd;dateTime\">" << started_at
         << "</cctk:startedAt>\n"
         << "\t<cctk:lastUpdated rdf:datatype=\"&xsd;dateTime\">" << started_at
         << "</cctk:lastUpdated>\n";

  //
  // metadata as references to other nodes
  //
  msgbuf << "\t<cctk:thornList rdf:resource=\"#ThornList\"/>\n"
         << "\t<cctk:parameterFile rdf:resource=\"#ParameterFile\"/>\n"
         << "</cctk:Simulation>\n\n";

#ifdef ALSO_STORE_THORNLIST_AND_PARAMETERS
  // store thorn list
  msgbuf << "<cctk:ThornList rdf:about=\"#ThornList\">\n";
  const int numthorns = CCTK_NumCompiledThorns();
  for (int thorn = 0; thorn < numthorns; ++thorn) {
    const char *const thornname = CCTK_CompiledThorn(thorn);

    msgbuf << "\t<cctk:thorn rdf:resource=\"#Thorns/" << thornname << "\"/>\n";
  }
  msgbuf << "</cctk:ThornList>\n\n";
#endif

  // store parameter file name and contents
  char parfilebuf[512] = "";
  CCTK_ParameterFilename(sizeof(parfilebuf), parfilebuf);
  const string parfile = clean(parfilebuf);
  msgbuf << "<cctk:ParameterFile rdf:about=\"#ParameterFile\"\n"
         << "\tcctk:name=\"" << parfile << "\">\n"
         << "\t<rdf:value>";
  ifstream file(parfile.c_str());
  char c;
  ostringstream filebuf;
  while (filebuf and file.get(c))
    filebuf.put(c);
  file.close();
  msgbuf << clean(filebuf.str());
  filebuf.clear();
  msgbuf << "</rdf:value>\n"
         << "</cctk:ParameterFile>\n\n";

#ifdef ALSO_STORE_THORNLIST_AND_PARAMETERS
  // store all parameters which have been set in the parfile
  msgbuf << "<!-- ============================================================ "
            "-->\n"
         << "<!-- thorn graphs with their parameters                           "
            "-->\n"
         << "<!-- ============================================================ "
            "-->\n";
  ostringstream parambuf;
  parambuf << "<!-- "
              "============================================================ "
              "-->\n"
           << "<!-- list of parameters and their values                        "
              "  -->\n"
           << "<!-- "
              "============================================================ "
              "-->\n";

  const bool list_all_parameters = CCTK_Equals(out_save_parameters, "all");

  for (int thorn = 0; thorn < numthorns; ++thorn) {
    const char *const thornname = CCTK_CompiledThorn(thorn);

    msgbuf << "<cctk:Thorn rdf:about=\"#Thorns/" << thornname << "\"\n";
    msgbuf << "\tcctk:name=\"" << thornname << "\">\n";

    // skip parameters that belong to inactive thorns
    const bool is_active = CCTK_IsThornActive(thornname);
    msgbuf << "\t<cctk:active rdf:datatype=\"&xsd;boolean\">"
           << (is_active ? "true" : "false") << "</cctk:active>\n";

    // loop over all parameters of this thorn (if it is active)
    if (is_active) {
      for (int first = 1;; first = 0) {
        char *fullname = NULL;
        const cParamData *pdata = NULL;

        // get the first/next parameter
        const int ierr =
            CCTK_ParameterWalk(first, thornname, &fullname, &pdata);
        assert(ierr >= 0);
        if (ierr > 0)
          break;

        // brackets in array parameter names have to be escaped
        msgbuf << "\t<cctk:parameter rdf:resource=\"#Parameters/"
               << pdata->thorn << "/" << cleanURI(pdata->name) << "\"/>"
               << "\n";

        // get its value
        const void *const pvalue =
            CCTK_ParameterGet(pdata->name, pdata->thorn, NULL);
        assert(pvalue);

        if (pdata->n_set or list_all_parameters) {
          const char *paramtype;
          const char *paramdatatype;
          ostringstream paramvaluebuf;

          switch (pdata->type) {
          case PARAMETER_BOOLEAN: {
            paramtype = "BooleanParameter";
            paramdatatype = "boolean";
            const CCTK_INT v = *static_cast<const CCTK_INT *>(pvalue);
            paramvaluebuf << (v ? "true" : "false");
          } break;

          case PARAMETER_INT: {
            paramtype = "IntegerParameter";
            paramdatatype = "integer";
            const CCTK_INT v = *static_cast<const CCTK_INT *>(pvalue);
            paramvaluebuf << v;
          } break;

          case PARAMETER_REAL: {
            paramtype = "RealParameter";
            paramdatatype = "double";
            CCTK_REAL const v = *static_cast<const CCTK_REAL *>(pvalue);
            paramvaluebuf << v;
          } break;

          case PARAMETER_KEYWORD: {
            paramtype = "KeywordParameter";
            paramdatatype = "string";
            const char *const v = *static_cast<const char *const *>(pvalue);
            paramvaluebuf << clean(v);
          } break;

          case PARAMETER_STRING: {
            paramtype = "StringParameter";
            paramdatatype = "string";
            const char *const v = *static_cast<const char *const *>(pvalue);
            paramvaluebuf << clean(v);
          } break;

          default:
            assert(0); // invalid parameter type

          } // switch (pdata->type)

          // brackets in array parameter names have to be escaped
          parambuf << "<cctk:" << paramtype << " rdf:about=\"#Parameters/"
                   << pdata->thorn << "/" << cleanURI(pdata->name) << "\""
                   << "\n"
                   << "\tcctk:name=\"" << fullname << "\">\n"
                   << "\t<cctk:value rdf:datatype=\"&xsd;" << paramdatatype
                   << "\">" << paramvaluebuf.str() << "</cctk:value>\n"
                   << "</cctk:" << paramtype << ">\n";
        } // if (pdata->n_set or list_all_parameters)

        free(fullname);

      } // loop over all parameters of this thorn
    }   // if (is_active)

    msgbuf << "</cctk:Thorn>\n";

  } // loop over all thorns

  msgbuf << "\n" << parambuf.str();
#endif
}

static string AppendToTransaction(const string operation, const string &subject,
                                  const string &predicate,
                                  const string &object) {
  ostringstream buf;
  buf << "<" << operation << ">\n"
                             "  <uri>&simID;" << subject << "</uri>\n"
                                                            "  <uri>&cctk;"
      << predicate << "</uri>\n"
                      "  " << object << "\n"
                                        "  <contexts>\n"
                                        "    <uri>&context;</uri>\n"
                                        "  </contexts>\n"
                                        "</" << operation << ">\n\n";
  return buf.str();
}

void rdf::Update(cGH const *const cctkGH) {
  DECLARE_CCTK_PARAMETERS;

  if (verbose) {
    if (get_state() == update) {
      CCTK_INFO("Announcing RDF metadata information update");
    } else {
      CCTK_INFO("Announcing final RDF metadata information");
    }
  }

  if (CCTK_IsFunctionAliased("PublishBoolean")) {
    const int retval = PublishBoolean(
        cctkGH, get_state() == final ? 1 : 2, get_state() == final ? 1 : 0,
        "Simulation finished ?", CCTK_THORNSTRING " runtime info");
    if (retval < 0) {
      CCTK_VWarn(1, __LINE__, __FILE__, CCTK_THORNSTRING,
                 "Failed to publish runtime information (error code %d)",
                 retval);
    }
  }

  // check if there was anything published
  if (rdfPublishList.empty())
    return;

  char *rundatebuf = Util_CurrentDateTime();
  const string started_at(clean(rundatebuf));
  free(rundatebuf);

  static int publishedItems = 0;
  for (size_t i = 0; i < rdfPublishList.size(); i++) {
    ostringstream object;
    object << "<uri>&simID;Publish/" << (publishedItems + i) << "</uri>";
    msgbuf << AppendToTransaction("add", jobID, "publish", object.str());
  }
  for (size_t i = 0; i < rdfPublishList.size(); i++, publishedItems++) {
    const rdfPublishItem &item = rdfPublishList[i];
    ostringstream subject, object;
    subject << "Publish/" << publishedItems;
    object << "<literal datatype=\"&xsd;dateTime\">" << item.datetime
           << "</literal>";
    msgbuf << AppendToTransaction("add", subject.str(), "datetime",
                                  object.str());
    object.str("");
    object << "<literal>" << item.key << "</literal>";
    msgbuf << AppendToTransaction("add", subject.str(), "key", object.str());
    if (not item.name.empty()) {
      object.str("");
      object << "<literal>" << item.name << "</literal>";
      msgbuf << AppendToTransaction("add", subject.str(), "name", object.str());
    }
    if (item.hasCCTKinfo) {
      object.str("");
      object << "<literal datatype=\"&xsd;double\">" << item.cctk_time
             << "</literal>";
      msgbuf << AppendToTransaction("add", subject.str(), "time", object.str());
      object.str("");
      object << "<literal datatype=\"&xsd;int\">" << item.cctk_iteration
             << "</literal>";
      msgbuf << AppendToTransaction("add", subject.str(), "iteration",
                                    object.str());
    }

    ostringstream tablebuf;
    if (item.isTable) {
      for (size_t j = 0; j < item.table.size(); j++) {
        const rdfTableEntry &entry = item.table[j];
        subject.str("");
        subject << "Publish/" << publishedItems;
        object.str("");
        object << "<uri>&simID;Publish/" << publishedItems << "/" << j
               << "</uri>";
        msgbuf << AppendToTransaction("add", subject.str(), "tableEntry",
                                      object.str());
        subject.str("");
        subject << "Publish/" << publishedItems << "/" << j;
        object.str("");
        object << "<literal>" << entry.key << "</literal>";
        msgbuf << AppendToTransaction("add", subject.str(), "key",
                                      object.str());
        object.str("");
        object << "<literal";
        if (not entry.value.type.empty()) {
          object << " datatype=\"&xsd;" << entry.value.type << "\"";
        }
        object << ">" << entry.value.value << "</literal>";
        msgbuf << AppendToTransaction("add", subject.str(), "value",
                                      object.str());
      }
    } else {
      object.str("");
      object << "<literal";
      if (not item.scalar.type.empty()) {
        object << " datatype=\"&xsd;" << item.scalar.type << "\"";
      }
      object << ">" << item.scalar.value << "</literal>";
      msgbuf << AppendToTransaction("add", subject.str(), "value",
                                    object.str());
    }
  }

  rdfPublishList.clear();
}

rdf::~rdf() {
  DECLARE_CCTK_PARAMETERS;

  if (parent) {
    parent->msgbuf << "<cctk:" << groupname << ">\n" << msgbuf.str()
                   << "</cctk:" << groupname << ">\n";
    return;
  }

  // check if anything needs to be done
  if (msgbuf.str().empty())
    return;

  // this simulation's RDF context
  const string context = "CactusSimulations:" + jobID;
  const string contextURI = cleanURI("<" + context + ">");

  // RDF/XML document header with some namespace definitions
  string header =
      "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
      "<!DOCTYPE owl [\n"
      "\t<!ENTITY rdf     'http://www.w3.org/1999/02/22-rdf-syntax-ns#'>\n"
      "\t<!ENTITY xsd     'http://www.w3.org/2001/XMLSchema#'>\n"
      "\t<!ENTITY cctk    "
      "'http://www.gac-grid.org/project-products/Software/InformationService/"
      "InformationProducer/CactusRDFProducer/2006/08/cctk-schema#'>\n"
      "\t<!ENTITY simID   '" +
      context + "#'>\n"
                "\t<!ENTITY context '" +
      context + "'>\n"
                "]>\n\n";
  if (get_state() == initial) {
    header += "<rdf:RDF xmlns:rdf=\"&rdf;\"\n"
              "  xmlns:xsd=\"&xsd;\"\n"
              "  xmlns:cctk=\"&cctk;\"\n"
              ">\n\n";
  } else {
    header += "<transaction>\n\n";
  }

  // RDF/XML document footer
  const string footer =
      get_state() == initial ? "\n</rdf:RDF>\n" : "\n</transaction>\n";

  if (get_state() != initial) {
    char *dateBuffer = Util_CurrentDateTime();
    const string lastUpdated = "<literal datatype=\"&xsd;dateTime\">" +
                               clean(dateBuffer) + "</literal>";
    free(dateBuffer);

    msgbuf << AppendToTransaction("remove", jobID, "lastUpdated", "<null/>");
    msgbuf << AppendToTransaction("add", jobID, "lastUpdated", lastUpdated);
  }

  const int len = header.length() + msgbuf.str().length() + footer.length();

  // Loop over all destinations
  for (int i = 0; i < NUM_RDF_ENTRIES; i++) {
    if (*rdf_hostname[i]) {

      // Create the data
      // use PUT to create a new context and
      // POST to add metadata to an existing one
      ostringstream databuf;
      databuf << (get_state() == initial ? "PUT" : "POST")
              << " /openrdf-sesame/repositories/Simulations/statements"
                 "?context=" << contextURI << "&baseURI=" << contextURI;
      //<< " /context/CactusSimulations/" << jobID;
      // set a metadata lifetime if requested by the user
      // (Formaline::time_to_live is hours but the RDF service wants seconds)
      // if (metadata_lifetime) databuf << "?ttl=" << (metadata_lifetime *
      // 3600);
      databuf << " HTTP/1.0\r\n"
              << "Host: " << rdf_hostname[i] << "\r\n"
              << "Content-Type: application/"
              << (get_state() == initial ? "rdf+xml" : "x-rdftransaction")
              << "\r\n"
              << "Content-Length: " << len << "\r\n\r\n" << header
              << msgbuf.str() << footer << "\r\n\r\n";

      // Send the data
      SendData(rdf_hostname[i], rdf_port[i], databuf.str());
    }
  } // loop over all destinations
}

rdf *rdf::open_group(char const *const name) {
  return new rdf(0, get_state(), 0, name, this);
}

void rdf::close_group(storage *group) { delete group; }

void rdf::store(char const *const key, bool const value) {
  const void *dummy = &dummy;
  dummy = &key;
  dummy = &value;
#if 0
    ostringstream valuebuf;
    valuebuf << (value ? "true" : "false");

    string node;
    list<string> const keys = parse (key, node);
    string indent_string (NUM_INDENT_SPACES, ' ');
    for (list<string>::const_iterator lsi = keys.begin();
         lsi != keys.end(); ++ lsi)
    {
      msgbuf << indent_string << "<form:" << * lsi << ">\n";
      indent_string.append (NUM_INDENT_SPACES, ' ');
    }

    msgbuf << indent_string
           << "<form:" << node << " rdf:datatype=\"&xsd;boolean\">"
           << clean (valuebuf.str())
           << "</form:" << node << ">\n";

    for (list<string>::const_reverse_iterator lsi = keys.rbegin();
         lsi != keys.rend(); ++ lsi)
    {
      indent_string.erase(0, NUM_INDENT_SPACES);
      msgbuf << indent_string << "</form:" << * lsi << ">\n";
    }
    msgbuf << "\n";
#endif
}

void rdf::store(char const *const key, CCTK_INT const value) {
  const void *dummy = &dummy;
  dummy = &key;
  dummy = &value;
#if 0
    ostringstream valuebuf;
    valuebuf << value;

    string node;
    list<string> const keys = parse (key, node);
    string indent_string (NUM_INDENT_SPACES, ' ');
    for (list<string>::const_iterator lsi = keys.begin();
         lsi != keys.end(); ++ lsi)
    {
      msgbuf << indent_string << "<form:" << * lsi << ">\n";
      indent_string.append (NUM_INDENT_SPACES, ' ');
    }

    msgbuf << indent_string
           << "<form:" << node << " rdf:datatype=\"&xsd;integer\">"
           << clean (valuebuf.str())
           << "</form:" << node << ">\n";

    for (list<string>::const_reverse_iterator lsi = keys.rbegin();
         lsi != keys.rend(); ++ lsi)
    {
      indent_string.erase(0, NUM_INDENT_SPACES);
      msgbuf << indent_string << "</form:" << * lsi << ">\n";
    }
    msgbuf << "\n";
#endif
}

void rdf::store(char const *const key, CCTK_REAL const value) {
  const void *dummy = &dummy;
  dummy = &key;
  dummy = &value;
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
      msgbuf << indent_string << "<form:" << * lsi << ">\n";
      indent_string.append (NUM_INDENT_SPACES, ' ');
    }

    msgbuf << indent_string
           << "<form:" << node << " rdf:datatype=\"&xsd;double\">"
           << clean (valuebuf.str())
           << "</form:" << node << ">\n";

    for (list<string>::const_reverse_iterator lsi = keys.rbegin();
         lsi != keys.rend(); ++ lsi)
    {
      indent_string.erase(0, NUM_INDENT_SPACES);
      msgbuf << indent_string << "</form:" << * lsi << ">\n";
    }
    msgbuf << "\n";
#endif
}

void rdf::store(char const *const key, char const *const value) {
  const void *dummy = &dummy;
  dummy = &key;
  dummy = &value;
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
      msgbuf << indent_string << "<form:" << * lsi << ">\n";
      indent_string.append (NUM_INDENT_SPACES, ' ');
    }

    msgbuf << indent_string
           // FIXME: is <string> the default datatype for RDF objects ??
           << "<form:" << node << ">" // " rdf:datatype=\"&xsd;string\">"
           << clean (valuebuf.str())
           << "</form:" << node << ">\n";

    for (list<string>::const_reverse_iterator lsi = keys.rbegin();
         lsi != keys.rend(); ++ lsi)
    {
      indent_string.erase(0, NUM_INDENT_SPACES);
      msgbuf << indent_string << "</form:" << * lsi << ">\n";
    }
    msgbuf << "\n";
#endif
}

string clean(string const &txt) {
  ostringstream buf;

  for (string::const_iterator p = txt.begin(); p != txt.end(); ++p) {
    switch (*p) {
    case '<':
      buf << "&lt;";
      break;
    case '&':
      buf << "&amp;";
      break;
    default:
      buf << *p;
    }
  }

  return buf.str();
}

string cleanURI(string const &uri) {
  const string allowed_charset("-_.!~*'()/");
  ostringstream buf;

  for (string::const_iterator p = uri.begin(); p != uri.end(); ++p) {
    if (isalnum(*p) or allowed_charset.find(*p, 0) != string::npos) {
      buf << *p;
    } else if (*p == ' ') {
      buf << '+';
    } else {
      buf << '%' << hex << int(*p);
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
