#ifndef _NET_IOTHREAD_H
#define _NET_IOTHREAD_H
#include "../base/notifyqueue.h"
#include "event.h"
#include "fd.h"
#include "poller.h"
#include "socket.h"
#include <vector>

namespace net{
  class ezPoller;
  class ezIFlashedFd;
  class ezIoThread:public ezPollerEventHander,public base::Threads,public ezThreadEventHander
  {
  public:
    ezIoThread(ezEventLoop* loop,int tid);
    virtual ~ezIoThread();
    ThreadEvQueue* GetEvQueue() {return evqueue_;}
    ezPoller* GetPoller() {return poller_;}
    int GetLoad(){return poller_->GetLoad();}
    void AddFlashedFd(ezIFlashedFd* ffd);
    void DelFlashedFd(ezIFlashedFd* ffd);
    virtual void HandleInEvent();
    virtual void HandleOutEvent(){}
    virtual void HandleTimer(){}
    virtual void ProcessEvent(ezThreadEvent& ev);
    virtual void Run();
  private:
    int                     load_;
    ezPoller*               poller_;
    ThreadEvQueue*          evqueue_;
    // 便于在关闭系统时清理监听套接字和连接套接字
    std::vector<ezIFlashedFd*>   flashedfd_;
  };
}
#endif