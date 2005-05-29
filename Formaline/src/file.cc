// $Header$

#include <iomanip>
#include <ios>
#include <sstream>

#include "cctk_Parameters.h"

#include "file.hh"



using namespace std;



file::
file (char const * const id,
      enum state const st)
  : storage (st)
{
  DECLARE_CCTK_PARAMETERS;
  
  ostringstream filenamebuf;
  filenamebuf << out_dir << "/" << storage_filename;
  string const filenamestring = filenamebuf.str();
  
  ios::openmode const mode = get_state() == initial ? ios::trunc : ios::app;
  fil.open (filenamestring.c_str(), mode);
  
  if (get_state() == initial)
  {
    store ("jobid", id);
  }
}



file::
~ file ()
{
  if (get_state() == final)
  {
    store ("simulation", "done");
  }
  fil.close();
}



void file::
store (char const * const key,
       bool const value)
{
  assert (key);
  
  ostringstream keybuf;
  keybuf << key;
  ostringstream valuebuf;
  valuebuf << (value ? "yes" : "no");
  
  ostringstream buf;
  buf << clean (keybuf.str()) << "=" << clean (valuebuf.str()) << endl;
  
  write (buf.str());
}



void file::
store (char const * const key,
       int const value)
{
  assert (key);
  
  ostringstream keybuf;
  keybuf << key;
  ostringstream valuebuf;
  valuebuf << value;
  
  ostringstream buf;
  buf << clean (keybuf.str()) << "=" << clean (valuebuf.str()) << endl;
  
  write (buf.str());
}



void file::
store (char const * const key,
       double const value)
{
  assert (key);
  
  ostringstream keybuf;
  keybuf << key;
  ostringstream valuebuf;
  valuebuf << setprecision(15) << value;
  
  ostringstream buf;
  buf << clean (keybuf.str()) << "=" << clean (valuebuf.str()) << endl;
  
  write (buf.str());
}



void file::
store (char const * const key,
       char const * const value)
{
  assert (key);
  
  ostringstream keybuf;
  keybuf << key;
  ostringstream valuebuf;
  valuebuf << value;
  
  ostringstream buf;
  buf << clean (keybuf.str()) << "="
      << "\"" << clean (valuebuf.str()) << "\"" << endl;
  
  write (buf.str());
}



void file::
write (std::string const & msg)
{
  fil << msg;
}



string file::
clean (string const & txt)
  const
{
  ostringstream buf;
  
  for (string::const_iterator p = txt.begin(); p != txt.end(); ++ p)
  {
    switch (* p)
    {
    case '=': buf << "\\="; break;
    case '"': buf << "\\\""; break;
    case '\\': buf << "\\\\"; break;
    default: buf << * p;
    }
  }
  
  return buf.str();
}
