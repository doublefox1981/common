#ifndef _NET_FD_H
#define _NET_FD_H
#include "buffer.h"
#include "netpack.h"
#include "event.h"
#include "poller.h"
#include "../base/readerwriterqueue.h"
#include "../base/notifyqueue.h"

namespace net{
  class ezIoThread;
  struct ezMsgWarper;

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

  class ezListenerFd:public ezPollerEventHander,public ezThreadEventHander
  {
  public:
    ezListenerFd(ezEventLoop* loop,ezIoThread* io,int fd);
    virtual ~ezListenerFd(){}
    virtual void ProcessEvent(ezThreadEvent& ev);
    virtual void HandleInEvent();
    virtual void HandleOutEvent(){}
  private:
    int fd_;
    ezIoThread* io_;
  };

  typedef moodycamel::ReaderWriterQueue<ezMsg> MsgQueue;
  class ezClientFd:public ezPollerEventHander,public ezThreadEventHander
  {
  public:
    ezClientFd(ezEventLoop* loop,ezIoThread* io,int fd);
    virtual ~ezClientFd();
    virtual void HandleInEvent();
    virtual void HandleOutEvent();
    virtual void ProcessEvent(ezThreadEvent& ev);
    virtual bool pullmsg(ezMsg* msg);
    void SendMsg(ezMsg& msg);
    bool RecvMsg(ezMsg& msg);
    void ActiveClose();
    void PassiveClose();
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