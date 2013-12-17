#include "../base/portable.h"
#include "../base/memorystream.h"
#include "../base/logging.h"
#include "../base/util.h"
#include "fd.h"
#include "poller.h"
#include "event.h"
#include "socket.h"
#include "netpack.h"
#include "connection.h"
#include "iothread.h"

void net::ezSelectPoller::addFd(int fd,int mask)
{
  if(mask&ezNetRead)
    FD_SET(fd,&rfds_);
  if(mask&ezNetWrite)
    FD_SET(fd,&wfds_);
}

void net::ezSelectPoller::delFd(int fd,int mask)
{
  if(mask&ezNetRead)
    FD_CLR(fd,&rfds_);
  if(mask&ezNetWrite)
    FD_CLR(fd,&wfds_);
}

void net::ezSelectPoller::modFd(int fd,int mask,int srcmask,bool set)
{
  if(!set)
    delFd(fd,mask);
  else
    addFd(fd,mask);
}

void net::ezSelectPoller::poll()
{
  net::ezIoThread* thread=getBindThread();
  struct timeval tm = {0,0};
  memcpy(&urfds_,&rfds_,sizeof(fd_set));
  memcpy(&uwfds_,&wfds_,sizeof(fd_set));
  int retval=select(thread->getmax()+1,&urfds_,&uwfds_,nullptr,&tm);
  if(retval>0)
  {
    for(int j=0;j<=thread->getmax();++j)
    {
      int mask=0;
      ezFdData* d=thread->getfd(j);
      if(!d||d->event_==ezNetNone) continue;
      if(d->event_&ezNetRead&&FD_ISSET(d->fd_,&urfds_))
        mask|=ezNetRead;
      if(d->event_&ezNetWrite&&FD_ISSET(d->fd_,&uwfds_))
        mask|=ezNetWrite;
      ezFdData* fireD=new ezFdData;
      fireD->event_=mask;
      fireD->fd_=d->fd_;
      fireD->uuid_=d->uuid_;
      thread->pushfired(fireD);
    }
  }
}

net::ezSelectPoller::ezSelectPoller(net::ezIoThread* io):ezPoller(io)
{
  FD_ZERO(&rfds_);
  FD_ZERO(&wfds_);
  FD_ZERO(&urfds_);
  FD_ZERO(&uwfds_);
}

#ifdef __linux__
net::ezEpollPoller::ezEpollPoller(ezIoThread* io):ezPoller(io)
{
  epollFd_=epoll_create1(0);
}

net::ezEpollPoller::~ezEpollPoller()
{
  close(epollFd_);
}

void net::ezEpollPoller::addFd(int fd,int mask)
{
  struct epoll_event ee;
  ee.events|=EPOLLERR;
  ee.events|=EPOLLHUP;
  if(mask&ezNetRead) ee.events|=EPOLLIN;
  if(mask&ezNetWrite) ee.events|=EPOLLOUT;
  ee.data.u64=0;
  ee.data.fd=fd;
  if(epoll_ctl(epollFd_,EPOLL_CTL_ADD,fd,&ee)==-1) 
    LOG_ERROR("EPOLL_CTL_ADD fail");
}

void net::ezEpollPoller::delFd(int fd,int mask)
{
  struct epoll_event ee;
  ee.events=0;
  if(mask&ezNetRead) ee.events|=EPOLLIN;
  if(mask&ezNetWrite) ee.events|=EPOLLOUT;
  ee.data.u64=0;
  ee.data.fd=fd;
  if(epoll_ctl(epollFd_,EPOLL_CTL_DEL,fd,&ee)==-1) 
    LOG_ERROR("EPOLL_CTL_DEL fail");
}

void net::ezEpollPoller::modFd(int fd,int mask,int srcmask,bool set)
{
  int dstmask=mask|srcmask;
  if(!set)
    dstmask&=(~mask);
  struct epoll_event ee;
  ee.events=0;
  ee.events|=EPOLLERR;
  ee.events|=EPOLLHUP;
  if(dstmask&ezNetRead) ee.events|=EPOLLIN;
  if(dstmask&ezNetWrite) ee.events|=EPOLLOUT;
  ee.data.u64 = 0;
  ee.data.fd = fd;
  if(epoll_ctl(epollFd_,EPOLL_CTL_MOD,fd,&ee)==-1) 
    LOG_ERROR("EPOLL_CTL_MOD fail");
}

void net::ezEpollPoller::poll()
{
  int retval=0;
  retval = epoll_wait(epollFd_,epollEvents_,sizeof(epollEvents_)/sizeof(struct epoll_event),5);
  for (int j=0;j<retval;j++) 
  {
    struct epoll_event *e = &epollEvents_[j];
    ezFdData* fdData=getEventLooper()->ezFdDatai(e->data.fd);
    assert(fdData);

    int mask = 0;
    if (e->events&EPOLLIN)  mask|=ezNetRead;
    if (e->events&EPOLLOUT) mask|=ezNetWrite;
    if (e->events&EPOLLERR) mask|=ezNetErr;
    if (e->events&EPOLLHUP) mask| ezNetErr;

    ezFdData* fireD=new ezFdData;
    fireD->event_=mask;
    fireD->fd_=e->data.fd;
    fireD->uuid_=fdData->uuid_;
    getEventLooper()->pushFired(fireD);
  }
}

#endif