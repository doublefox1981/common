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

  class IMessagePusher
  {
  public:
    virtual ~IMessagePusher(){}
    virtual bool PushMsg(Msg* msg)=0;
  };

  class IMessagePuller
  {
  public:
    virtual ~IMessagePuller(){}
    virtual bool PullMsg(Msg* msg)=0;
    virtual void Rollback(Msg* msg)=0;
  };

  // 可被监听套接字和连接套接字继承
  class ezIFlashedFd
  {
  public:
    virtual ~ezIFlashedFd(){}
    virtual void Close()=0;
  };

  class ezListenerFd
    :public IPollerEventHander
    ,public ezThreadEventHander
    ,public ezIFlashedFd
  {
  public:
    ezListenerFd(EventLoop* loop,ezIoThread* io,int fd);
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
  class ezClientMessagePusher:public IMessagePusher
  {
  public:
    explicit ezClientMessagePusher(ezClientFd* cli);
    virtual bool PushMsg(Msg* msg);
  private:
    ezClientFd* client_;
  };

  class ezClientMessagePuller:public IMessagePuller
  {
  public:
    explicit ezClientMessagePuller(ezClientFd* cli);
    virtual bool PullMsg(Msg* msg);
    virtual void Rollback(Msg*  msg);
  private:
    ezClientFd* client_;
  };

  typedef moodycamel::ReaderWriterQueue<Msg> MsgQueue;
  class ezClientFd:public IPollerEventHander,public ezThreadEventHander
  {
  public:
    ezClientFd(EventLoop* loop,ezIoThread* io,int fd,int64_t userdata);
    virtual ~ezClientFd();
    virtual void HandleInEvent();
    virtual void HandleOutEvent();
    virtual void HandleTimer(){}
    virtual void ProcessEvent(ezThreadEvent& ev);
    void SendMsg(Msg& msg);
    bool RecvMsg(Msg& msg);
    void ActiveClose();
    void PassiveClose();
    int64_t GetUserData(){return userdata_;}
  private:
    IDecoder*       decoder_;
    IEncoder*       encoder_;
    IMessagePusher* pusher_;
    IMessagePuller* puller_;
    ezIoThread* io_;
    int         fd_;
    int64_t     userdata_;
    Buffer*   inbuf_;
    Buffer*   outbuf_;
    MsgQueue    sendqueue_;
    MsgQueue    recvqueue_;
    Msg       cachemsg_;
    bool        cached_;
    Connection* conn_;

    friend class ezClientMessagePusher;
    friend class ezClientMessagePuller;
  };

  class ezConnectToFd:public IPollerEventHander,public ezThreadEventHander,public ezIFlashedFd
  {
  public:
    static const int CONNECTTO_TIMER_ID=1;
  public:
    ezConnectToFd(EventLoop* loop,ezIoThread* io,int64_t userd,int32_t reconnect/*reconnect timeout,second*/);
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