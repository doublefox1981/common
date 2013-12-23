#ifndef _NET_IOTHREAD_H
#define _NET_IOTHREAD_H
#include "../base/notifyqueue.h"
#include "event.h"
#include "fd.h"
#include "poller.h"
#include "socket.h"

namespace net{
  class ezPoller;
  typedef base::ezNotifyQueue<ezCrossEventData> CrossEvQueue;
  typedef moodycamel::ReaderWriterQueue<ezMsgWarper> MsgWarperQueue;
  class ezIoThread:public ezFd,public base::Threads,public ezThreadEventHander
  {
  public:
    ezIoThread();
    virtual ~ezIoThread();
    ThreadEvQueue* getevqueue() {return evqueue_;}
    int getload(){return load_;}
    uint64_t add(int fd,uint64_t uuid,ezFd *ezfd,int event);
    int del(int fd);
    int mod(int fd,int event,bool set);
    ezFdData* getfd(int i);
    int getmax(){return maxfd_;}
    void pushfired(ezFdData* fd);
    virtual void OnEvent(ezIoThread* io,int fd,int event,uint64_t uuid);
    virtual void ProcessEvent(ezThreadEvent& ev);
    virtual void Run();
  private:
    int                     load_;
    int                     maxfd_;
    ezPoller                poller_;
    ThreadEvQueue*          evqueue_;
    MsgWarperQueue*         msgqueue_;
    std::vector<ezFdData*>  fds_;        
    std::vector<ezFdData*>  firedfd_;
  };
}
#endif