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
  class ezIoThread:public ezFd,public base::Threads,public ezIMessagePusher
  {
  public:
    ezIoThread();
    virtual ~ezIoThread();
    void setlooper(ezEventLoop* loop){looper_=loop;}
    ezEventLoop* getlooper(){return looper_;}
    int getload(){return load_;}
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
    void sendMsg(int fd,ezMsg& msg);
    virtual void onEvent(ezIoThread* io,int fd,int event,uint64_t uuid);
    virtual void sendMsg(ezMsg& blk){}
    virtual size_t formatMsg(){return 0;}
    virtual bool pushmsg(ezMsgWarper* msg);
    virtual void Run();
  private:
    int                     load_;
    int                     maxfd_;
    int                     tid_;
    ezPoller*               poller_;
    ezEventLoop*            looper_;
    CrossEvQueue*           ioqueue_;
    CrossEvQueue*           mainqueue_;
    MsgWarperQueue*         msgqueue_;
    std::vector<ezFdData*>  fds_;        // all fd
    std::vector<ezFdData*>  firedfd_;    // fd that fired event
  };
}
#endif