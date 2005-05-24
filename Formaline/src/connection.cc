// $Header$

#include <cassert>
#include <string>
#include <sstream>

#include "cctk_Parameters.h"

#include "connection.hh"



using std::string;
using std::ostringstream;



connection::
connection (char const * const id)
{
  DECLARE_CCTK_PARAMETERS;
  
  sock = socket (PF_INET, SOCK_STREAM, 0);
#warning "TODO: handle errors"
  
  struct hostent * hostinfo;
  hostinfo = gethostbyname (portal_hostname);
#warning "TODO: handle errors"
  assert (hostinfo);
  
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = portal_port;
  addr.sin_addr = * (struct in_addr *) hostinfo->h_addr;
  
  int const ierr = connect (sock, & addr, sizeof addr);
#warning "TODO: handle errors"
  assert (! ERROR_CHECK (ierr));
  
  ostringstream buf;
  buf << "<simulation id=\"" << clean (id) << "\">\n";
  write (buf.str());
}



connection::
~ connection ()
{
  ostringstream buf;
  buf << "</simulation>\n";
  write (buf.str());
  
  CLOSESOCKET (sock);
#warning "TODO: handle errors"
}



template<typename T>
void connection::
store (char const * const key,
       T const value)
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

template
void connection::
store (char const * key,
       int value);

template
void connection::
store (char const * key,
       char const * value);

template
void connection::
store (char const * key,
       char * value);



void connection::  
write (string const & msg)
{
  char const * const cmsg = msg.c_str();
  
  size_t const len = strlen (cmsg);
  ssize_t const nelems = send (sock, cmsg, len, MSG_NOSIGNAL);
  assert (! ERROR_CHECK (nelems));
#warning "TODO: handle errors"
#warning "TODO: handle overflow"
  assert (nelems == len);
}



string connection::
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
    case '"': buf << "<quote>"; break;
    case '\\': buf << "<backslash>"; break;
    default: buf << * p;
    }
  }
  
  return buf.str();
}
