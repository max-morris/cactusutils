// $Header$

#ifndef FORMALINE_RDF_HH
#define FORMALINE_RDF_HH

#include "cctk.h"

#include <sstream>
#include <string>

#include "storage.hh"



namespace Formaline
{



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
    
    std::string jobID;

    std::string
    clean (std::string const & txt)
      const;

    std::string
    cleanURI (std::string const & uri)
      const;

    void Initial (void);
    void Update (cGH const * cctkGH);
  };



} // namespace Formaline



#endif // ifndef FORMALINE_RDF_HH
