#ifndef _POLLER_H
#define _POLLER_H
#include <map>

namespace net
{
  class ezPollerEventHander
  {
  public:
    virtual ~ezPollerEventHander(){}
    virtual void HandleInEvent()=0;
    virtual void HandleOutEvent()=0;
    virtual void HandleTimer()=0;
  };

  class ezPoller
  {
  public:
    virtual ~ezPoller(){}
    virtual void AddTimer(int64_t timeout,ezPollerEventHander* hander,int32_t timerid)=0;
    virtual void DelTimer(ezPollerEventHander* hander,int32_t timerid)=0;
    virtual bool AddFd(int fd,ezPollerEventHander* hander)=0;
    virtual void DelFd(int fd)=0;
    virtual void SetPollIn(int fd)=0;
    virtual void ResetPollIn(int fd)=0;
    virtual void SetPollOut(int fd)=0;
    virtual void ResetPollOut(int fd)=0;
    virtual void Poll()=0;
  };

  class ezPollTimer
  {
  public:
    void AddTimer(int64_t timeout,ezPollerEventHander* hander,int32_t timerid);
    void DelTimer(ezPollerEventHander* hander,int32_t timerid);
    int64_t InvokeTimer();
  private:
    struct ezTimerEntry
    {
      int32_t timerid_;
      ezPollerEventHander* hander_;
    };
    std::multimap<int64_t,ezTimerEntry> timers_;
  };

  class ezSelectPoller:public ezPoller
  {
  public:
    ezSelectPoller();
    virtual ~ezSelectPoller(){}
    virtual void AddTimer(int64_t timeout,ezPollerEventHander* hander,int32_t timerid);
    virtual void DelTimer(ezPollerEventHander* hander,int32_t timerid);
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
    ezPollTimer timer_;
    fd_set wfds_;
    fd_set uwfds_;
    fd_set rfds_;
    fd_set urfds_;
    fd_set efds_;
    fd_set uefds_;
    int    maxfd_;
    bool   willdelfd_;
  };

#ifdef __linux__
#include <sys/epoll.h>
#include <unistd.h>
  class ezEpollPoller:public ezPoller
  {
  public:
    ezEpollPoller();
    virtual ~ezEpollPoller();
    virtual void AddTimer(int64_t timeout,ezPollerEventHander* hander,int32_t timerid);
    virtual void DelTimer(ezPollerEventHander* hander,int32_t timerid);
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
    ezPollTimer timer_;
  private:
    int epollfd_;
    struct epoll_event epollevents_[1024];
  };
#endif
}
#endif