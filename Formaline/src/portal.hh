#ifndef FORMALINE_PORTAL_HH
#define FORMALINE_PORTAL_HH

#include <sstream>
#include <string>

#include "storage.hh"



namespace Formaline
{



  class portal : public storage
  {
    
    std::ostringstream msgbuf;
    
  public:
  
    portal (char const * id,
            enum state st);
  
    virtual
    ~ portal ();
  
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
    
    std::string
    clean (std::string const & txt)
      const;
  };



} // namespace Formaline



#endif // ifndef FORMALINE_PORTAL_HH
