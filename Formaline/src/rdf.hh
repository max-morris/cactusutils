// $Header$

#ifndef FORMALINE_RDF_HH
#define FORMALINE_RDF_HH

#include "cctk.h"

#include <sstream>
#include <string>
#include <vector>

#include "storage.hh"



namespace Formaline
{

  typedef struct {
    std::string type;
    std::string value;
  } rdfScalarValue;

  typedef struct {
    std::string    key;
    rdfScalarValue value;
  } rdfTableEntry;

  typedef struct {
    std::string datetime;
    bool        hasCCTKinfo;
    CCTK_REAL   cctk_time;
    int         cctk_iteration;
    std::string name;
    std::string key;
// classes (with constructors) are not allowed as members of a union
//    union value {
      rdfScalarValue             scalar;
      std::vector<rdfTableEntry> table;
//    }
    bool isTable;
  } rdfPublishItem;

  // buffer to keep RDF metadata until the next Update() call
  extern std::vector<rdfPublishItem> rdfPublishList;

  // the jobID string
  extern std::string jobID;

  std::string clean (std::string const & txt);

  std::string cleanURI (std::string const & uri);



  class rdf : public storage
  {
    
    std::ostringstream msgbuf;
    
  public:
  
    rdf (char const * id,
         enum state st,
         cGH const * cctkGH);
  
    virtual
    ~ rdf ();
  
    virtual void
    store (char const * key,
           bool value);
  
    virtual void
    store (char const * key,
           CCTK_INT value);
  
    virtual void
    store (char const * key,
           CCTK_REAL value);
  
    virtual void
    store (char const * key,
           char const * value);
  
  private:
    
    void Initial (void);
    void Update (cGH const * cctkGH);

  };



} // namespace Formaline



#endif // ifndef FORMALINE_RDF_HH
