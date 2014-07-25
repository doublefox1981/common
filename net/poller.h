#ifndef _POLLER_H
#define _POLLER_H
#include <queue>
#include <unordered_map>

namespace net
{
  class IPollerEventHander
  {
  public:
    virtual ~IPollerEventHander(){}
    virtual void HandleInEvent()=0;
    virtual void HandleOutEvent()=0;
    virtual void HandleTimer()=0;
  };

  class Poller
  {
  public:
    virtual ~Poller(){}
    virtual void AddTimer(int64_t timeout,IPollerEventHander* hander)=0;
    virtual void DelTimer(IPollerEventHander* hander)=0;
    virtual bool AddFd(int fd,IPollerEventHander* hander)=0;
    virtual void DelFd(int fd)=0;
    virtual void SetPollIn(int fd)=0;
    virtual void ResetPollIn(int fd)=0;
    virtual void SetPollOut(int fd)=0;
    virtual void ResetPollOut(int fd)=0;
    virtual void Poll()=0;
    virtual long GetLoad()=0;
  };

  struct PollTimerEntry
  {
    int64_t             fired_time_;
    IPollerEventHander* hander_;
    PollTimerEntry():fired_time_(0),hander_(NULL){}
  };
  struct PollCmp
  {
    bool operator()(const PollTimerEntry* a,const PollTimerEntry* b)
    {
      return a->fired_time_>b->fired_time_;
    }
  };
  typedef std::priority_queue<PollTimerEntry*,std::vector<PollTimerEntry*>,PollCmp> POLL_TIMER_HEAP;
  typedef std::unordered_map<IPollerEventHander*,PollTimerEntry*> POLL_TIMER_MAP;

  class PollTimer
  {
  public:
    void AddTimer(IPollerEventHander* hander,int64_t timeout);
    void DelTimer(IPollerEventHander* hander);
    int64_t InvokeTimer();
  private:
    POLL_TIMER_HEAP heap_;
    POLL_TIMER_MAP  map_;
  };

  class SelectPoller:public Poller
  {
  public:
    SelectPoller();
    virtual ~SelectPoller(){}
    virtual void AddTimer(int64_t timeout,IPollerEventHander* hander);
    virtual void DelTimer(IPollerEventHander* hander);
    virtual bool AddFd(int fd,IPollerEventHander* hander);
    virtual void DelFd(int fd);
    virtual void SetPollIn(int fd);
    virtual void ResetPollIn(int fd);
    virtual void SetPollOut(int fd);
    virtual void ResetPollOut(int fd);
    virtual void Poll();
    virtual long  GetLoad(){return load_.Get();}
  private:
    struct SelectFdEntry
    {
      int fd_;
      IPollerEventHander* hander_;
    };
    static bool WillDelete(const SelectFdEntry& entry);
  private:
    std::vector<SelectFdEntry> fdarray_;
    PollTimer timer_;
    fd_set wfds_;
    fd_set uwfds_;
    fd_set rfds_;
    fd_set urfds_;
    fd_set efds_;
    fd_set uefds_;
    int    maxfd_;
    bool   willdelfd_;
    base::AtomicNumber load_;
  };
  Poller*   CreatePoller();
#ifdef __linux__
#include <sys/epoll.h>
#include <unistd.h>
  class EpollPoller:public Poller
  {
  public:
    EpollPoller();
    virtual ~EpollPoller();

    virtual void AddTimer(int64_t timeout,IPollerEventHander* hander);
    virtual void DelTimer(IPollerEventHander* hander);
    virtual bool AddFd(int fd,IPollerEventHander* hander);
    virtual void DelFd(int fd);
    virtual void SetPollIn(int fd);
    virtual void ResetPollIn(int fd);
    virtual void SetPollOut(int fd);
    virtual void ResetPollOut(int fd);
    virtual void Poll();
    virtual long  GetLoad(){return load_.Get();}
  private:
    struct EpollFdEntry
    {
      int fd_;
      int event_;
      IPollerEventHander* hander_;
    };
    std::vector<EpollFdEntry*> fdarray_;
    std::vector<int> delarray_;
    bool willdelfd_;
    PollTimer timer_;
  private:
    int epollfd_;
    struct epoll_event epollevents_[1024];
    base::AtomicNumber   load_;
  };
#endif
}
#endif