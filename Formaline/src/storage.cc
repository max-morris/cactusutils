// $Header$

#include <sstream>

#include "storage.hh"

using namespace std;



storage::
~ storage ()
{
}



void storage::
store (char const * const key,
       int const value)
{
  assert (key);
  
  ostringstream keybuf;
  keybuf << key;
  ostringstream valuebuf;
  valuebuf << value;
  
  ostringstream buf;
  buf << "<key>" << clean (keybuf.str()) << "</key> "
      << "<value>" << clean (valuebuf.str()) << "</value>\n";
  
  write (buf.str());
}



void storage::
store (char const * const key,
       char const * const value)
{
  assert (key);
  
  ostringstream keybuf;
  keybuf << key;
  ostringstream valuebuf;
  valuebuf << value;
  
  ostringstream buf;
  buf << "<key>" << clean (keybuf.str()) << "</key> "
      << "<value>" << clean (valuebuf.str()) << "</value>\n";
  
  write (buf.str());
}



string storage::
clean (string const & txt)
  const
{
  ostringstream buf;
  
  for (string::const_iterator p = txt.begin(); p != txt.end(); ++ p)
  {
    switch (* p)
    {
    case '<': buf << "<langle>"; break;
    case '>': buf << "<rangle>"; break;
//     case '"': buf << "<quote>"; break;
//     case '\\': buf << "<backslash>"; break;
    default: buf << * p;
    }
  }
  
  return buf.str();
}
