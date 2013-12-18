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
    virtual void onEvent(ezIoThread* io,int fd,int event,uint64_t uuid)=0;
    virtual void sendMsg(ezMsg& blk)=0;
    virtual size_t formatMsg()=0;
    virtual ~ezFd(){}
  };

  class ezListenerFd:public ezFd
  {
  public:
    virtual void onEvent(ezIoThread* io,int fd,int event,uint64_t uuid);
    virtual void sendMsg(ezMsg& blk);
    virtual size_t formatMsg();
  };
  
  typedef moodycamel::ReaderWriterQueue<ezMsg> MsgQueue;
  class ezClientFd:public ezFd,public ezIMessagePuller
  {
  public:
    ezClientFd();
    virtual ~ezClientFd();
    virtual void onEvent(ezIoThread* io,int fd,int event,uint64_t uuid);
    virtual void sendMsg(ezMsg& blk);
    virtual size_t formatMsg();
    virtual bool pullmsg(ezMsg* msg);
  private:
    ezBuffer* inbuf_;
    ezBuffer* outbuf_;
    MsgQueue  sendqueue_;
  };
}
#endif