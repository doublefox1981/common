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

net::ezListenerFd::ezListenerFd(EventLoop* loop,IoThread* io,int fd)
  :fd_(fd),
  io_(io),
  ThreadEventHander(loop,io->get_tid())
{}

void net::ezListenerFd::handle_in_event()
{
  struct sockaddr_in si;
  SOCKET s=net::Accept(fd_,&si);
  if(s==INVALID_SOCKET)
    return;
  IoThread* newio=get_looper()->choose_thread();
  assert(newio);
  ClientFd* clifd=new ClientFd(get_looper(),newio,s,0);
  ThreadEvent ev;
  ev.type_=ThreadEvent::NEW_FD;
  clifd->occur_event(ev);
}

void net::ezListenerFd::process_event(ThreadEvent& ev)
{
  switch(ev.type_)
  {
  case ThreadEvent::NEW_SERVICE:
    {
      io_->AddFlashedFd(this);
      io_->GetPoller()->add_fd(fd_,this);
      io_->GetPoller()->set_poll_in(fd_);
    }
    break;
  default: break;
  }
}

void net::ezListenerFd::close()
{
  io_->DelFlashedFd(this);
  io_->GetPoller()->del_fd(fd_);
  CloseSocket(fd_);
  fd_=INVALID_SOCKET;
  delete this;
}

net::ClientFd::ClientFd(EventLoop* loop,IoThread* io,int fd,int64_t userdata)
  :io_(io)
  ,fd_(fd)
  ,userdata_(userdata)
  ,ThreadEventHander(loop,io->get_tid())
{
  decoder_=get_looper()->get_decoder();
  encoder_=get_looper()->get_encoder();
  pusher_=new ezClientMessagePusher(this);
  puller_=new ezClientMessagePuller(this);
  inbuf_=new Buffer(loop->get_buffer_size());
  outbuf_=new Buffer(loop->get_buffer_size());
  msg_init(&cachemsg_);
  cached_=false;
}

net::ClientFd::~ClientFd()
{
  net::CloseSocket(fd_);
  if(pusher_) delete pusher_;
  if(puller_) delete puller_;
  if(inbuf_) delete inbuf_;
  if(outbuf_) delete outbuf_;
  Msg msg;
  while(sendqueue_.try_dequeue(msg))
    msg_free(&msg);
}

void net::ClientFd::handle_in_event()
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
    int rets=decoder_->decode(pusher_,rbuf,rs);
    if(rets>0)
      inbuf_->drain(rets);
    else if(rets<0)
    {
      PassiveClose();
      return;
    }
  }
}

void net::ClientFd::handle_out_event()
{
  bool encoderet=true;
  while(true)
  {
    encoderet=encoder_->encode(puller_,outbuf_);
    char* pbuf=nullptr;
    int s=outbuf_->readable(pbuf);
    if(s<=0)
    {
      io_->GetPoller()->reset_poll_out(fd_);
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
      else if(retval==1||(retval==0&&!encoderet))
        break;
    }
  }
  if(!encoderet)
  {
    io_->GetPoller()->reset_poll_out(fd_);
    active_close();
    return;
  }
}

void net::ClientFd::process_event(ThreadEvent& ev)
{
  switch(ev.type_)
  {
  case ThreadEvent::NEW_FD:
    {
      Poller* poller=io_->GetPoller();
      if(!poller->add_fd(fd_,this))
      {
        delete this;
        return;
      }
      poller->set_poll_in(fd_);
      conn_=new Connection(get_looper(),this,get_looper()->get_tid(),userdata_);
      char ipport[128];
      net::ToIpPort(ipport,sizeof(ipport),net::GetPeerAddr(fd_));
      conn_->set_ip_addr(ipport);
      ThreadEvent newev;
      newev.type_=ThreadEvent::NEW_CONNECTION;
      conn_->occur_event(newev);
    }
    break;
  case ThreadEvent::CLOSE_FD:
    {
      io_->GetPoller()->del_fd(fd_);
      Msg msg;
      while(recvqueue_.try_dequeue(msg))
        msg_free(&msg);
      while(sendqueue_.try_dequeue(msg))
        msg_free(&msg);
      ThreadEvent ev;
      ev.type_=ThreadEvent::CLOSE_CONNECTION;
      conn_->occur_event(ev);
      delete this;
    }
    break;
  case ThreadEvent::ENABLE_POLLOUT:
    {
      io_->GetPoller()->set_poll_out(fd_);
      handle_out_event();
    }
    break;
  default:
    break;
  }
}

void net::ClientFd::send_msg(Msg& msg)
{
  sendqueue_.enqueue(msg);
}

bool net::ClientFd::recv_msg(Msg& msg)
{
  return recvqueue_.try_dequeue(msg);
}

void net::ClientFd::active_close()
{
  io_->GetPoller()->del_fd(fd_);
  ThreadEvent ev;
  ev.type_=ThreadEvent::CLOSE_ACTIVE;
  conn_->occur_event(ev);
}

