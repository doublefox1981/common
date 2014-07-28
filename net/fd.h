#ifndef _NET_FD_H
#define _NET_FD_H
#include "buffer.h"
#include "netpack.h"
#include "event.h"
#include "poller.h"
#include "../base/readerwriterqueue.h"
#include "../base/notifyqueue.h"

namespace net{
  class IoThread;

  class IMessagePusher
  {
  public:
    virtual ~IMessagePusher(){}
    virtual bool push_msg(Msg* msg)=0;
  };

  class IMessagePuller
  {
  public:
    virtual ~IMessagePuller(){}
    virtual bool pull_msg(Msg* msg)=0;
    virtual void rollback(Msg* msg)=0;
  };

  // 可被监听套接字和连接套接字继承
  class ezIFlashedFd
  {
  public:
    virtual ~ezIFlashedFd(){}
    virtual void close()=0;
  };

  class ezListenerFd
    :public IPollerEventHander
    ,public ThreadEventHander
    ,public ezIFlashedFd
  {
  public:
    ezListenerFd(EventLoop* loop,IoThread* io,int fd);
    virtual ~ezListenerFd(){}
    virtual void process_event(ThreadEvent& ev);
    virtual void handle_in_event();
    virtual void handle_out_event(){}
    virtual void handle_timer(){}
    virtual void close();
  private:
    int fd_;
    IoThread* io_;
  };

  class ClientFd;
  class ezClientMessagePusher:public IMessagePusher
  {
  public:
    explicit ezClientMessagePusher(ClientFd* cli);
    virtual bool push_msg(Msg* msg);
  private:
    ClientFd* client_;
  };

  class ezClientMessagePuller:public IMessagePuller
  {
  public:
    explicit ezClientMessagePuller(ClientFd* cli);
    virtual bool pull_msg(Msg* msg);
    virtual void rollback(Msg*  msg);
  private:
    ClientFd* client_;
  };

  typedef moodycamel::ReaderWriterQueue<Msg> MsgQueue;
  class ClientFd:public IPollerEventHander,public ThreadEventHander
  {
  public:
    ClientFd(EventLoop* loop,IoThread* io,int fd,int64_t userdata);
    virtual ~ClientFd();
    virtual void handle_in_event();
    virtual void handle_out_event();
    virtual void handle_timer(){}
    virtual void process_event(ThreadEvent& ev);
    void send_msg(Msg& msg);
    bool recv_msg(Msg& msg);
    void active_close();
    void PassiveClose();
    int64_t get_user_data(){return userdata_;}
  private:
    IDecoder*       decoder_;
    IEncoder*       encoder_;
    IMessagePusher* pusher_;
    IMessagePuller* puller_;
    IoThread* io_;
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

  class ezConnectToFd:public IPollerEventHander,public ThreadEventHander,public ezIFlashedFd
  {
  public:
    static const int CONNECTTO_TIMER_ID=1;
  public:
    ezConnectToFd(EventLoop* loop,IoThread* io,int64_t userd,int32_t reconnect/*reconnect timeout,second*/);
    void SetIpPort(const std::string& ip,int port);
    virtual void process_event(ThreadEvent& ev);
    virtual void handle_in_event();
    virtual void handle_out_event();
    virtual void handle_timer();
    virtual void close();
  private:
    void connect_to();
    int  check_async_error();
    void reconnect();
    void close_me();
    void post_close_me();
  private:
    IoThread* io_;
    std::string ip_;
    int port_;
    int fd_;
    int64_t userdata_;
    int32_t reconnect_;
  };
}
#endif