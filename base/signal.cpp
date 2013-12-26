#include "portable.h"
#include "signal.h"
#ifdef __linux__
#define HAVE_EVENTFD
#endif
#ifdef HAVE_EVENTFD
#include <sys/eventfd.h>
#endif
#include <assert.h>
#include <errno.h>

static void UnblockSocket(fd_t s)
{
#ifdef __linux__
  int flags = fcntl (s, F_GETFL, 0);
  if (flags == -1)
    flags = 0;
  int rc = fcntl (s, F_SETFL, flags | O_NONBLOCK);
#else
  u_long nonblock = 1;
  int rc = ioctlsocket (s, FIONBIO, &nonblock);
  assert (rc != SOCKET_ERROR);
#endif
}

static int MakeSocketPair(fd_t& r,fd_t& w)
{
#if defined __linux__
#if defined HAVE_EVENTFD
  fd_t fd=eventfd(0,0);
  assert(fd!=-1);
  w=fd;
  r=fd;
  return 0;
#else
  int sv[2];
  int rc=socketpair(AF_UNIX,SOCK_STREAM,0,sv);
  assert(rc==0);
  w=sv[0];
  r=sv[1];
  return 0;
#endif
#else
  SECURITY_DESCRIPTOR sd = {0};
  SECURITY_ATTRIBUTES sa = {0};
  InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
  SetSecurityDescriptorDacl(&sd, TRUE, 0, FALSE);
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = &sd;
  HANDLE sync = CreateEvent (&sa, FALSE, TRUE, TEXT ("Global\\zmq-signaler-port-sync"));
  if (sync == NULL && GetLastError () == ERROR_ACCESS_DENIED)
    sync = OpenEvent (SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, TEXT ("Global\\zmq-signaler-port-sync"));

  assert(sync != NULL);
  DWORD dwrc = WaitForSingleObject (sync, INFINITE);
  assert (dwrc == WAIT_OBJECT_0);
  w = INVALID_SOCKET;
  r = INVALID_SOCKET;

  SOCKET listener;
  listener = socket(AF_INET, SOCK_STREAM, 0);
  BOOL brc = SetHandleInformation ((HANDLE) listener, HANDLE_FLAG_INHERIT, 0);

  //  Set SO_REUSEADDR and TCP_NODELAY on listening socket.
  BOOL so_reuseaddr = 1;
  int rc = setsockopt (listener, SOL_SOCKET, SO_REUSEADDR,
    (char *)&so_reuseaddr, sizeof (so_reuseaddr));
  BOOL tcp_nodelay = 1;
  rc = setsockopt (listener, IPPROTO_TCP, TCP_NODELAY,
    (char *)&tcp_nodelay, sizeof (tcp_nodelay));

  struct sockaddr_in addr;
  memset (&addr, 0, sizeof (addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
  addr.sin_port = htons (5905);
  rc = bind (listener, (const struct sockaddr*) &addr, sizeof (addr));
  rc = listen (listener, 1);

  //  Create the writer socket.
  w = WSASocket (AF_INET, SOCK_STREAM, 0, NULL, 0,  0);

  //  On Windows, preventing sockets to be inherited by child processes.
  brc = SetHandleInformation ((HANDLE) w, HANDLE_FLAG_INHERIT, 0);

  //  Set TCP_NODELAY on writer socket.
  rc = setsockopt (w, IPPROTO_TCP, TCP_NODELAY,
    (char *)&tcp_nodelay, sizeof (tcp_nodelay));

  //  Connect writer to the listener.
  rc = connect (w, (struct sockaddr*) &addr, sizeof (addr));

  //  Save errno if connection fails
  int conn_errno = 0;
  if (rc == SOCKET_ERROR) {
    conn_errno = WSAGetLastError ();
  } else {
    //  Accept connection from writer.
    r = accept (listener, NULL, NULL);

    if (r == INVALID_SOCKET) {
      conn_errno = WSAGetLastError ();
    }
  }

  rc = closesocket (listener);
  brc = SetEvent (sync);
  brc = CloseHandle (sync);

  if (r != INVALID_SOCKET) {
    brc = SetHandleInformation ((HANDLE) r, HANDLE_FLAG_INHERIT, 0);
    return 0;
  } else {
    rc = closesocket (w);
    w = INVALID_SOCKET;
    return -1;
  }
#endif
}

fd_t base::ezSignaler::getfd()
{
  return r_;
}

void base::ezSignaler::send()
{
#if defined __linux__
#if defined HAVE_EVENTFD
  uint64_t dummy=1;
  ssize_t n=write(w_,&dummy,sizeof(dummy));
  assert(n==sizeof(dummy));
#else
  unsigned char dummy=0;
  while(true) 
  {
    ssize_t nbytes = ::send (w_, &dummy, sizeof (dummy), 0);
    if (nbytes==-1&&errno==EINTR)
      continue;
    break;
  }
#endif
#else
  unsigned char dummy=0;
  int n=::send(w_,(char*)&dummy,sizeof(dummy),0);
  if(n<0)
  {
    int err=WSAGetLastError();
    errno=err;
  }
  assert(n==sizeof(dummy)); 
#endif
}

void base::ezSignaler::recv()
{
#if defined __linux__
#if defined ZMQ_HAVE_EVENTFD
  uint64_t dummy;
  ssize_t sz=read(r_,&dummy,sizeof (dummy));
  if (dummy==2) 
  {
    const uint64_t inc=1;
    ssize_t sz=write(w_,&inc,sizeof(inc));
    assert(sz==sizeof (inc));
    return;
  }
  //assert(dummy == 1);
#else
  unsigned char dummy;
  ssize_t nbytes=::recv(r_,&dummy,sizeof(dummy),0);
//   assert(nbytes >= 0);
//   assert(dummy==0);
#endif
#else
  unsigned char dummy=0;
  int nbytes=::recv(r_,(char*)&dummy,sizeof(dummy),0);
  if(nbytes<0)
  {
    int err=WSAGetLastError();
    errno=err;
    if(errno==WSAEWOULDBLOCK)
      nbytes=1;
  }
  assert(nbytes!=SOCKET_ERROR);
  assert(dummy==0);
#endif
}

int base::ezSignaler::wait(int tms)
{
  fd_set fds;
  FD_ZERO (&fds);
  FD_SET (r_, &fds);
  struct timeval timeout;
  if (tms >= 0) {
    timeout.tv_sec = tms / 1000;
    timeout.tv_usec = tms % 1000 * 1000;
  }
#ifdef __linux__
  int rc = select (r_ + 1, &fds, NULL, NULL,
    tms >= 0 ? &timeout : NULL);
  if (rc < 0) {
    return -1;
  }
#else

  int rc = select (0, &fds, NULL, NULL,
    tms >= 0 ? &timeout : NULL);
  assert (rc != SOCKET_ERROR);
  if (rc == 0) {
    return -1;
  }
  assert (rc == 1);
  return 0;
#endif
  return 1;
}

base::ezSignaler::ezSignaler()
{
  MakeSocketPair(r_,w_);
  UnblockSocket(r_);
  UnblockSocket(w_);
}

base::ezSignaler::~ezSignaler()
{
#ifdef __linux__
  close(r_);
#if !defined HAVE_EVENTFD
  close(w_);
#endif
#else
  _close(r_);
  _close(w_);
#endif
}
