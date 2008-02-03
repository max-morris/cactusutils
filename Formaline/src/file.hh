#ifndef FORMALINE_FILE_HH
#define FORMALINE_FILE_HH



#include <fstream>
#include <string>

#include "storage.hh"



namespace Formaline
{

  class file : public storage
  {
    std::ofstream fil;
  
  public:
  
    file (char const * id,
          enum state st);
  
    virtual
    ~ file ();
  
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
  
    void
    write (std::string const & msg);
  
    std::string
    clean (std::string const & txt)
      const;
  };



} // namespace Formaline



#endif // ifndef FORMALINE_FILE_HH
