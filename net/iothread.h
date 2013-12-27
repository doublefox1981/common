#ifndef _NET_IOTHREAD_H
#define _NET_IOTHREAD_H
#include "../base/notifyqueue.h"
#include "event.h"
#include "fd.h"
#include "poller.h"
#include "socket.h"

namespace net{
  class ezPoller;
  class ezIoThread:public ezPollerEventHander,public base::Threads,public ezThreadEventHander
  {
  public:
    ezIoThread(ezEventLoop* loop,int tid);
    virtual ~ezIoThread();
    ThreadEvQueue* GetEvQueue() {return evqueue_;}
    ezPoller* GetPoller() {return poller_;}
    int GetLoad(){return load_;}
    virtual void HandleInEvent();
    virtual void HandleOutEvent(){}
    virtual void HandleTimer(){}
    virtual void ProcessEvent(ezThreadEvent& ev);
    virtual void Run();
  private:
    int                     load_;
    ezPoller*               poller_;
    ThreadEvQueue*          evqueue_;
    int iiii;
  };
}
#endif