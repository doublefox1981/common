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

net::ezListenerFd::ezListenerFd(ezEventLoop* loop,ezIoThread* io,int fd)
  :fd_(fd),
  io_(io),
  ezThreadEventHander(loop,io->GetTid())
{}

void net::ezListenerFd::HandleInEvent()
{
  struct sockaddr_in si;
  SOCKET s=net::Accept(fd_,&si);
  if(s==INVALID_SOCKET)
    return;
  ezIoThread* newio=GetLooper()->ChooseThread();
  assert(newio);
  ezClientFd* clifd=new ezClientFd(GetLooper(),newio,s,0);
  ezThreadEvent ev;
  ev.type_=ezThreadEvent::NEW_FD;
  clifd->OccurEvent(ev);
}

void net::ezListenerFd::ProcessEvent(ezThreadEvent& ev)
{
  switch(ev.type_)
  {
  case ezThreadEvent::NEW_SERVICE:
    {
      io_->GetPoller()->AddFd(fd_,this);
      io_->GetPoller()->SetPollIn(fd_);
    }
    break;
  default: break;
  }
}

net::ezClientFd::ezClientFd(ezEventLoop* loop,ezIoThread* io,int fd,int64_t userdata)
  :io_(io)
  ,fd_(fd)
  ,userdata_(userdata)
  ,ezThreadEventHander(loop,io->GetTid())
{
  decoder_=GetLooper()->GetDecoder();
  pusher_=new ezClientMessagePusher(this);
  inbuf_=new ezBuffer();
  outbuf_=new ezBuffer();
  ezMsgInit(&cachemsg_);
  cached_=false;
}

net::ezClientFd::~ezClientFd()
{
  net::CloseSocket(fd_);
  if(pusher_) delete pusher_;
  if(inbuf_) delete inbuf_;
  if(outbuf_) delete outbuf_;
  ezMsg msg;
  while(sendqueue_.try_dequeue(msg))
    ezMsgFree(&msg);
}

// size_t net::ezClientFd::formatMsg()
// {
//   MSVC_PUSH_DISABLE_WARNING(4018);
//   if(cached_)
//   {
//     int canadd=outbuf_->fastadd();
//     uint16_t msize=(uint16_t)ezMsgSize(&cachemsg_);
//     if(sizeof(uint16_t)+msize>canadd)
//       return 0;
//     outbuf_->add(&msize,sizeof(msize));
//     outbuf_->add(ezMsgData(&cachemsg_),msize);
//     ezMsgFree(&cachemsg_);
//     cached_=false;
//   }
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
//       ezMsgCopy(&msg,&cachemsg_);
//       cached_=true;
//       break;
//     }
//   }
//   return outbuf_->off();
//   MSVC_POP_WARNING();
// }

bool net::ezClientFd::pullmsg(ezMsg* msg)
{
  return sendqueue_.try_dequeue(*msg);
}


void net::ezClientFd::HandleInEvent()
{
  int retval=inbuf_->readfd(fd_);
  if((retval==0)||(retval<0&&errno!=EAGAIN&&errno!=EINTR))
  {
    PassiveClose();
    return;
  }
  char* rbuf=nullptr;
  int rs=inbuf_->readable(rbuf);
  if(rs>0)
  {
    int rets=decoder_->Decode(pusher_,rbuf,rs);
    if(rets>0)
      inbuf_->drain(rets);
    else if(rets<0)
    {
      PassiveClose();
      return;
    }
  }
}


void net::ezClientFd::HandleOutEvent()
{
//   while(true)
//   {
//     if(formatMsg()<=0)
//       break;
//     char* pbuf=nullptr;
//     int s=outbuf_->readable(pbuf);
//     int retval=0;
//     if(s>0)
//     {
//       retval=outbuf_->writefd(fd);
//       if(retval<0)
//       {
//         ezCrossEventData data;
//         data.fromtid_=io->gettid();
//         data.fd_=fd;
//         data.event_=ezCrossError;
//         data.uuid_=uuid;
//         io->pushmain(data);
//         io->del(fd);
//         return;
//       }
//       else if(retval>0)
//         break;
//     }
//   }
//   io->mod(fd,ezNetWrite,false);
}

void net::ezClientFd::ProcessEvent(ezThreadEvent& ev)
{
  switch(ev.type_)
  {
  case ezThreadEvent::NEW_FD:
    {
      ezPoller* poller=io_->GetPoller();
      if(!poller->AddFd(fd_,this))
      {
        delete this;
        return;
      }
      poller->SetPollIn(fd_);
      conn_=new ezConnection(GetLooper(),this,GetLooper()->GetTid());
      char ipport[128];
      net::ToIpPort(ipport,sizeof(ipport),net::GetPeerAddr(fd_));
      conn_->SetIpAddr(ipport);
      ezThreadEvent newev;
      newev.type_=ezThreadEvent::NEW_CONNECTION;
      conn_->OccurEvent(newev);
    }
    break;
  case ezThreadEvent::CLOSE_FD:
    {
      //TODO clear msg
      io_->GetPoller()->DelFd(fd_);
      ezThreadEvent ev;
      ev.type_=ezThreadEvent::CLOSE_CONNECTION;
      conn_->OccurEvent(ev);
      delete this;
    }
    break;
  default:
    break;
  }
}

