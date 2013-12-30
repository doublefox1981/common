#include "../base/portable.h"
#include "../base/memorystream.h"
#include "../base/logging.h"
#include "../base/util.h"
#include "../base/eztime.h"
#include "fd.h"
#include "poller.h"
#include "event.h"
#include "socket.h"
#include "netpack.h"
#include "connection.h"
#include "iothread.h"
#include <algorithm>

bool net::ezSelectPoller::WillDelete(const ezSelectFdEntry& entry)
{
  return entry.fd_==INVALID_SOCKET;
}

void net::ezSelectPoller::Poll()
{
  int64_t timeout=timer_.InvokeTimer();
  struct timeval tm={(long)(timeout/1000),(long)(timeout%1000*1000)};
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
  else if(retval<0)
  {
#ifndef __linux__
    errno=wsa_error_to_errno(WSAGetLastError());
#endif
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
  load_.Inc();
  return true;
}

void net::ezSelectPoller::DelFd(int fd)
{
  for (auto iter=fdarray_.begin ();iter!=fdarray_.end ();++iter)
  {
    if (iter->fd_==fd)
    {
      load_.Dec();
      willdelfd_=true;
      iter->fd_=INVALID_SOCKET;
      break;
    }
  }

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

void net::ezSelectPoller::AddTimer(int64_t timeout,ezPollerEventHander* hander,int32_t timerid)
{
  timer_.AddTimer(timeout,hander,timerid);
}

void net::ezSelectPoller::DelTimer(ezPollerEventHander* hander,int32_t timerid)
{
  timer_.DelTimer(hander,timerid);
}

#ifdef __linux__

net::ezEpollPoller::ezEpollPoller():willdelfd_(false)
{
  epollfd_=epoll_create1(0);
}

net::ezEpollPoller::~ezEpollPoller()
{
  close(epollfd_);
  // fdarray_ clear
}

bool net::ezEpollPoller::AddFd(int fd,ezPollerEventHander* hander)
{
  assert(fd>0);
  if(fdarray_.size()<=fd)
    fdarray_.resize(fd*2+1);
  if(fdarray_[fd])
    delete fdarray_[fd];
  ezEpollFdEntry* entry=new ezEpollFdEntry;
  entry->fd_=fd;
  entry->hander_=hander;
  entry->event_=0;
  entry->event_|=EPOLLERR;
  entry->event_|=EPOLLHUP;
  fdarray_[fd]=entry;

  struct epoll_event ee;
  ee.events=entry->event_;
  ee.data.ptr=entry;
  int rc=epoll_ctl(epollfd_,EPOLL_CTL_ADD,fd,&ee);
  assert(rc!=-1);
  load_.Inc();
  return true;
}

void net::ezEpollPoller::DelFd(int fd)
{
  assert(fd>0&&fd<fdarray_.size());
  if(fd<=0||fd>=fdarray_.size())
    return;
  ezEpollFdEntry* entry=fdarray_[fd];
  if(!entry||entry->fd_==INVALID_SOCKET)
    return;
  willdelfd_=true;
  delarray_.push_back(entry->fd_);
  entry->fd_=INVALID_SOCKET;
  struct epoll_event ee;
  ee.events=entry->event_;
  ee.data.ptr=entry;
  int rc=epoll_ctl(epollfd_,EPOLL_CTL_DEL,fd,&ee);
  assert(rc!=-1);
  load_.Dec();
}

void net::ezEpollPoller::SetPollIn(int fd)
{
  assert(fd>0&&fd<fdarray_.size());
  if(fd<=0||fd>=fdarray_.size())
    return;
  ezEpollFdEntry* entry=fdarray_[fd];
  if(!entry||entry->fd_==INVALID_SOCKET)
    return;
  if(entry->event_&EPOLLIN)
    return;
  entry->event_|=EPOLLIN;
  struct epoll_event ee;
  ee.events=entry->event_;
  ee.data.ptr=entry;
  int rc=epoll_ctl(epollfd_,EPOLL_CTL_MOD,fd,&ee);
  assert(rc!=-1);
}

void net::ezEpollPoller::ResetPollIn( int fd )
{
  assert(fd>0&&fd<fdarray_.size());
  if(fd<=0||fd>=fdarray_.size())
    return;
  ezEpollFdEntry* entry=fdarray_[fd];
  if(!entry||entry->fd_==INVALID_SOCKET)
    return;
  if(entry->event_&EPOLLIN)
  {
    entry->event_&=(~EPOLLIN);
    struct epoll_event ee;
    ee.events=entry->event_;
    ee.data.ptr=entry;
    int rc=epoll_ctl(epollfd_,EPOLL_CTL_MOD,fd,&ee);
    assert(rc!=-1);
  }
}

void net::ezEpollPoller::SetPollOut( int fd )
{
  assert(fd>0&&fd<fdarray_.size());
  if(fd<=0||fd>=fdarray_.size())
    return;
  ezEpollFdEntry* entry=fdarray_[fd];
  if(!entry||entry->fd_==INVALID_SOCKET)
    return;
  if(entry->event_&EPOLLOUT)
    return;
  entry->event_|=EPOLLOUT;
  struct epoll_event ee;
  ee.events=entry->event_;
  ee.data.ptr=entry;
  int rc=epoll_ctl(epollfd_,EPOLL_CTL_MOD,fd,&ee);
  assert(rc!=-1);
}

void net::ezEpollPoller::ResetPollOut( int fd )
{
  assert(fd>0&&fd<fdarray_.size());
  if(fd<=0||fd>=fdarray_.size())
    return;
  ezEpollFdEntry* entry=fdarray_[fd];
  if(!entry||entry->fd_==INVALID_SOCKET)
    return;
  if(entry->event_&EPOLLOUT)
  {
    entry->event_&=(~EPOLLOUT);
    struct epoll_event ee;
    ee.events=entry->event_;
    ee.data.ptr=entry;
    int rc=epoll_ctl(epollfd_,EPOLL_CTL_MOD,fd,&ee);
    assert(rc!=-1);
  }
}

void net::ezEpollPoller::Poll()
{
  int timeout=(int)(timer_.InvokeTimer());
  int retval=0;
  retval = epoll_wait(epollfd_,epollevents_,sizeof(epollevents_)/sizeof(struct epoll_event),timeout>0?timeout:-1);
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
    for(size_t s=0;s<delarray_.size();++s)
    {
      delete fdarray_[delarray_[s]];
      fdarray_[delarray_[s]]=nullptr;
    }
    delarray_.clear();
    willdelfd_=false;
  }
}

void net::ezEpollPoller::AddTimer(int64_t timeout,ezPollerEventHander* hander,int32_t timerid)
{
  timer_.AddTimer(timeout,hander,timerid);
}

void net::ezEpollPoller::DelTimer(ezPollerEventHander* hander,int32_t timerid)
{
  timer_.DelTimer(hander,timerid);
}

#endif

void net::ezPollTimer::AddTimer(int64_t timeout,ezPollerEventHander* hander,int32_t timerid)
{
  ezTimerEntry entry={timerid,hander};
  timers_.insert(std::multimap<int64_t,ezTimerEntry>::value_type(timeout+base::ezNowTick(),entry));
}

void net::ezPollTimer::DelTimer(ezPollerEventHander* hander,int32_t timerid)
{
  for(auto iter=timers_.begin();iter!=timers_.end();)
  {
    ezTimerEntry& entry=iter->second;
    if(entry.hander_==hander&&entry.timerid_==timerid)
      iter=timers_.erase(iter);
    else
      ++iter;
  }
}

int64_t net::ezPollTimer::InvokeTimer()
{
  if(timers_.empty())
    return 0;
  int64_t cur=base::ezNowTick();
  for(auto iter=timers_.begin();iter!=timers_.end();)
  {
    ezTimerEntry& entry=iter->second;
    if(iter->first>cur)
      return iter->first-cur;
    entry.hander_->HandleTimer();
    iter=timers_.erase(iter);
  }
  return 0;
}

net::ezPoller* net::CreatePoller()
{
#ifdef __linux__
  return new ezEpollPoller;
#else
  return new ezSelectPoller;
#endif
}