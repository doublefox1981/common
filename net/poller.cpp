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

bool net::ezSelectPoller::WillDelete(const ezSelectFdEntry& entry)
{
  return entry.fd_==INVALID_SOCKET;
}

void net::ezSelectPoller::Poll()
{
  struct timeval tm = {0,0};
  memcpy(&urfds_,&rfds_,sizeof(fd_set));
  memcpy(&uwfds_,&wfds_,sizeof(fd_set));
  memcpy(&uefds_,&efds_,sizeof(fd_set));
  int retval=select(maxfd_+1,&urfds_,&uwfds_,&uefds_,&tm);
  if(retval>0)
  {
    for(size_t j=0;j<fdarray_.size();++j)
    {
      ezSelectFdEntry& entry=fdarray_[j];
      if(entry.fd_==INVALID_SOCKET)
        continue;
      if(FD_ISSET(entry.fd_,&urfds_))
        entry.hander_->HandleInEvent();
      if(FD_ISSET(entry.fd_,&uwfds_))
        entry.hander_->HandleOutEvent();
      if(FD_ISSET(entry.fd_,&uefds_))
        entry.hander_->HandleInEvent();
    }
  }
  if(willdelfd_)
  {
    fdarray_.erase(std::remove_if(fdarray_.begin(),fdarray_.end(),ezSelectPoller::WillDelete),fdarray_.end());
    willdelfd_ = false;
  }
}

net::ezSelectPoller::ezSelectPoller()
{
  FD_ZERO(&rfds_);
  FD_ZERO(&wfds_);
  FD_ZERO(&urfds_);
  FD_ZERO(&uwfds_);
  FD_ZERO(&efds_);
  FD_ZERO(&uefds_);
  maxfd_=-1;
  willdelfd_=false;
}

bool net::ezSelectPoller::AddFd(int fd,ezPollerEventHander* hander)
{
  if(fdarray_.size()>=FD_SETSIZE)
    return false;
  ezSelectFdEntry entry={fd,hander};
  fdarray_.push_back(entry);
  FD_SET(fd,&efds_);
  if(fd>maxfd_)
    maxfd_=fd;
  return true;
}

void net::ezSelectPoller::DelFd(int fd)
{
  bool found=false;
  for (auto iter=fdarray_.begin ();iter!=fdarray_.end ();++iter)
  {
    if (iter->fd_==fd)
    {
      found=true;
      iter->fd_=INVALID_SOCKET;
      break;
    }
  }
  if(!found)
    return;
  willdelfd_=true;

  FD_CLR(fd,&rfds_);
  FD_CLR(fd,&wfds_);
  FD_CLR(fd,&efds_);

  FD_CLR(fd,&urfds_);
  FD_CLR(fd,&uwfds_);
  FD_CLR(fd,&uefds_);

  if (fd==maxfd_) 
  {
    maxfd_=INVALID_SOCKET;
    for (auto it=fdarray_.begin();it!=fdarray_.end ();++it)
    {
      if(it->fd_>maxfd_)
        maxfd_=it->fd_;
    }
  }
}

void net::ezSelectPoller::SetPollIn(int fd)
{
  FD_SET(fd,&rfds_);
}

void net::ezSelectPoller::ResetPollIn(int fd)
{
  FD_CLR(fd,&rfds_);
}

void net::ezSelectPoller::SetPollOut(int fd)
{
  FD_SET(fd,&wfds_);
}

void net::ezSelectPoller::ResetPollOut(int fd)
{
  FD_CLR(fd,&uwfds_);
}

net::ezPoller* net::CreatePoller()
{
  return new ezSelectPoller;
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