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

bool net::SelectPoller::will_delete(const SelectFdEntry& entry)
{
  return entry.fd_==INVALID_SOCKET;
}

void net::SelectPoller::poll()
{
  int64_t timeout=timer_.invoke_timer();
  struct timeval tm={(long)(timeout/1000),(long)(timeout%1000*1000)};
  memcpy(&urfds_,&rfds_,sizeof(fd_set));
  memcpy(&uwfds_,&wfds_,sizeof(fd_set));
  memcpy(&uefds_,&efds_,sizeof(fd_set));
  int retval=select(maxfd_+1,&urfds_,&uwfds_,&uefds_,&tm);
  if(retval>0)
  {
    for(size_t j=0;j<fdarray_.size();++j)
    {
      SelectFdEntry& entry=fdarray_[j];
      if(entry.fd_==INVALID_SOCKET)
        continue;
      if(FD_ISSET(entry.fd_,&urfds_))
        entry.hander_->handle_in_event();
      if(FD_ISSET(entry.fd_,&uwfds_))
        entry.hander_->handle_out_event();
      if(FD_ISSET(entry.fd_,&uefds_))
        entry.hander_->handle_in_event();
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
    fdarray_.erase(std::remove_if(fdarray_.begin(),fdarray_.end(),SelectPoller::will_delete),fdarray_.end());
    willdelfd_ = false;
  }
}

net::SelectPoller::SelectPoller()
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

bool net::SelectPoller::add_fd(int fd,IPollerEventHander* hander)
{
  if(fdarray_.size()>=FD_SETSIZE)
    return false;
  SelectFdEntry entry={fd,hander};
  fdarray_.push_back(entry);
  FD_SET(fd,&efds_);
  if(fd>maxfd_)
    maxfd_=fd;
  load_.Inc();
  return true;
}

void net::SelectPoller::del_fd(int fd)
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

void net::SelectPoller::set_poll_in(int fd)
{
  FD_SET(fd,&rfds_);
}

void net::SelectPoller::reset_poll_in(int fd)
{
  FD_CLR(fd,&rfds_);
}

void net::SelectPoller::set_poll_out(int fd)
{
  FD_SET(fd,&wfds_);
}

void net::SelectPoller::reset_poll_out(int fd)
{
  FD_CLR(fd,&uwfds_);
}

void net::SelectPoller::add_timer(int64_t timeout,IPollerEventHander* hander)
{
  timer_.add_timer(hander,timeout);
}

void net::SelectPoller::del_timer(IPollerEventHander* hander)
{
  timer_.del_timer(hander);
}

#ifdef __linux__
net::EpollPoller::EpollPoller():willdelfd_(false)
{
  epollfd_=epoll_create1(0);
}

net::EpollPoller::~EpollPoller()
{
  close(epollfd_);
  // fdarray_ clear
}

bool net::EpollPoller::add_fd(int fd,IPollerEventHander* hander)
{
  assert(fd>0);
  if(fdarray_.size()<=fd)
    fdarray_.resize(fd*2+1);
  if(fdarray_[fd])
    delete fdarray_[fd];
  EpollFdEntry* entry=new EpollFdEntry;
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

void net::EpollPoller::del_fd(int fd)
{
  assert(fd>0&&fd<fdarray_.size());
  if(fd<=0||fd>=fdarray_.size())
    return;
  EpollFdEntry* entry=fdarray_[fd];
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

void net::EpollPoller::set_poll_in(int fd)
{
  assert(fd>0&&fd<fdarray_.size());
  if(fd<=0||fd>=fdarray_.size())
    return;
  EpollFdEntry* entry=fdarray_[fd];
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

void net::EpollPoller::reset_poll_in( int fd )
{
  assert(fd>0&&fd<fdarray_.size());
  if(fd<=0||fd>=fdarray_.size())
    return;
  EpollFdEntry* entry=fdarray_[fd];
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

void net::EpollPoller::set_poll_out( int fd )
{
  assert(fd>0&&fd<fdarray_.size());
  if(fd<=0||fd>=fdarray_.size())
    return;
  EpollFdEntry* entry=fdarray_[fd];
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

void net::EpollPoller::reset_poll_out( int fd )
{
  assert(fd>0&&fd<fdarray_.size());
  if(fd<=0||fd>=fdarray_.size())
    return;
  EpollFdEntry* entry=fdarray_[fd];
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

void net::EpollPoller::poll()
{
  int timeout=(int)(timer_.invoke_timer());
  int retval=0;
  retval = epoll_wait(epollfd_,epollevents_,sizeof(epollevents_)/sizeof(struct epoll_event),timeout>0?timeout:100);
  if(retval<0&&errno!=EINTR)
  {
    char err[256];
    strerror_r(errno,err,sizeof(err));
    LOG_INFO("epoll_wait return errno=%d,'%s'",errno,err);
    return;
  }
  for (int j=0;j<retval;j++) 
  {
    struct epoll_event *e = &epollevents_[j];
    EpollFdEntry* entry=(EpollFdEntry*)(e->data.ptr);
    if(entry->fd_==INVALID_SOCKET)
      continue;
    if(e->events&(EPOLLERR|EPOLLHUP))
      entry->hander_->handle_in_event();
    if(entry->fd_==INVALID_SOCKET)
      continue;
    if(e->events&EPOLLOUT)
      entry->hander_->handle_out_event();
    if(entry->fd_==INVALID_SOCKET)
      continue;
    if(e->events&EPOLLIN)
      entry->hander_->handle_in_event();
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

void net::EpollPoller::add_timer(int64_t timeout,IPollerEventHander* hander)
{
  timer_.add_timer(hander,timeout);
}

void net::EpollPoller::del_timer(IPollerEventHander* hander)
{
  timer_.del_timer(hander,timerid);
}
#endif

void net::PollTimer::add_timer(IPollerEventHander* hander,int64_t timeout)
{
  if(map_.find(hander)!=map_.end())
    return;
  PollTimerEntry* entry=new PollTimerEntry;
  entry->fired_time_=base::now_tick()+timeout;
  entry->hander_=hander;
  heap_.push(entry);
  map_[hander]=entry;
}

void net::PollTimer::del_timer(IPollerEventHander* hander)
{
  auto iter=map_.find(hander);
  if(iter!=map_.end())
    iter->second->hander_=NULL;
}

int64_t net::PollTimer::invoke_timer()
{
  if(heap_.empty())
    return 0;
  int64_t cur=base::now_tick();
  while(!heap_.empty())
  {
    PollTimerEntry* entry=heap_.top();
    if(entry->fired_time_>cur)
      return entry->fired_time_-cur;
    if(entry->hander_)
      entry->hander_->handle_timer();
    heap_.pop();
    auto iter=map_.find(entry->hander_);
    if(iter!=map_.end())
      map_.erase(iter);
  }
  return 0;
}

net::Poller* net::create_poller()
{
#ifdef __linux__
  return new EpollPoller;
#else
  return new SelectPoller;
#endif
}