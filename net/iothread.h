#ifndef _NET_IOTHREAD_H
#define _NET_IOTHREAD_H
#include "../base/notifyqueue.h"
#include "event.h"
#include "fd.h"
#include "poller.h"
#include "socket.h"
#include <vector>

namespace net{
  class Poller;
  class ezIFlashedFd;
  class IoThread:public IPollerEventHander,public base::Threads,public ThreadEventHander
  {
  public:
    IoThread(EventLoop* loop,int tid);
    virtual ~IoThread();
    ThreadEvQueue* GetEvQueue() {return evqueue_;}
    Poller* GetPoller() {return poller_;}
    int get_load(){return poller_->get_load();}
    void AddFlashedFd(ezIFlashedFd* ffd);
    void DelFlashedFd(ezIFlashedFd* ffd);
    virtual void handle_in_event();
    virtual void handle_out_event(){}
    virtual void handle_timer(){}
    virtual void process_event(ThreadEvent& ev);
    virtual void run();
  private:
    int                     load_;
    Poller*               poller_;
    ThreadEvQueue*          evqueue_;
    // 便于在关闭系统时清理监听套接字和连接套接字
    std::vector<ezIFlashedFd*>   flashedfd_;
  };
}
#endif