void net::ClientFd::PassiveClose()
{
  io_->GetPoller()->del_fd(fd_);
  ThreadEvent ev;
  ev.type_=ThreadEvent::CLOSE_PASSIVE;
  conn_->occur_event(ev);
}

net::ezClientMessagePusher::ezClientMessagePusher(ClientFd* cli) :client_(cli)
{}

inline bool net::ezClientMessagePusher::push_msg(Msg* msg)
{
  client_->recvqueue_.enqueue(*msg);
  ThreadEvent ev;
  ev.type_=ThreadEvent::NEW_MESSAGE;
  client_->conn_->occur_event(ev);
  return true;
}

net::ezConnectToFd::ezConnectToFd(EventLoop* loop,IoThread* io,int64_t userd,int32_t reconnect)
  :fd_(0)
  ,io_(io)
  ,userdata_(userd)
  ,reconnect_(reconnect*1000)
  ,ThreadEventHander(loop,io->get_tid())
{}

void net::ezConnectToFd::SetIpPort(const std::string& ip,int port)
{
  ip_=ip;
  port_=port;
}

void net::ezConnectToFd::connect_to()
{
  int retval=net::connect_to(ip_.c_str(),port_,fd_);
  LOG_INFO("connecting %s:%d",ip_.c_str(),port_);
  if(retval==0)
  {
    if(io_->GetPoller()->add_fd(fd_,this))
      handle_out_event();
    else
      post_close_me();
  }
  else if(retval==-1&&errno==EINPROGRESS)
  {
    if(io_->GetPoller()->add_fd(fd_,this))
    {
      io_->GetPoller()->set_poll_in(fd_);
      io_->GetPoller()->set_poll_out(fd_);
    }
    else
      post_close_me();
  }
  else
  {
    if(fd_!=INVALID_SOCKET)
    {
      CloseSocket(fd_);
      fd_=INVALID_SOCKET;
    }
    reconnect();
  }
}

void net::ezConnectToFd::process_event(ThreadEvent& ev)
{
  switch(ev.type_)
  {
  case ThreadEvent::NEW_CONNECTTO:
    connect_to();
    io_->AddFlashedFd(this);
    break;
  case ThreadEvent::CLOSE_CONNECTTO:
    {
      close_me();
      return;
    }
    break;
  default:
    break;
  }
}

void net::ezConnectToFd::close_me()
{
  io_->GetPoller()->del_timer(this);
  if(fd_!=INVALID_SOCKET)
  {
    io_->GetPoller()->del_fd(fd_);
    CloseSocket(fd_);
    fd_=INVALID_SOCKET;
  }
  io_->DelFlashedFd(this);
  LOG_INFO("delete net::ezConnectToFd(%s:%d)",ip_.c_str(),port_);
  delete this;
}

void net::ezConnectToFd::reconnect()
{
  if(reconnect_>0)
    io_->GetPoller()->add_timer(reconnect_,this);
  else
    post_close_me();
}

void net::ezConnectToFd::post_close_me()
{
  ThreadEvent ev;
  ev.type_=ThreadEvent::CLOSE_CONNECTTO;
  occur_event(ev);
}

int net::ezConnectToFd::check_async_error()
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

void net::ezConnectToFd::handle_in_event()
{
  handle_out_event();
}

void net::ezConnectToFd::handle_out_event()
{
  int result=check_async_error();
  io_->GetPoller()->del_fd(fd_);
  fd_=INVALID_SOCKET;
  if(result==INVALID_SOCKET)
  {
    LOG_INFO("connect to %s:%d fail",ip_.c_str(),port_);
    CloseSocket(result);
    reconnect();
    return;
  }
  else
  {
    LOG_INFO("connect to %s:%d ok",ip_.c_str(),port_);
    ClientFd* clifd=new ClientFd(get_looper(),io_,result,userdata_);
    ThreadEvent ev;
    ev.type_=ThreadEvent::NEW_FD;
    clifd->occur_event(ev);
    post_close_me();
  }
}

void net::ezConnectToFd::handle_timer()
{
  connect_to();
}

void net::ezConnectToFd::close()
{
  close_me();
}

net::ezClientMessagePuller::ezClientMessagePuller(ClientFd* cli):client_(cli){}

bool net::ezClientMessagePuller::pull_msg(Msg* msg)
{
  if(client_->cached_)
  {
    client_->cached_=false;
    msg_copy(&client_->cachemsg_,msg);
    return true;
  }
  else
    return client_->sendqueue_.try_dequeue(*msg);
}

void net::ezClientMessagePuller::rollback(Msg* msg)
{
  assert(!client_->cached_);
  if(!client_->cached_)
  {
    client_->cached_=true;
    msg_copy(msg,&client_->cachemsg_);
  }
}
