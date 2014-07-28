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
    virtual void handle_in_event()=0;
    virtual void handle_out_event()=0;
    virtual void handle_timer()=0;
  };

  class Poller
  {
  public:
    virtual ~Poller(){}
    virtual void add_timer(int64_t timeout,IPollerEventHander* hander)=0;
    virtual void del_timer(IPollerEventHander* hander)=0;
    virtual bool add_fd(int fd,IPollerEventHander* hander)=0;
    virtual void del_fd(int fd)=0;
    virtual void set_poll_in(int fd)=0;
    virtual void reset_poll_in(int fd)=0;
    virtual void set_poll_out(int fd)=0;
    virtual void reset_poll_out(int fd)=0;
    virtual void poll()=0;
    virtual long get_load()=0;
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
    void add_timer(IPollerEventHander* hander,int64_t timeout);
    void del_timer(IPollerEventHander* hander);
    int64_t invoke_timer();
  private:
    POLL_TIMER_HEAP heap_;
    POLL_TIMER_MAP  map_;
  };

  class SelectPoller:public Poller
  {
  public:
    SelectPoller();
    virtual ~SelectPoller(){}
    virtual void add_timer(int64_t timeout,IPollerEventHander* hander);
    virtual void del_timer(IPollerEventHander* hander);
    virtual bool add_fd(int fd,IPollerEventHander* hander);
    virtual void del_fd(int fd);
    virtual void set_poll_in(int fd);
    virtual void reset_poll_in(int fd);
    virtual void set_poll_out(int fd);
    virtual void reset_poll_out(int fd);
    virtual void poll();
    virtual long  get_load(){return load_.Get();}
  private:
    struct SelectFdEntry
    {
      int fd_;
      IPollerEventHander* hander_;
    };
    static bool will_delete(const SelectFdEntry& entry);
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
  Poller*   create_poller();
#ifdef __linux__
#include <sys/epoll.h>
#include <unistd.h>
  class EpollPoller:public Poller
  {
  public:
    EpollPoller();
    virtual ~EpollPoller();

    virtual void add_timer(int64_t timeout,IPollerEventHander* hander);
    virtual void del_timer(IPollerEventHander* hander);
    virtual bool add_fd(int fd,IPollerEventHander* hander);
    virtual void del_fd(int fd);
    virtual void set_poll_in(int fd);
    virtual void reset_poll_in(int fd);
    virtual void set_poll_out(int fd);
    virtual void reset_poll_out(int fd);
    virtual void poll();
    virtual long  get_load(){return load_.Get();}
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