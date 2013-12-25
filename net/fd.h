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

  class ezIMessagePusher
  {
  public:
    virtual ~ezIMessagePusher(){}
    virtual bool PushMsg(ezMsg* msg)=0;
  };

  class ezIMessagePuller
  {
  public:
    virtual ~ezIMessagePuller(){}
    virtual bool PullMsg(ezMsg* msg)=0;
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

  class ezClientFd;
  class ezClientMessagePusher:public ezIMessagePusher
  {
  public:
    explicit ezClientMessagePusher(ezClientFd* cli);
    virtual bool PushMsg(ezMsg* msg);
  private:
    ezClientFd* client_;
  };

  typedef moodycamel::ReaderWriterQueue<ezMsg> MsgQueue;
  class ezClientFd:public ezPollerEventHander,public ezThreadEventHander
  {
  public:
    ezClientFd(ezEventLoop* loop,ezIoThread* io,int fd,int64_t userdata);
    virtual ~ezClientFd();
    virtual void HandleInEvent();
    virtual void HandleOutEvent();
    virtual void ProcessEvent(ezThreadEvent& ev);
    virtual bool pullmsg(ezMsg* msg);
    void SendMsg(ezMsg& msg);
    bool RecvMsg(ezMsg& msg);
    void ActiveClose();
    void PassiveClose();
    int64_t GetUserData(){return userdata_;}
  private:
    ezIDecoder*       decoder_;
    ezIMessagePusher* pusher_;
    ezIoThread* io_;
    int         fd_;
    int64_t     userdata_;
    ezBuffer*   inbuf_;
    ezBuffer*   outbuf_;
    MsgQueue    sendqueue_;
    MsgQueue    recvqueue_;
    ezMsg       cachemsg_;
    bool        cached_;
    ezConnection* conn_;

    friend class ezClientMessagePusher;
  };

  class ezConnectToFd:public ezPollerEventHander,public ezThreadEventHander
  {
  public:
    ezConnectToFd(ezEventLoop* loop,ezIoThread* io,int64_t userd);
    void SetIpPort(const std::string& ip,int port);
    virtual void ProcessEvent(ezThreadEvent& ev);
    virtual void HandleInEvent();
    virtual void HandleOutEvent();
    int CheckAsyncError();
    void Reconnect();
    void CloseMe();
    void PostCloseMe();
  private:
    ezIoThread* io_;
    std::string ip_;
    int port_;
    int fd_;
    int64_t userdata_;
  };
}
#endif