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

int net::ezEventLoop::init(ezIHander* hander,ezConnectionMgr* mgr,int tnum)
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
  evqueues_=new (ThreadEvQueue*)[tnum+1];
  evqueues_[0]=mainevqueue_;
  for(int i=1;i<=tnum;++i)
  {
    evqueues_[i]=getThread(i)->getevqueue();
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

net::ezEventLoop::ezEventLoop()
{
	hander_=nullptr;
	conMgr_=nullptr;
  threadnum_=0;
  mainevqueue_=new ThreadEvQueue;
}

net::ezEventLoop::~ezEventLoop()
{
	if(hander_) delete hander_;
	if(conMgr_) delete conMgr_;
  if(mainevqueue_) delete mainevqueue_; // TODO: clean
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
  notify(&threads_[tid-1],ev);
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
  notify(&threads_[0],ev);
}

void net::ezEventLoop::sendMsg(int tid,int fd,ezMsg& msg)
{
  assert(tid>0&&tid<=threadnum_);
  ezIoThread& th=threads_[tid-1];
  th.sendMsg(fd,msg);
  ezCrossEventData ev;
  ev.fromtid_=0;
  ev.fd_=fd;
  ev.uuid_=0;
  ev.event_=ezCrossPollout;
  notify(&th,ev);
}

void net::ezEventLoop::OccerEvent(int tid,ezThreadEvent& ev)
{
  if(tid<=threadnum_)
    evqueues_[tid]->send(ev);
}

ezIoThread* net::ezEventLoop::GetThread(int idx)
{
  if(idx>0&&idx<=threadnum_)
    return &threads_[idx];
  else
    return NULL;
}

net::ezIoThread* net::ezEventLoop::ChooseThread()
{
  int idx=0;
  int minload=threads_[idx].getload();
  for(int i=0;i<threadnum_;++i)
  {
    if(threads_[i].getload()<minload)
    {
      minload=threads_[i].getload();
      idx=i;
    }
  }
  return &threads_[idx];
}

void net::ezEventLoop::notify(net::ezIoThread* thread,net::ezCrossEventData& data)
{
  thread->notify(data);
}

void net::ezEventLoop::loop()
{
  ezCrossEventData data;
  ezMsgWarper warpmsg;
  for(int i=0;i<threadnum_;++i)
  {
    ezIoThread* thread=&threads_[i];
    while(threads_[i].mainpull(data))
    {
      switch(data.event_)
      {
      case ezCrossOpen:
        hander_->onOpen(thread,data.fd_,data.uuid_,data.fromtid_);
        break;
      case ezCrossClose:
        hander_->onClose(thread,data.fd_,data.uuid_);
        break;
      case ezCrossError:
        hander_->onError(thread,data.fd_,data.uuid_);
        break;
      default: break;
      }
      ezCloseCrossEventData(data);
    }
    while(thread->pullwarpmsg(warpmsg))
    {
      hander_->onData(thread,0,warpmsg.uuid_,&warpmsg.msg_);
      ezMsgFree(&warpmsg.msg_);
    }
  }
}

net::ezCrossEventData::ezCrossEventData():fromtid_(-1),fd_(-1),uuid_(0),event_(ezCrossNone),msg_(nullptr){}
void net::ezCloseCrossEventData(ezCrossEventData& ev)
{
  if(ev.msg_)
    ezMsgFree(ev.msg_);
}

net::ezThreadEventHander::ezThreadEventHander(ezEventLoop* loop,int tid):looper_(loop),tid_(tid)
{}

void net::ezThreadEventHander::OccurEvent(ezThreadEvent& ev)
{
  looper_->OccerEvent(tid_,ev);
}
