// $Header$

#include <cassert>
#include <string>
#include <sstream>

#include "cctk_Parameters.h"

#include "portal.hh"



using std::string;
using std::ostringstream;



portal::
portal (char const * const id)
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
  
  int const ierr
    = connect (sock, (struct sockaddr const *) (& addr), sizeof addr);
#warning "TODO: handle errors"
  assert (! ERROR_CHECK (ierr));
  
  ostringstream buf;
  buf << "<simulation id=\"" << clean (id) << "\">\n";
  write (buf.str());
}



portal::
~ portal ()
{
  ostringstream buf;
  buf << "</simulation>\n";
  write (buf.str());
  
  CLOSESOCKET (sock);
#warning "TODO: handle errors"
}



void portal::  
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
