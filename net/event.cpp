#include "event.h"
#include "socket.h"
#include "poller.h"
#include "connection.h"
#include "fd.h"
#include "iothread.h"
#include "../base/memorystream.h"
#include "../base/thread.h"
#include "../base/logging.h"
#include "../base/util.h"
#include <assert.h>

int net::ezEventLoop::init(ezHander* hander,ezConnectionMgr* mgr,int tnum)
{
	hander_=hander;
	conMgr_=mgr;
	conMgr_->looper_=this;
  threadnum_=tnum;
  threads_=new ezIoThread[tnum];
  for(int i=0;i<tnum;++i)
  {
    threads_[i].settid(i+1);
    threads_[i].setlooper(this);
    threads_[i].Start();
  }
	return 0;
}

int net::ezEventLoop::shutdown()
{
	return 0;
}

int net::ezEventLoop::serveOnPort(int port)
{
  SOCKET s=CreateTcpServer(port,nullptr);
  if(s==INVALID_SOCKET)
    return -1;
  ezIoThread* thread=chooseThread();
  thread->add(s,ezUUID::instance()->uuid(),new ezListenerFd,ezNetRead);
  return 0;
}

// void net::ezEventLoop::n2oCrossEvent(ezCrossEventData& ev)
// {
//   toApp_.enqueue(ev);
// }
// 
// void net::ezEventLoop::n2oCloseFd(int fd,uint64_t uuid)
// {
// 	ezCrossEventData data;
// 	data.fd_=fd;
// 	data.event_=ezCrossClose;
// 	data.uuid_=uuid;
// 	n2oCrossEvent(data);
// }
// 
// 
// void net::ezEventLoop::n2oError(int fd,uint64_t uuid)
// {
// 	ezCrossEventData data;
// 	data.fd_=fd;
// 	data.event_=ezCrossError;
// 	data.uuid_=uuid;
// 	n2oCrossEvent(data);
// }

net::ezEventLoop::ezEventLoop()
{
	hander_=nullptr;
	conMgr_=nullptr;
  threadnum_=0;
}

net::ezEventLoop::~ezEventLoop()
{
	if(hander_) delete hander_;
	if(conMgr_) delete conMgr_;
}

uint64_t net::ezUUID::uuid()
{
	return suuid_.Add(1);
}

void net::ezEventLoop::o2nCloseFd(int tid,int fd,uint64_t uuid)
{
  if(tid<=0||tid>threadnum_)
    return;
  ezCrossEventData ev;
  ev.fromtid_=0;
  ev.fd_=fd;
  ev.uuid_=uuid;
  ev.event_=ezCrossClose;
  notify(&threads_[tid],ev);
}

void net::ezEventLoop::o2nConnectTo(uint64_t uuid,const char* toip,int toport)
{
  ezCrossEventData ev;
  ev.fromtid_=0;
  ev.uuid_=uuid;
  ev.event_=ezCrossOpen;
  ezMsg* msg=new ezMsg;
  ezMsgInitSize(msg,sizeof(int)+sizeof(uint16_t)+strlen(toip));
  ev.msg_=msg;
  base::ezBufferWriter writer((char*)ezMsgData(msg),ezMsgSize(msg));
  writer.Write(toport);
  writer.Write(uint16_t(strlen(toip)));
  writer.WriteBuffer(toip,int(strlen(toip)));
  msg->size_=writer.GetUsedSize();
  notify(chooseThread(),ev);
}

void net::ezEventLoop::sendMsg(int tid,int fd,ezMsg& msg)
{
  assert(tid>0&&tid<=threadnum_);
  ezIoThread& th=threads_[tid];
}

net::ezIoThread* net::ezEventLoop::chooseThread()
{
  return &threads_[rand()%threadnum_];
}

void net::ezEventLoop::notify(net::ezIoThread* thread,net::ezCrossEventData& data)
{
  thread->notify(data);
}

void net::ezEventLoop::loop()
{
  ezCrossEventData data;
  for(int i=0;i<threadnum_;++i)
  {
    while(threads_[i].mainpull(data))
    {
      switch(data.event_)
      {
      case ezCrossOpen:
        hander_->onOpen(this,data.fd_,data.uuid_,data.fromtid_);
        break;
      case ezCrossClose:
        hander_->onClose(this,data.fd_,data.uuid_,data.fromtid_);
        break;
      case ezCrossError:
        hander_->onError(this,data.fd_,data.uuid_,data.fromtid_);
        break;
        //       case ezCrossData:
        //         {
        //           assert(evd->msg_);
        //           hander_->onData(this,data.fd_,data.uuid_,data.msg_);
        //         }
        break;
      default: break;
      }
      ezCloseCrossEventData(data);
    }
  }
}

net::ezCrossEventData::ezCrossEventData():fromtid_(-1),fd_(-1),uuid_(0),event_(ezCrossNone),msg_(nullptr){}
void net::ezCloseCrossEventData(ezCrossEventData& ev)
{
  if(ev.msg_)
    ezMsgFree(ev.msg_);
}