// $Header$

#ifndef FORMALINE_RDF_HH
#define FORMALINE_RDF_HH

#include "cctk_Arguments.h"

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
         CCTK_ARGUMENTS);
  
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

    void Initial (void);
    void Update (CCTK_ARGUMENTS);
  };



} // namespace Formaline



#endif // ifndef FORMALINE_RDF_HH
