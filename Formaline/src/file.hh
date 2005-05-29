// $Header$

#ifndef FILE_HH
#define FILE_HH



#include <fstream>
#include <string>

#include "storage.hh"



class file : public storage
{
  ofstream fil;
  
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
         int value);
  
  virtual void
  store (char const * key,
         double value);
  
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



#endif // ifndef FILE_HH
