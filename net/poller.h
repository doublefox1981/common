#ifndef _POLLER_H
#define _POLLER_H

namespace net
{
  class ezIoThread;
  class ezPoller
  {
  public:
    ezPoller(ezIoThread* io):io_(io){}
    virtual ~ezPoller(){}
    virtual void addFd(int fd,int event)=0;
    virtual void delFd(int fd,int event)=0;
    virtual void modFd(int fd,int event,int srcevent,bool set)=0;
    virtual void poll()=0;
    ezIoThread* getBindThread(){return io_;}
  private:
    ezIoThread* io_;
  };

  class ezSelectPoller:public ezPoller
  {
  public:
    ezSelectPoller(ezIoThread* io);
    virtual void addFd(int fd,int mask);
    virtual void delFd(int fd,int mask);
    virtual void modFd(int fd,int mask,int srcmask,bool set);
    virtual void poll();
  private:
    fd_set wfds_;
    fd_set rfds_;
    fd_set uwfds_;
    fd_set urfds_;
  };

#ifdef __linux__
#include <sys/epoll.h>
#include <unistd.h>
  class ezEpollPoller:public ezPoller
  {
  public:
    ezEpollPoller(ezEventLoop* loop);
    virtual ~ezEpollPoller();
    virtual void addFd(int fd,int mask);
    virtual void delFd(int fd,int mask);
    virtual void modFd(int fd,int mask,int srcmask,bool set);
    virtual void poll();
  private:
    int epollFd_;
    struct epoll_event epollEvents_[1024];
  };
#endif
}
#endif