#ifndef _POLLER_H
#define _POLLER_H

namespace net
{
  class ezPollerEventHander
  {
  public:
    virtual ~ezPollerEventHander(){}
    virtual void HandleInEvent()=0;
    virtual void HandleOutEvent()=0;
  };

  class ezPoller
  {
  public:
    virtual ~ezPoller(){}
    virtual bool AddFd(int fd,ezPollerEventHander* hander)=0;
    virtual void DelFd(int fd)=0;
    virtual void SetPollIn(int fd)=0;
    virtual void ResetPollIn(int fd)=0;
    virtual void SetPollOut(int fd)=0;
    virtual void ResetPollOut(int fd)=0;
    virtual void Poll()=0;
  };

  class ezSelectPoller:public ezPoller
  {
  public:
    ezSelectPoller();
    virtual ~ezSelectPoller(){}
    virtual bool AddFd(int fd,ezPollerEventHander* hander);
    virtual void DelFd(int fd);
    virtual void SetPollIn(int fd);
    virtual void ResetPollIn(int fd);
    virtual void SetPollOut(int fd);
    virtual void ResetPollOut(int fd);
    virtual void Poll();
  private:
    struct ezSelectFdEntry
    {
      int fd_;
      ezPollerEventHander* hander_;
    };
    static bool WillDelete(const ezSelectFdEntry& entry);
  private:
    std::vector<ezSelectFdEntry> fdarray_;
    fd_set wfds_;
    fd_set uwfds_;
    fd_set rfds_;
    fd_set urfds_;
    fd_set efds_;
    fd_set uefds_;
    int    maxfd_;
    bool   willdelfd_;
  };
  ezPoller* CreatePoller();
#ifdef __linux__
#include <sys/epoll.h>
#include <unistd.h>
  class ezEpollPoller:public ezPoller
  {
  public:
    ezEpollPoller();
    virtual ~ezEpollPoller();
    virtual bool AddFd(int fd,ezPollerEventHander* hander);
    virtual void DelFd(int fd);
    virtual void SetPollIn(int fd);
    virtual void ResetPollIn(int fd);
    virtual void SetPollOut(int fd);
    virtual void ResetPollOut(int fd);
    virtual void Poll();
  private:
    struct ezEpollFdEntry
    {
      int fd_;
      int event_;
      ezPollerEventHander* hander_;
    };
    std::vector<ezEpollFdEntry*> fdarray_;
    std::vector<int> delarray_;
    bool willdelfd_;
  private:
    int epollfd_;
    struct epoll_event epollevents_[1024];
  };
#endif
}
#endif