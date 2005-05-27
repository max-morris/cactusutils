// $Header$

#include <sstream>

#include "cctk_Parameters.h"

#include "file.hh"



using namespace std;



file::
file (char const * const id)
{
  DECLARE_CCTK_PARAMETERS;
  
  ostringstream filenamebuf;
  filenamebuf << out_dir << "/" << storage_filename;
  string const filenamestring = filenamebuf.str();
  fil.open (filenamestring.c_str());
}



file::
~ file ()
{
  fil.close();
}



void file::
write (std::string const & msg)
{
  fil << msg;
}
