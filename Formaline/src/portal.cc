// $Header$

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>

#include "cctk.h"
#include "cctk_Parameters.h"

#include "portal.hh"



#ifndef MSG_NOSIGNAL
#  define MSG_NOSIGNAL 0
#endif



namespace Formaline
{

  using namespace std;



  portal::
  portal (char const * const id,
          enum state const st)
    : storage (st),
      errorcount (0)
  {
    DECLARE_CCTK_PARAMETERS;
  
    sock = socket (PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
      CCTK_VWarn (1, __LINE__, __FILE__, CCTK_THORNSTRING,
                  "%s", strerror (errno));
    }
  
    struct hostent * hostinfo;
    hostinfo = gethostbyname (portal_hostname);
    if (hostinfo == 0)
    {
      switch (h_errno)
      {
      case HOST_NOT_FOUND:
        CCTK_WARN (1, "The specified host is unknown.");
        break;
      case NO_ADDRESS:
        // case NO_DATA:
        CCTK_WARN (1, "The requested name is valid but does not have an IP address.");
        break;
      case NO_RECOVERY:
        CCTK_WARN (1, "A non-recoverable name server error occurred.");
        break;
      case TRY_AGAIN:
        CCTK_WARN (1, "A temporary error occurred on an authoritative name server.  Try again later.");
        break;
      default:
        CCTK_WARN (1, "unknown error");
      }
    }
  
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons (portal_port);
    addr.sin_addr = * (struct in_addr *) hostinfo->h_addr;
  
    int const ierr
      = connect (sock, (struct sockaddr const *) (& addr), sizeof addr);
    if (ierr < 0)
    {
      CCTK_VWarn (1, __LINE__, __FILE__, CCTK_THORNSTRING,
                  "%s", strerror (errno));
    }
  
    msgbuf << "<?xml version='1.0' ?>"
           << "<methodCall><methodName>";
    switch (get_state())
    {
    case initial:
      msgbuf << "cactus.registerApplication";
      break;
    case update:
      msgbuf << "cactus.updateApplication";
      break;
    case final:
      msgbuf << "cactus.deregisterApplication";
      break;
    default:
      assert (0);
    }
    msgbuf << "</methodName>"
           << "<params><param><value><struct>"
           << "<member>"
           << "<name>jobid</name>"
           << "<value><string>" << clean (id) << "</string></value>"
           << "</member>";
  }



  portal::
  ~ portal ()
  {
    msgbuf << "</struct></value></param></params>";
    msgbuf << "</methodCall>";
    string const msgstr = msgbuf.str();
  
    ostringstream buf;
    buf << "POST HTTP/1.0 200\r\n"
        << "Content-Type: text/xml\r\n"
        << "Content-Length: " << msgstr.length() << "\r\n"
        << "\r\n"
        << msgstr << "\r\n"
        << "\r\n";
    write (buf.str());
  
    CLOSESOCKET (sock);
  }



  void portal::
  store (char const * const key,
         bool const value)
  {
    assert (key);
  
    ostringstream keybuf;
    keybuf << key;
    ostringstream valuebuf;
    valuebuf << (value ? "true" : "false");
  
    msgbuf << "<member>"
           << "<name>" << clean (keybuf.str()) << "</name>"
           << "<value><boolean>" << clean (valuebuf.str()) << "</boolean></value>"
           << "</member>";
  }



  void portal::
  store (char const * const key,
         int const value)
  {
    assert (key);
  
    ostringstream keybuf;
    keybuf << key;
    ostringstream valuebuf;
    valuebuf << value;
  
    msgbuf << "<member>"
           << "<name>" << clean (keybuf.str()) << "</name>"
           << "<value><int>" << clean (valuebuf.str()) << "</int></value>"
           << "</member>";
  }



  void portal::
  store (char const * const key,
         double const value)
  {
    assert (key);
  
    ostringstream keybuf;
    keybuf << key;
    ostringstream valuebuf;
    valuebuf << setprecision(15) << value;
  
    msgbuf << "<member>"
           << "<name>" << clean (keybuf.str()) << "</name>"
           << "<value><double>" << clean (valuebuf.str()) << "</double></value>"
           << "</member>";
  }



  void portal::
  store (char const * const key,
         char const * const value)
  {
    assert (key);
  
    ostringstream keybuf;
    keybuf << key;
    ostringstream valuebuf;
    valuebuf << value;
  
    msgbuf << "<member>"
           << "<name>" << clean (keybuf.str()) << "</name>"
           << "<value><string>" << clean (valuebuf.str()) << "</string></value>"
           << "</member>";
  }



  void portal::  
  write (string const & msg0)
  {
    // cout << "[" << msg0 << "]" << endl;
    string msg = msg0;
    for (;;)
    {
    
      char const * const cmsg = msg.c_str();
      size_t const len = msg.length();
    
      // TODO: Make sure that we don't wait forever here.  Maybe use
      // MSG_DONTWAIT.
      ssize_t const nelems = send (sock, cmsg, len, MSG_NOSIGNAL);
      if (nelems < 0)
      {
#ifdef EMSGSIZE
        if (nelems == EMSGSIZE)
        {
          // The socket type requires that message be sent atomically,
          // and the size of the message to be sent made this
          // impossible.
          // Try to send the message as two smaller messages.
          if (len > 0)
          {
            string const msg1 = msg.substr (0, len/2);
            string const msg2 = msg.substr (len/2);
            write (msg1);
            write (msg2);
          }
          else
          {
            // There is a limit.  In order to not be too inefficient,
            // just do nothing if even small messages cannot be sent.
          }
        }
        else
#endif
        {
          int const maxerrors = 10;
          ++ errorcount;
          if (errorcount <= maxerrors)
          {
            CCTK_VWarn (1, __LINE__, __FILE__, CCTK_THORNSTRING,
                        "%s", strerror (errno));
            if (errorcount == maxerrors)
            {
              CCTK_WARN (1, "Too many errors.  Suppressing further error messages.");
            }
          }
        }
        return;
      }
    
      if (nelems == len) return;
    
      // Wait until can write
      fd_set fds;
      FD_ZERO (& fds);
      assert (sock >= 0 and sock < FD_SETSIZE);
      FD_SET (sock, & fds);
      struct timeval timeout;
      timeout.tv_sec  = 10;       // wait no more than ten seconds
      timeout.tv_usec = 0;
      // TODO: Make sure that we don't wait forever here.
      int const icnt = select (FD_SETSIZE, 0, & fds, 0, & timeout);
      if (icnt == 0)
      {
        // There was a timeout
        CCTK_WARN (1, "Timeout: could not send message");
        return;
      }
      else if (icnt < 0)
      {
        // There was an error
        CCTK_VWarn (1, __LINE__, __FILE__, CCTK_THORNSTRING,
                    "%s", strerror (errno));
        return;
      }
    
      // Remove the characters that have been sent
      assert (nelems < len);
      msg = msg.substr (nelems);
    
    }
  }



  string portal::
  clean (string const & txt)
    const
  {
    ostringstream buf;
  
    for (string::const_iterator p = txt.begin(); p != txt.end(); ++ p)
    {
      switch (* p)
      {
      case '<': buf << "&lt;"; break;
      case '&': buf << "&amp;"; break;
      default: buf << * p;
      }
    }
  
    return buf.str();
  }



} // namespace Formaline
