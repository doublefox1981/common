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

net::ezConnection::ezConnection(ezEventLoop* looper,ezClientFd* client,int tid):ezThreadEventHander(looper,tid)
  ,client_(client)
  ,gameObj_(nullptr){}

net::ezConnection::~ezConnection()
{
  DetachGameObject();
}

void net::ezConnection::AttachGameObject(ezGameObject* obj)
{
	gameObj_=obj;
	if(gameObj_)
		gameObj_->SetConnection(this);
}

void net::ezConnection::DetachGameObject()
{
	if(gameObj_)
		gameObj_->SetConnection(nullptr);
	gameObj_=nullptr;
}

void net::ezConnection::SendMsg(ezMsg& msg)
{
  if(client_)
  {
    client_->SendMsg(msg);
    ezThreadEvent ev;
    ev.type_=ezThreadEvent::ENABLE_POLLOUT;
    client_->OccurEvent(ev);
  }
  else
    ezMsgFree(&msg);
}

bool net::ezConnection::RecvMsg(ezMsg& msg)
{
  if(client_)
    return client_->RecvMsg(msg);
  return 
    false;
}

void net::ezConnection::ActiveClose()
{
  ezMsg msg;
  ezMsgInitDelimiter(&msg);
  SendMsg(msg);
}

void net::ezConnection::CloseClient()
{
  if(client_)
  {
    ezThreadEvent ev;
    ev.type_=ezThreadEvent::CLOSE_FD;
    client_->OccurEvent(ev);
    client_=nullptr;
  }
}

void net::ezConnection::ProcessEvent(ezThreadEvent& ev)
{
  ezIConnnectionHander* hander=GetLooper()->GetHander();
  ezMsg msg;
  switch(ev.type_)
  {
  case ezThreadEvent::NEW_CONNECTION:
    GetLooper()->AddConnection(this);
    GetLooper()->GetHander()->OnOpen(this);
    break;
  case ezThreadEvent::CLOSE_PASSIVE:
  case ezThreadEvent::CLOSE_ACTIVE:
    while(RecvMsg(msg))
    {
      hander->OnData(this,&msg);
      ezMsgFree(&msg);
    }
    CloseClient();
    break;
  case ezThreadEvent::CLOSE_CONNECTION:
    GetLooper()->GetHander()->OnClose(this);
    GetLooper()->DelConnection(this);
    delete this;
    break;
  case ezThreadEvent::NEW_MESSAGE:
    while(RecvMsg(msg))
    {
      hander->OnData(this,&msg);
      ezMsgFree(&msg);
    }
    break;
  default:
    break;
  }
}

int64_t net::ezConnection::GetUserdata()
{
  return client_->GetUserData();
}

void net::ezServerHander::OnOpen(ezConnection* conn)
{
  LOG_INFO("new connector from %s",conn->GetIpAddr().c_str());
}

void net::ezServerHander::OnClose(ezConnection* conn)
{
  LOG_INFO("disconnect %s",conn->GetIpAddr().c_str());
}

void net::ezServerHander::OnData(ezConnection* conn,ezMsg* msg)
{
  base::ezBufferReader reader((char*)ezMsgData(msg),ezMsgSize(msg));
  int seq=0;
  reader.Read(seq);
  LOG_INFO("seq=%d,size=%d",seq,ezMsgSize(msg));
}

void net::ezGameObject::Close()
{
	if(conn_) conn_->ActiveClose();
}

void net::ezGameObject::SendNetpack(ezMsg& msg)
{
  if(conn_)
    conn_->SendMsg(msg);
  else
    ezMsgFree(&msg);
}

net::ezGameObject::ezGameObject()
{

}

net::ezGameObject::~ezGameObject()
{
	if(conn_)
		conn_->DetachGameObject();
}

void net::ezConnectToGameObject::Close()
{
	ezGameObject::Close();
}

void net::ezClientHander::OnOpen(ezConnection* conn)
{
}

void net::ezClientHander::OnClose(ezConnection* conn)
{
  LOG_INFO("disconnect %s",conn->GetIpAddr().c_str());
}

void net::ezClientHander::OnData(ezConnection* conn,ezMsg* msg)
{
}

int net::ezMsgDecoder::Decode(ezIMessagePusher* pusher,char* buf,size_t s)
{
  base::ezBufferReader reader(buf,s);
  int retlen=0;
  while(true)
  {
    uint16_t msglen=0;
    reader.Read<uint16_t>(msglen);
    if(reader.Fail())
      break;
    if(msglen<=0||msglen>maxMsgSize_)
      return -1;
    if(!reader.CanIncreaseSize(msglen))
      break;
    ezMsg msg;
    ezMsgInitSize(&msg,msglen);
    reader.ReadBuffer((char*)ezMsgData(&msg),msglen);
    retlen+=sizeof(uint16_t);
    retlen+=msglen;
    pusher->PushMsg(&msg);
  }
  return retlen;
}

bool net::ezMsgEncoder::Encode(ezIMessagePuller* puller,ezBuffer* buffer)
{
  ezMsg msg;
  while(puller->PullMsg(&msg))
  {
    if(ezMsgIsDelimiter(&msg))
      return false;
    int canadd=buffer->fastadd();
    uint16_t msize=(uint16_t)ezMsgSize(&msg);
    if(int(sizeof(uint16_t)+msize)<=canadd)
    {
      buffer->add(&msize,sizeof(msize));
      buffer->add(ezMsgData(&msg),msize);
      ezMsgFree(&msg);
    }
    else
    {
      puller->Rollback(&msg);
      break;
    }
  }
  return true;
}
