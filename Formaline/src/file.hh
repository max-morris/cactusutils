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
  file (char const * id);
  
  virtual
  ~ file ();
  
protected:
  
  virtual void
  write (std::string const & msg);
};



#endif // ifndef FILE_HH
