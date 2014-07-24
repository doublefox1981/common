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
    virtual void Rollback(ezMsg* msg)=0;
  };

  // 可被监听套接字和连接套接字继承
  class ezIFlashedFd
  {
  public:
    virtual ~ezIFlashedFd(){}
    virtual void Close()=0;
  };

  class ezListenerFd
    :public ezPollerEventHander
    ,public ezThreadEventHander
    ,public ezIFlashedFd
  {
  public:
    ezListenerFd(ezEventLoop* loop,ezIoThread* io,int fd);
    virtual ~ezListenerFd(){}
    virtual void ProcessEvent(ezThreadEvent& ev);
    virtual void HandleInEvent();
    virtual void HandleOutEvent(){}
    virtual void HandleTimer(){}
    virtual void Close();
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

  class ezClientMessagePuller:public ezIMessagePuller
  {
  public:
    explicit ezClientMessagePuller(ezClientFd* cli);
    virtual bool PullMsg(ezMsg* msg);
    virtual void Rollback(ezMsg*  msg);
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
    virtual void HandleTimer(){}
    virtual void ProcessEvent(ezThreadEvent& ev);
    void SendMsg(ezMsg& msg);
    bool RecvMsg(ezMsg& msg);
    void ActiveClose();
    void PassiveClose();
    int64_t GetUserData(){return userdata_;}
  private:
    ezIDecoder*       decoder_;
    ezIEncoder*       encoder_;
    ezIMessagePusher* pusher_;
    ezIMessagePuller* puller_;
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
    friend class ezClientMessagePuller;
  };

  class ezConnectToFd:public ezPollerEventHander,public ezThreadEventHander,public ezIFlashedFd
  {
  public:
    static const int CONNECTTO_TIMER_ID=1;
  public:
    ezConnectToFd(ezEventLoop* loop,ezIoThread* io,int64_t userd,int32_t reconnect/*reconnect timeout,second*/);
    void SetIpPort(const std::string& ip,int port);
    virtual void ProcessEvent(ezThreadEvent& ev);
    virtual void HandleInEvent();
    virtual void HandleOutEvent();
    virtual void HandleTimer();
    virtual void Close();
  private:
    void ConnectTo();
    int  CheckAsyncError();
    void Reconnect();
    void CloseMe();
    void PostCloseMe();
  private:
    ezIoThread* io_;
    std::string ip_;
    int port_;
    int fd_;
    int64_t userdata_;
    int32_t reconnect_;
  };
}
#endif