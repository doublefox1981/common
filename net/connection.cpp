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
	client_->SendMsg(msg);
}

bool net::ezConnection::RecvMsg(ezMsg& msg)
{
  return client_->RecvMsg(msg);
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
  switch(ev.type_)
  {
  case ezThreadEvent::NEW_CONNECTION:
    GetLooper()->AddConnection(this);
    GetLooper()->GetHander()->OnOpen(this);
    break;
  case ezThreadEvent::CLOSE_PASSIVE:
  case ezThreadEvent::CLOSE_ACTIVE:
    {
      ezIConnnectionHander* hander=GetLooper()->GetHander();
      ezMsg msg;
      while(RecvMsg(msg))
      {
        hander->OnData(this,&msg);
        ezMsgFree(&msg);
      }
      CloseClient();
    }
    break;
  case ezThreadEvent::CLOSE_CONNECTION:
    {
      GetLooper()->GetHander()->OnClose(this);
      GetLooper()->DelConnection(this);
      delete this;
    }
    break;
  default:
    break;
  }
}

void net::ezServerHander::OnOpen(ezConnection* conn)
{
//   ezEventLoop* looper=io->getlooper();
// 	ezConnection* conn=looper->getConnectionMgr()->addConnection(looper,fd,uuid,bindtid);
// 	if(conn)
// 	{
// 		char ipport[128];
// 		net::ToIpPort(ipport,sizeof(ipport),net::GetPeerAddr(fd));
// 		conn->setIpAddr(ipport);
// 		LOG_INFO("new connector from %s,tid=%d,fd=%d",ipport,bindtid,fd);
// 	}
}

void net::ezServerHander::OnClose(ezConnection* conn)
{
//   ezEventLoop* looper=io->getlooper();
//   int tid=io->gettid();
// 	ezConnection* conn=looper->getConnectionMgr()->findConnection(uuid);
// 	std::string ip=conn->getIpAddr();
// 	LOG_INFO("disconnect %s",ip.c_str());
// 	conn=nullptr;
// 	looper->getConnectionMgr()->delConnection(uuid);
}

void net::ezServerHander::OnData(ezConnection* conn,ezMsg* msg)
{
//   ezEventLoop* looper=io->getlooper();
//   net::ezConnection* conn=looper->getConnectionMgr()->findConnection(uuid);
//   if(conn)
//   {
//     conn->onRecvNetPack(msg);
//   }
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
  // 
}

void net::ezClientHander::OnOpen(ezConnection* conn)
{
//   ezEventLoop* looper=io->getlooper();
// 	ezConnectToInfo* info=looper->getConnectionMgr()->findConnectToInfo(uuid);
// 	assert(info);
// 	if(fd>0)
// 	{
// 		ezConnection* conn=looper->getConnectionMgr()->addConnection(looper,fd,uuid,bindtid);
// 		assert(conn);
// 		info->connectOK_=true;
// 		char ipport[128];
// 		net::ToIpPort(ipport,sizeof(ipport),GetPeerAddr(fd));
// 		conn->setIpAddr(ipport);
// 		LOG_INFO("connect to %s ok,fd=%d",ipport,fd);
// 	}
// 	else
// 	{
// 		info->connectOK_=false;
// 		LOG_WARN("connect to %s:%d fail ",info->ip_.c_str(),info->port_);
// 	}
}

void net::ezClientHander::OnClose(ezConnection* conn)
{
//   ezEventLoop* looper=io->getlooper();
//   int tid=io->gettid();
// 	ezConnection* conn=looper->getConnectionMgr()->findConnection(uuid);
// 	if(!conn)
// 		return;
// 	std::string ip=conn->getIpAddr();
// 	LOG_INFO("disconnect %s",ip.c_str());
// 	conn=nullptr;
// 	ezConnectToInfo* info=looper->getConnectionMgr()->findConnectToInfo(uuid);
// 	if(info)
// 		info->connectOK_=false;
// 	looper->getConnectionMgr()->delConnection(uuid);
}

void net::ezClientHander::OnData(ezConnection* conn,ezMsg* msg)
{
//   ezEventLoop* looper=io->getlooper();
//   int tid=io->gettid();
// 	net::ezConnection* conn=looper->getConnectionMgr()->findConnection(uuid);
// 	if(conn)
// 	{
// 		conn->onRecvNetPack(msg);
// 	}
}

int net::ezMsgDecoder::decode(ezIMessagePusher* pusher,uint64_t uuid,char* buf,size_t s)
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
    int onelen=sizeof(uint16_t)+msglen;
    ezMsgWarper msg;
    msg.uuid_=uuid;
    ezMsgInitSize(&msg.msg_,onelen);
    reader.ReadBuffer((char*)ezMsgData(&msg.msg_),msglen);
    retlen+=onelen;
    pusher->pushmsg(&msg);
  }
  return retlen;
}
