// $Header$

#ifndef PORTAL_HH
#define PORTAL_HH



#include <sstream>
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
  
  std::ostringstream msgbuf;
  
  int errorcount;
  
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



#endif // ifndef PORTAL_HH