void net::ezClientFd::SendMsg(ezMsg& msg)
{
  sendqueue_.enqueue(msg);
}

bool net::ezClientFd::RecvMsg(ezMsg& msg)
{
  return recvqueue_.try_dequeue(msg);
}

void net::ezClientFd::ActiveClose()
{
  io_->GetPoller()->DelFd(fd_);
  ezThreadEvent ev;
  ev.type_=ezThreadEvent::CLOSE_ACTIVE;
  conn_->OccurEvent(ev);
}

void net::ezClientFd::PassiveClose()
{
  io_->GetPoller()->DelFd(fd_);
  ezThreadEvent ev;
  ev.type_=ezThreadEvent::CLOSE_PASSIVE;
  conn_->OccurEvent(ev);
}

net::ezClientMessagePusher::ezClientMessagePusher(ezClientFd* cli) :client_(cli)
{}

inline bool net::ezClientMessagePusher::PushMsg(ezMsg* msg)
{
  client_->recvqueue_.enqueue(*msg);
  ezThreadEvent ev;
  ev.type_=ezThreadEvent::NEW_MESSAGE;
  client_->conn_->OccurEvent(ev);
  return true;
}

net::ezConnectToFd::ezConnectToFd(ezEventLoop* loop,ezIoThread* io,int64_t userd)
  :fd_(0)
  ,io_(io)
  ,userdata_(userd)
  ,ezThreadEventHander(loop,io->GetTid())
{}

void net::ezConnectToFd::SetIpPort(const std::string& ip,int port)
{
  ip_=ip;
  port_=port;
}

void net::ezConnectToFd::ProcessEvent(ezThreadEvent& ev)
{
  switch(ev.type_)
  {
  case ezThreadEvent::NEW_CONNECTTO:
    {
      int retval=ConnectTo(ip_.c_str(),port_,fd_);
      if(retval==0)
      {
        if(io_->GetPoller()->AddFd(fd_,this))
          HandleOutEvent();
        else
          PostCloseMe();
      }
      else if(retval==-1&&errno==EINPROGRESS)
      {
        if(io_->GetPoller()->AddFd(fd_,this))
        {
          io_->GetPoller()->SetPollIn(fd_);
          io_->GetPoller()->SetPollOut(fd_);
        }
        else
          PostCloseMe();
      }
      else
      {
        if(fd_!=INVALID_SOCKET)
        {
          CloseSocket(fd_);
          fd_=INVALID_SOCKET;
        }
        Reconnect();
      }
    }
    break;
  case ezThreadEvent::CLOSE_CONNECTTO:
    {
      CloseMe();
      return;
    }
    break;
  default:
    break;
  }
}

void net::ezConnectToFd::CloseMe()
{
  // delete timer
  if(fd_!=INVALID_SOCKET)
  {
    io_->GetPoller()->DelFd(fd_);
    CloseSocket(fd_);
    fd_=INVALID_SOCKET;
  }
  delete this;
}

void net::ezConnectToFd::Reconnect()
{

}

void net::ezConnectToFd::PostCloseMe()
{
  ezThreadEvent ev;
  ev.type_=ezThreadEvent::CLOSE_CONNECTTO;
  OccurEvent(ev);
}

int net::ezConnectToFd::CheckAsyncError()
{
  int err=0;
  socklen_t len=sizeof(err);
  int rc=getsockopt(fd_,SOL_SOCKET,SO_ERROR,(char*)&err,&len);
  if (rc==-1)
    err=errno;
#ifdef __linux__
  if(err!=0) 
  {
    errno=err;
    return INVALID_SOCKET;
  }
#else
  if(err!=0)
  {
    if(err==WSAECONNREFUSED||err == WSAETIMEDOUT ||err == WSAECONNABORTED || err == WSAEHOSTUNREACH ||err == WSAENETUNREACH ||err == WSAENETDOWN ||err == WSAEINVAL||err == WSAEADDRINUSE)
      return INVALID_SOCKET;
  }
#endif
  return fd_;
}

void net::ezConnectToFd::HandleInEvent()
{
  HandleOutEvent();
}

void net::ezConnectToFd::HandleOutEvent()
{
  int result=CheckAsyncError();
  io_->GetPoller()->DelFd(fd_);
  fd_=INVALID_SOCKET;
  if(result==INVALID_SOCKET)
  {
    CloseSocket(result);
    Reconnect();
    return;
  }
  ezClientFd* clifd=new ezClientFd(GetLooper(),io_,result,userdata_);
  ezThreadEvent ev;
  ev.type_=ezThreadEvent::NEW_FD;
  clifd->OccurEvent(ev);
  PostCloseMe();
}
