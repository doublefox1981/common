#ifndef _NET_FD_H
#define _NET_FD_H
#include "buffer.h"
#include "netpack.h"
#include "../base/readerwriterqueue.h"
#include "../base/notifyqueue.h"

namespace net{
  class ezIoThread;
  class ezFd;
  struct ezMsgWarper;
  struct ezFdData
  {
    int fd_;
    uint64_t uuid_;
    int event_;
    ezFd* ezfd_;
    ezFdData();
    ~ezFdData();
  };

  class ezIMessagePusher
  {
  public:
    virtual bool pushmsg(ezMsgWarper* msg)=0;
  };

  class ezIMessagePuller
  {
  public:
    virtual bool pullmsg(ezMsg* msg)=0;
  };

  class ezFd
  {
  public:
    virtual void OnEvent(ezIoThread* io,int fd,int event,uint64_t uuid)=0;
    virtual ~ezFd(){}
  };

  class ezListenerFd:public ezFd
  {
  public:
    virtual void OnEvent(ezIoThread* io,int fd,int event,uint64_t uuid);
  };
  
  typedef moodycamel::ReaderWriterQueue<ezMsg> MsgQueue;
  class ezClientFd:public ezFd,public ezIMessagePuller,public ezThreadEventHander
  {
  public:
    ezClientFd(ezEventLoop* loop,ezIoThread* io,int fd);
    virtual ~ezClientFd();
    virtual void OnEvent(ezIoThread* io,int fd,int event,uint64_t uuid);
    virtual void ProcessEvent(ezThreadEvent& ev);
    virtual bool pullmsg(ezMsg* msg);
  private:
    ezIoThread* io_;
    int         fd_;
    ezBuffer*   inbuf_;
    ezBuffer*   outbuf_;
    MsgQueue    sendqueue_;
    MsgQueue    recvqueue_;
    ezMsg       cachemsg_;
    bool        cached_;
    ezConnection* conn_;
  };
}
#endif