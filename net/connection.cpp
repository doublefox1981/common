#include "../base/portable.h"
#include "../base/memorystream.h"
#include "../base/logging.h"
#include "connection.h"
#include "netpack.h"
#include "event.h"
#include "fd.h"
#include "poller.h"
#include "iothread.h"
#include "socket.h"

using namespace net;

net::Connection::Connection(EventLoop* looper,ezClientFd* client,int tid,int64_t userdata):ezThreadEventHander(looper,tid)
  ,client_(client)
  ,gameObj_(nullptr)
  ,userdata_(userdata)
{}

net::Connection::~Connection()
{
  dettach_game_object();
}

void net::Connection::attach_game_object(GameObject* obj)
{
  assert(obj);
  if(gameObj_)
    dettach_game_object();
  gameObj_=obj;
  gameObj_->SetConnection(this);
}

void net::Connection::dettach_game_object()
{
  if(gameObj_)
  {
    gameObj_->SetConnection(nullptr);
    gameObj_=nullptr;
  }
}

void net::Connection::SendMsg(Msg& msg)
{
  if(client_)
  {
    client_->SendMsg(msg);
    ezThreadEvent ev;
    ev.type_=ezThreadEvent::ENABLE_POLLOUT;
    client_->OccurEvent(ev);
  }
  else
    msg_free(&msg);
}

bool net::Connection::RecvMsg(Msg& msg)
{
  if(client_)
    return client_->RecvMsg(msg);
  return 
    false;
}

void net::Connection::ActiveClose()
{
  Msg msg;
  msg_init_delimiter(&msg);
  SendMsg(msg);
}

void net::Connection::CloseClient()
{
  if(client_)
  {
    ezThreadEvent ev;
    ev.type_=ezThreadEvent::CLOSE_FD;
    client_->OccurEvent(ev);
    client_=nullptr;
  }
}

void net::Connection::ProcessEvent(ezThreadEvent& ev)
{
  IConnnectionHander* hander=GetLooper()->GetHander();
  Msg msg;
  switch(ev.type_)
  {
  case ezThreadEvent::NEW_CONNECTION:
    GetLooper()->AddConnection(this);
    GetLooper()->GetHander()->on_open(this);
    break;
  case ezThreadEvent::CLOSE_PASSIVE:
  case ezThreadEvent::CLOSE_ACTIVE:
    while(RecvMsg(msg))
    {
      hander->on_data(this,&msg);
      msg_free(&msg);
    }
    CloseClient();
    break;
  case ezThreadEvent::CLOSE_CONNECTION:
    GetLooper()->GetHander()->on_close(this);
    GetLooper()->DelConnection(this);
    delete this;
    break;
  case ezThreadEvent::NEW_MESSAGE:
    while(RecvMsg(msg))
    {
      hander->on_data(this,&msg);
      msg_free(&msg);
    }
    break;
  default:
    break;
  }
}

int64_t net::Connection::GetUserdata()
{
  return userdata_;
}

void net::ServerHander::on_open(Connection* conn)
{
  LOG_INFO("new connector from %s",conn->get_ip_addr().c_str());
}

void net::ServerHander::on_close(Connection* conn)
{
  LOG_INFO("disconnect %s",conn->get_ip_addr().c_str());
}

void net::ServerHander::on_data(Connection* conn,Msg* msg)
{
  base::BufferReader reader((char*)msg_data(msg),msg_size(msg));
  int seq=0;
  reader.read(seq);
  LOG_INFO("seq=%d,size=%d",seq,msg_size(msg));
}

void net::GameObject::Close()
{
  if(conn_) 
  {
    conn_->ActiveClose();
    conn_->dettach_game_object();
  }
}

void net::GameObject::SendNetpack(Msg& msg)
{
  if(conn_)
    conn_->SendMsg(msg);
  else
    msg_free(&msg);
}

net::GameObject::GameObject()
{

}

net::GameObject::~GameObject()
{
	if(conn_)
		conn_->dettach_game_object();
}

void net::ClientHander::on_open(Connection* conn)
{
}

void net::ClientHander::on_close(Connection* conn)
{
  LOG_INFO("disconnect %s",conn->get_ip_addr().c_str());
}

void net::ClientHander::on_data(Connection* conn,Msg* msg)
{
}

int net::MsgDecoder::decode(IMessagePusher* pusher,char* buf,size_t s)
{
  base::BufferReader reader(buf,s);
  int retlen=0;
  while(true)
  {
    uint16_t msglen=0;
    reader.read<uint16_t>(msglen);
    if(reader.fail())
      break;
    if(msglen<=0||msglen>maxMsgSize_)
      return -1;
    if(!reader.can_increase_size(msglen))
      break;
    Msg msg;
    msg_init_size(&msg,msglen);
    reader.read_buffer((char*)msg_data(&msg),msglen);
    retlen+=sizeof(uint16_t);
    retlen+=msglen;
    pusher->PushMsg(&msg);
  }
  return retlen;
}

bool net::MsgEncoder::encode(IMessagePuller* puller,Buffer* buffer)
{
  Msg msg;
  while(puller->PullMsg(&msg))
  {
    if(msg_is_delimiter(&msg))
      return false;
    int canadd=buffer->fastadd();
    uint16_t msize=(uint16_t)msg_size(&msg);
    if(int(sizeof(uint16_t)+msize)<=canadd)
    {
      buffer->add(&msize,sizeof(msize));
      buffer->add(msg_data(&msg),msize);
      msg_free(&msg);
    }
    else
    {
      puller->Rollback(&msg);
      break;
    }
  }
  return true;
}

net::GameObject*  net::get_game_object(net::Connection* conn)
{
  return conn->get_game_object();
}

void net::attach_game_object(net::Connection* conn,net::GameObject* obj)
{
  conn->attach_game_object(obj);
}

void net::dettach_game_object(net::Connection* conn)
{
  conn->dettach_game_object();
}

const char* net::get_ip_addr(net::Connection* conn)
{
  return conn->get_ip_addr().c_str();
}