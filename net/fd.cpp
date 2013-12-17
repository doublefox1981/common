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
//     ezHander* hander=looper->getHander();
//     char* rbuf=nullptr;
//     int rs=inbuf_->readable(rbuf);
//     if(rs>0)
//     {
//       int rets=hander->decode(looper,fd,uuid,rbuf,rs);
//       if(rets>0)
//         inbuf_->drain(rets);
//       else if(rets<0)
//       {
//         looper->n2oError(fd,uuid);
//         looper->del(fd);
//         return;
//       }
//     }
  }
  if(event&ezNetWrite)
  {
//     char* pbuf=nullptr;
//     int s=outbuf_->readable(pbuf);
//     int retval=0;
//     if(s>0)
//     {
//       retval=outbuf_->writefd(fd);
//       if(retval<0)
//       {
//         looper->n2oError(fd,uuid);
//         looper->del(fd);
//         return;
//       }
//     }
//     // 本次数据已经全部write成功
//     ezMsg msg;
//     if(!sendqueue_.try_dequeue(msg)&&retval==0)
//     {
//       looper->delWriteFd(fd);
//       looper->getPoller()->modFd(fd,ezNetWrite,ezNetWrite|ezNetRead,false);
//     }
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
  //   ezMsg msg;
  //   while(sendqueue_.try_dequeue(msg))
  //   {
  //     int canadd=outbuf_->fastadd();
  //     uint16_t msize=(uint16_t)ezMsgSize(&msg);
  //     if(sizeof(uint16_t)+msize<=canadd)
  //     {
  //       outbuf_->add(&msize,sizeof(msize));
  //       outbuf_->add(ezMsgData(&msg),msize);
  //       ezMsgFree(&msg);
  //     }
  //     else
  //     {
  //       ezMsgCopy(&msg,&cacheMsg_);
  //       break;
  //     }
  //   }
  return outbuf_->off();
  MSVC_POP_WARNING();
}

net::ezFdData::~ezFdData()
{
  if(ezfd_)
    delete ezfd_;
}

net::ezFdData::ezFdData():fd_(-1),event_(ezNetNone),ezfd_(nullptr),uuid_(0){}
