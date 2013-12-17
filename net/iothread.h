#ifndef _NET_IOTHREAD_H
#define _NET_IOTHREAD_H
#include "../base/notifyqueue.h"

namespace net{
  class ezPoller;
  typedef base::ezNotifyQueue<ezCrossEventData> CrossEvQueue;
  class ezIoThread:public ezFd,public base::Threads
  {
  public:
    ezIoThread();
    virtual ~ezIoThread();
    void setlooper(ezEventLoop* loop){looper_=loop;}
    ezEventLoop* getlooper(){return looper_;}
    void settid(int tid);
    int  gettid(){return tid_;}
    void notify(ezCrossEventData& ev);
    uint64_t add(int fd,uint64_t uuid,ezFd *ezfd,int event);
    int del(int fd);
    int mod(int fd,int event,bool set);
    ezFdData* getfd(int i);
    int getmax(){return maxfd_;}
    void pushfired(ezFdData* fd);
    void pushmain(ezCrossEventData& data);
    bool mainpull(ezCrossEventData& data);
    virtual void onEvent(ezIoThread* io,int fd,int event,uint64_t uuid);
    virtual void sendMsg(ezMsg& blk){}
    virtual size_t formatMsg(){return 0;}
    virtual void Run();
  private:
    int                     maxfd_;
    int                     tid_;
    ezPoller*               poller_;
    ezEventLoop*            looper_;
    CrossEvQueue*           ioqueue_;
    CrossEvQueue*           mainqueue_;
    std::vector<ezFdData*>  fds_;        // all fd
    std::vector<ezFdData*>  firedfd_;    // fd that fired event
  };
}
#endif