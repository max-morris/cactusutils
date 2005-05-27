// $Header$

#ifndef PORTAL_HH
#define PORTAL_HH



#include <string>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#  include <netinet/in.h>
#endif
#ifdef HAVE_NETDB_H
#  include <netdb.h>
#endif
#ifdef HAVE_ARPA_INET_H
#  include <arpa/inet.h>
#endif
#ifdef HAVE_WINSOCK2_H
#  include <winsock2.h>
#endif
#include <errno.h>

#ifndef SOCKET
#  define SOCKET int
#endif

#ifdef SOCKET_ERROR
#  define ERROR_CHECK(a)  ((a) == SOCKET_ERROR)
#else
#  define ERROR_CHECK(a)  ((a) < 0)
#endif

#ifdef HAVE_WINSOCK2_H
#  define CLOSESOCKET(a) closesocket(a)
#else
#  define CLOSESOCKET(a) close(a)
#endif

#include "storage.hh"



class portal : public storage
{
  SOCKET sock;
  
public:
  portal (char const * id);
  
  virtual
  ~ portal ();
  
protected:
  
  virtual void
  write (std::string const & msg);
};



#endif // ifndef PORTAL_HH
