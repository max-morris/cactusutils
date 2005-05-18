/* $Header$ */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"
#include "cctk_Version.h"

#include "util_Network.h"

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

#ifndef MSG_NOSIGNAL
#  define MSG_NOSIGNAL 0
#endif



struct connection;

static struct connection *
OpenConnection (char const * id);

static void
CloseConnection (struct connection * conn);

static void
WriteToConnection (struct connection * conn,
                   char const * msg);



static void
StoreIntKey (struct connection * conn,
             char const * key,
             int value);

static void
StoreStringKey (struct connection *,
                char const * key,
                char const * value);



void
Formaline (CCTK_ARGUMENTS)
{
  /* Create a unique job id */
  char const * restrict const jobid = "not unique";
  
  struct connection * restrict const conn = OpenConnection (jobid);
  assert (conn);
  
  /* Cactus */
  
  {
    char const * const cactus_version = CCTK_FullVersion();
    StoreStringKey (conn, "Cactus version", cactus_version);
  }
  
  
  
  /* Compiling */
  
  {
    char const * const compile_user = CCTK_CompileUser();
    StoreStringKey (conn, "compile user", compile_user);
  }
  
  {
    char const * const compile_date = CCTK_CompileDate();
    StoreStringKey (conn, "compile date", compile_date);
  }
  
  {
    char const * const compile_time = CCTK_CompileTime();
    StoreStringKey (conn, "compile time", compile_time);
  }
  
  
  
  /* Running */
  
  {
    char const * const run_user = CCTK_RunUser();
    StoreStringKey (conn, "run user", run_user);
  }
  
  {
    char run_date [1000];
    Util_CurrentDate (sizeof run_date, run_date);
    StoreStringKey (conn, "run date", run_date);
  }
  
  {
    char run_time [1000];
    Util_CurrentTime (sizeof run_time, run_time);
    StoreStringKey (conn, "run time", run_time);
  }
  
  {
    char run_host [1000];
    Util_GetHostName (run_host, sizeof run_host);
    StoreStringKey (conn, "run host", run_host);
  }
  
  {
    char const * run_title;
    int type;
    run_title = CCTK_ParameterGet ("run_title", "Cactus", & type);
    assert (type == CCTK_VARIABLE_STRING);
    StoreStringKey (conn, "run title", run_title);
  }
  
  
  
  /* Parameters */
  
  {
    char ** argv;
    int argc;
    int n;
    CCTK_CommandLine (& argv);
    for (argc = 0; argv [argc]; ++ argc);
    StoreIntKey (conn, "argc", argc);
    for (n = 0; n < argc; ++ n)
    {
      char buffer [1000];
      snprintf (buffer, sizeof buffer, "argv[%d]", n);
      StoreStringKey (conn, buffer, argv[n]);
    }
  }
  
  {
    char parameter_filename [10000];
    CCTK_ParameterFilename (sizeof parameter_filename, parameter_filename);
    StoreStringKey (conn, "parameter filename", parameter_filename);
  }
  
  {
    char parameter_filename [10000];
    char parameter_file [1000000];
    size_t count;
    FILE * file;
    CCTK_ParameterFilename (sizeof parameter_filename, parameter_filename);
    file = fopen (parameter_filename, "r");
    count = fread (parameter_file, 1, sizeof parameter_file - 1, file);
    fclose (file);
    assert (count < sizeof parameter_file - 1);
    parameter_file [count] = '\0';
    StoreStringKey (conn, "parameter file", parameter_file);
  }
  
  {
    char const * out_dir;
    int type;
    out_dir = CCTK_ParameterGet ("out_dir", "IO", & type);
    assert (type == CCTK_VARIABLE_STRING);
    StoreStringKey (conn, "out dir", out_dir);
  }
  
  {
    int nprocs;
    nprocs = CCTK_nProcs (cctkGH);
    StoreIntKey (conn, "nprocs", nprocs);
  }
  
  CloseConnection (conn);
}



/*
  <simulation id="ID">
  <key>key</key> <value>value</value>
  </simulation>
 */



struct connection {
  SOCKET socket;
};

static struct connection *
OpenConnection (char const * restrict const id)
{
  DECLARE_CCTK_PARAMETERS;
  struct connection * const conn = malloc (sizeof * conn);
#warning "TODO: handle errors"
  assert (conn);
  
  conn->socket = socket (PF_INET, SOCK_STREAM, 0);
#warning "TODO: handle errors"
  
  struct hostent * hostinfo;
  hostinfo = gethostbyname (portal_hostname);
#warning "TODO: handle errors"
  assert (hostinfo);
  
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = portal_port;
  addr.sin_addr = * (struct in_addr *) hostinfo->h_addr;
  
  int const ierr = connect (conn->socket, & addr, sizeof addr);
#warning "TODO: handle errors"
  assert (! ERROR_CHECK (ierr));
  
  char buf[1000];
  snprintf (buf, sizeof buf, "<simulation id=\"%s\">\n", id);
  WriteToConnection (conn, buf);
  
  return conn;
}

static void
CloseConnection (struct connection * restrict const conn)
{
  assert (conn);
  
  char buf[1000];
  snprintf (buf, sizeof buf, "</simulation>\n");
  WriteToConnection (conn, buf);
  
  CLOSESOCKET (conn->socket);
#warning "TODO: handle errors"
  
  free (conn);
}

static void
WriteToConnection (struct connection * restrict const conn,
                   char const * restrict const msg)
{
  assert (conn);
  assert (msg);
  
  size_t const len = strlen (msg);
  ssize_t const nelems = send (conn, msg, len, MSG_NOSIGNAL);
  assert (! ERROR_CHECK (nelems));
#warning "TODO: handle errors"
#warning "TODO: handle overflow"
  assert (nelems == len);
}



static void
StoreIntKey (struct connection * restrict const conn,
             char const * restrict const key,
             int const value)
{
  assert (conn);
  assert (key);
  
  char buf[1000];
#warning "TODO: quote key and value"
  snprintf (buf, sizeof buf, "<key>%s</key> <value>%d</value>\n", key, value);
#warning "TODO: handle overflow"
  WriteToConnection (conn, buf);
}

static void
StoreStringKey (struct connection * restrict const conn,
                char const * restrict const key,
                char const * restrict const value)
{
  assert (conn);
  assert (key);
  assert (value);
  
  char buf[1000];
#warning "TODO: quote key and value"
  snprintf (buf, sizeof buf, "<key>%s</key> <value>%s</value>\n", key, value);
#warning "TODO: handle overflow"
  WriteToConnection (conn, buf);
}
