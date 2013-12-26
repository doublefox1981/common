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

net::ezEpollPoller::ezEpollPoller():willdelfd_(false)
{
  epollfd_=epoll_create1(0);
}

net::ezEpollPoller::~ezEpollPoller()
{
  close(epollfd_);
}

bool net::ezEpollPoller::AddFd(int fd,ezPollerEventHander* hander)
{
  assert(fd>0);
  if(fdarray_.size()<=fd)
    fdarray_.resize(fd*2+1);
  if(fdarray_[f])
    delete fdarray_[f];
  ezEpollFdEntry* entry=new ezEpollFdEntry;
  entry->fd_=fd;
  entry->hander_=hander;
  entry->event_=0;
  entry->event_|=EPOLLERR;
  entry->event_|=EPOLLHUP;
  fdarray_[f]=entry;

  struct epoll_event ee;
  ee.events=entry->event_;
  ee.data.ptr=entry;
  int rc=epoll_ctl(epollFd_,EPOLL_CTL_ADD,fd,&ee);
  assert(rc!=-1);
  return true;
}

void net::ezEpollPoller::DelFd(int fd)
{
  assert(fd>0&&fd<fdarray_.size());
  if(fd<=0||fd>=fdarray_.size())
    return;
  ezEpollFdEntry* entry=fdarray_[fd];
  if(!entry)
    return;
  willdelfd_=true;
  delarray_.push_back(entry->fd_);
  entry->fd_=INVALID_SOCKET;
  struct epoll_event ee;
  ee.events=entry->event_;
  ee.data.ptr=entry;
  int rc=epoll_ctl(epollFd_,EPOLL_CTL_DEL,fd,&ee);
  assert(rc!=-1);
}

void net::ezEpollPoller::SetPollIn(int fd)
{
  assert(fd>0&&fd<fdarray_.size());
  if(fd<=0||fd>=fdarray_.size())
    return;
  ezEpollFdEntry* entry=fdarray_[fd];
  if(!entry)
    return;
  if(entry->event_&EPOLLIN)
    return;
  entry->event_|=EPOLLIN;
  struct epoll_event ee;
  ee.events=entry->event_;
  ee.data.ptr=entry;
  int rc=epoll_ctl(epollFd_,EPOLL_CTL_MOD,fd,&ee);
  assert(rc!=-1);
}

void net::ezEpollPoller::ResetPollIn( int fd )
{
  assert(fd>0&&fd<fdarray_.size());
  if(fd<=0||fd>=fdarray_.size())
    return;
  ezEpollFdEntry* entry=fdarray_[fd];
  if(!entry)
    return;
  if(entry->event_&EPOLLIN)
  {
    entry->event_&=(~EPOLLIN);
    struct epoll_event ee;
    ee.events=entry->event_;
    ee.data.ptr=entry;
    int rc=epoll_ctl(epollFd_,EPOLL_CTL_MOD,fd,&ee);
    assert(rc!=-1);
  }
}

void net::ezEpollPoller::SetPollOut( int fd )
{
  assert(fd>0&&fd<fdarray_.size());
  if(fd<=0||fd>=fdarray_.size())
    return;
  ezEpollFdEntry* entry=fdarray_[fd];
  if(!entry)
    return;
  if(entry->event_&EPOLLOUT)
    return;
  entry->event_|=EPOLLOUT;
  struct epoll_event ee;
  ee.events=entry->event_;
  ee.data.ptr=entry;
  int rc=epoll_ctl(epollFd_,EPOLL_CTL_MOD,fd,&ee);
  assert(rc!=-1);
}

void net::ezEpollPoller::ResetPollOut( int fd )
{
  assert(fd>0&&fd<fdarray_.size());
  if(fd<=0||fd>=fdarray_.size())
    return;
  ezEpollFdEntry* entry=fdarray_[fd];
  if(!entry)
    return;
  if(entry->event_&EPOLLOUT)
  {
    entry->event_&=(~EPOLLOUT);
    struct epoll_event ee;
    ee.events=entry->event_;
    ee.data.ptr=entry;
    int rc=epoll_ctl(epollFd_,EPOLL_CTL_MOD,fd,&ee);
    assert(rc!=-1);
  }
}

void net::ezEpollPoller::Poll()
{
  int retval=0;
  retval = epoll_wait(epollfd_,epollevents_,sizeof(epollevents_)/sizeof(struct epoll_event),1);
  for (int j=0;j<retval;j++) 
  {
    struct epoll_event *e = &epollevents_[j];
    ezEpollFdEntry* entry=(ezEpollFdEntry*)(e->data.ptr);
    if(entry->fd_==INVALID_SOCKET)
      continue;
    if(e->events&(EPOLLERR|EPOLLHUP))
      entry->hander_->HandleInEvent();
    if(entry->fd_==INVALID_SOCKET)
      continue;
    if(e->events&EPOLLOUT)
      entry->hander_->HandleOutEvent();
    if(entry->fd_==INVALID_SOCKET)
      continue;
    if(e->events&EPOLLIN)
      entry->hander_->HandleInEvent();
  }
  if(willdelfd_)
  {
    for(size_t s=0;s<delarray_.size()++s)
    {
      delete fdarray_[delarray_[s]];
      fdarray_[delarray_[s]]=nullptr;
    }
    delarray_.clear();
    willdelfd_=false;
  }
}

#endif