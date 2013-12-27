#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#else
#include <io.h>
#endif
#include "../base/util.h"
#include "../base/eztime.h"
#include "socket.h"
#include "event.h"
#include "connection.h"
#include "poller.h"
#include "fd.h"
#include "iothread.h"
#include "../base/logging.h"

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
  encoder_=GetLooper()->GetEncoder();
  pusher_=new ezClientMessagePusher(this);
  puller_=new ezClientMessagePuller(this);
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
  bool encoderet=true;
  while(true)
  {
    encoderet=encoder_->Encode(puller_,outbuf_);
    char* pbuf=nullptr;
    int s=outbuf_->readable(pbuf);
    if(s<=0)
    {
      io_->GetPoller()->ResetPollOut(fd_);
      break;
    }
    else
    {
      int retval=outbuf_->writefd(fd_);
      if(retval<0)
      {
        PassiveClose();
        return;
      }
      else if(retval==1)
        break;
    }
  }
  if(!encoderet)
    ActiveClose();
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
      io_->GetPoller()->DelFd(fd_);
      ezMsg msg;
      while(recvqueue_.try_dequeue(msg))
        ezMsgFree(&msg);
      while(sendqueue_.try_dequeue(msg))
        ezMsgFree(&msg);
      ezThreadEvent ev;
      ev.type_=ezThreadEvent::CLOSE_CONNECTION;
      conn_->OccurEvent(ev);
      delete this;
    }
    break;
  case ezThreadEvent::ENABLE_POLLOUT:
    {
      io_->GetPoller()->SetPollOut(fd_);
      HandleOutEvent();
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

net::ezConnectToFd::ezConnectToFd(ezEventLoop* loop,ezIoThread* io,int64_t userd,int32_t reconnect)
  :fd_(0)
  ,io_(io)
  ,userdata_(userd)
  ,reconnect_(reconnect*1000)
  ,ezThreadEventHander(loop,io->GetTid())
{}

void net::ezConnectToFd::SetIpPort(const std::string& ip,int port)
{
  ip_=ip;
  port_=port;
}

void net::ezConnectToFd::ConnectTo()
{
  int retval=net::ConnectTo(ip_.c_str(),port_,fd_);
  LOG_INFO("connecting %s:%d",ip_.c_str(),port_);
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

void net::ezConnectToFd::ProcessEvent(ezThreadEvent& ev)
{
  switch(ev.type_)
  {
  case ezThreadEvent::NEW_CONNECTTO:
    ConnectTo();
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
  io_->GetPoller()->DelTimer(this,CONNECTTO_TIMER_ID);
  if(fd_!=INVALID_SOCKET)
  {
    io_->GetPoller()->DelFd(fd_);
    CloseSocket(fd_);
    fd_=INVALID_SOCKET;
  }
  LOG_INFO("delete net::ezConnectToFd(%s:%d)",ip_.c_str(),port_);
  delete this;
}

void net::ezConnectToFd::Reconnect()
{
  if(reconnect_>0)
    io_->GetPoller()->AddTimer(reconnect_,this,CONNECTTO_TIMER_ID);
  else
    PostCloseMe();
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
    LOG_INFO("connect to %s:%d fail",ip_.c_str(),port_);
    CloseSocket(result);
    Reconnect();
    return;
  }
  else
  {
    LOG_INFO("connect to %s:%d ok",ip_.c_str(),port_);
    ezClientFd* clifd=new ezClientFd(GetLooper(),io_,result,userdata_);
    ezThreadEvent ev;
    ev.type_=ezThreadEvent::NEW_FD;
    clifd->OccurEvent(ev);
    PostCloseMe();
  }
}

void net::ezConnectToFd::HandleTimer()
{
  ConnectTo();
}

net::ezClientMessagePuller::ezClientMessagePuller(ezClientFd* cli):client_(cli){}

bool net::ezClientMessagePuller::PullMsg(ezMsg* msg)
{
  if(client_->cached_)
  {
    client_->cached_=false;
    ezMsgCopy(&client_->cachemsg_,msg);
    return true;
  }
  else
    return client_->sendqueue_.try_dequeue(*msg);
}

void net::ezClientMessagePuller::Rollback(ezMsg* msg)
{
  assert(!client_->cached_);
  if(!client_->cached_)
  {
    client_->cached_=true;
    ezMsgCopy(msg,&client_->cachemsg_);
  }
}
