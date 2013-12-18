#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#else
#include <io.h>
#endif
#include "../base/util.h"
#include "socket.h"
#include "event.h"
#include "connection.h"
#include "poller.h"
#include "fd.h"
#include "iothread.h"

void net::ezListenerFd::onEvent(ezIoThread* io,int fd,int mask,uint64_t uuid)
{
  if(mask&ezNetRead)
  {
    struct sockaddr_in si;
    SOCKET s=net::Accept(fd,&si);
    if(s==INVALID_SOCKET)
      return;
    ezIoThread* newio=io->getlooper()->chooseThread();
    uint64_t uuid=newio->add(s,ezUUID::instance()->uuid(),new ezClientFd,ezNetRead);
    ezCrossEventData data;
    data.fromtid_=newio->gettid();
    data.fd_=fd;
    data.event_=ezCrossOpen;
    data.uuid_=uuid;
    io->pushmain(data);
  }
}

void net::ezListenerFd::sendMsg(ezMsg& blk )
{
  assert(false);
}

size_t net::ezListenerFd::formatMsg()
{
  return 0;
}

void net::ezClientFd::onEvent(ezIoThread* io,int fd,int event,uint64_t uuid)
{
  if(event&ezNetRead)
  {
    ezEventLoop* looper=io->getlooper();
    int retval=inbuf_->readfd(fd);
    if((retval==0)||(retval<0&&errno!=EAGAIN&&errno!=EINTR))
    {
      ezCrossEventData data;
      data.fromtid_=io->gettid();
      data.fd_=fd;
      data.event_=ezCrossClose;
      data.uuid_=uuid;
      io->pushmain(data);
      io->del(fd);
      return;
    }
    ezIHander* hander=looper->getHander();
    char* rbuf=nullptr;
    int rs=inbuf_->readable(rbuf);
    if(rs>0)
    {
      int rets=hander->getDecoder()->decode(io,uuid,rbuf,rs);
      if(rets>0)
      {
        ezCrossEventData data;
        data.fromtid_=io->gettid();
        data.fd_=fd;
        data.event_=ezCrossData;
        data.uuid_=uuid;
        io->pushmain(data);
        inbuf_->drain(rets);
      }
      else if(rets<0)
      {
        ezCrossEventData data;
        data.fromtid_=io->gettid();
        data.fd_=fd;
        data.event_=ezCrossError;
        data.uuid_=uuid;
        io->pushmain(data);
        io->del(fd);
        return;
      }
    }
  }
  if(event&ezNetWrite)
  {
    while(true)
    {
      if(formatMsg()<=0)
        break;
      char* pbuf=nullptr;
      int s=outbuf_->readable(pbuf);
      int retval=0;
      if(s>0)
      {
        retval=outbuf_->writefd(fd);
        if(retval<0)
        {
          ezCrossEventData data;
          data.fromtid_=io->gettid();
          data.fd_=fd;
          data.event_=ezCrossError;
          data.uuid_=uuid;
          io->pushmain(data);
          io->del(fd);
          return;
        }
        else if(retval>0)
          break;
      }
    }
    io->mod(fd,ezNetWrite,false);
  }
  if(event&ezNetErr)
  {
    ezCrossEventData data;
    data.fromtid_=io->gettid();
    data.fd_=fd;
    data.event_=ezCrossError;
    data.uuid_=uuid;
    io->pushmain(data);
    io->del(fd);
    return;
  }
}

net::ezClientFd::ezClientFd()
{
  inbuf_=new ezBuffer();
  outbuf_=new ezBuffer();
  ezMsgInit(&cachemsg_);
  cached_=false;
}

net::ezClientFd::~ezClientFd()
{
  if(inbuf_)
    delete inbuf_;
  if(outbuf_)
    delete outbuf_;
  ezMsg msg;
  while(sendqueue_.try_dequeue(msg))
    ezMsgFree(&msg);
}

void net::ezClientFd::sendMsg(ezMsg& blk)
{
  sendqueue_.enqueue(blk);
}

size_t net::ezClientFd::formatMsg()
{
  MSVC_PUSH_DISABLE_WARNING(4018);
  if(cached_)
  {
    int canadd=outbuf_->fastadd();
    uint16_t msize=(uint16_t)ezMsgSize(&cachemsg_);
    if(sizeof(uint16_t)+msize>canadd)
      return 0;
    outbuf_->add(&msize,sizeof(msize));
    outbuf_->add(ezMsgData(&cachemsg_),msize);
    ezMsgFree(&cachemsg_);
    cached_=false;
  }
  ezMsg msg;
  while(sendqueue_.try_dequeue(msg))
  {
    int canadd=outbuf_->fastadd();
    uint16_t msize=(uint16_t)ezMsgSize(&msg);
    if(sizeof(uint16_t)+msize<=canadd)
    {
      outbuf_->add(&msize,sizeof(msize));
      outbuf_->add(ezMsgData(&msg),msize);
      ezMsgFree(&msg);
    }
    else
    {
      ezMsgCopy(&msg,&cachemsg_);
      cached_=true;
      break;
    }
  }
  return outbuf_->off();
  MSVC_POP_WARNING();
}

bool net::ezClientFd::pullmsg(ezMsg* msg)
{
  return sendqueue_.try_dequeue(*msg);
}

net::ezFdData::~ezFdData()
{
  if(ezfd_)
    delete ezfd_;
}

net::ezFdData::ezFdData():fd_(-1),event_(ezNetNone),ezfd_(nullptr),uuid_(0){}
