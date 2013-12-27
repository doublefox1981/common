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

int net::ezEventLoop::Initialize(ezIConnnectionHander* hander,ezIDecoder* decoder,ezIEncoder* encoder,int tnum)
{
	hander_=hander;
  decoder_=decoder;
  encoder_=encoder;
  threadnum_=tnum;
  threads_=new ezIoThread*[tnum];
  for(int i=0;i<tnum;++i)
  {
    threads_[i]=new ezIoThread(this,i+1);
    threads_[i]->Start();
  }
  evqueues_=new ThreadEvQueue*[tnum+1];
  evqueues_[0]=mainevqueue_;
  for(int i=1;i<=tnum;++i)
  {
    evqueues_[i]=GetThread(i)->GetEvQueue();
  }
	return 0;
}

int net::ezEventLoop::shutdown()
{
	return 0;
}

int net::ezEventLoop::ServeOnPort(int port)
{
  SOCKET s=CreateTcpServer(port,nullptr);
  if(s==INVALID_SOCKET)
    return -1;
  ezIoThread* thread=ChooseThread();
  ezThreadEvent ev;
  ev.type_=ezThreadEvent::NEW_SERVICE;
  ev.hander_=new ezListenerFd(this,thread,s);
  ev.hander_->OccurEvent(ev);
  return 0;
}

int net::ezEventLoop::ConnectTo(const std::string& ip,int port,int64_t userdata,int32_t reconnect)
{
  ezIoThread* thread=ChooseThread();
  ezThreadEvent ev;
  ev.type_=ezThreadEvent::NEW_CONNECTTO;
  ezConnectToFd* conn=new ezConnectToFd(this,thread,userdata,reconnect);
  conn->SetIpPort(ip,port);
  ev.hander_=conn;
  ev.hander_->OccurEvent(ev);
  return 0;
}

net::ezEventLoop::ezEventLoop()
{
	hander_=nullptr;
  threadnum_=0;
  mainevqueue_=new ThreadEvQueue;
}

net::ezEventLoop::~ezEventLoop()
{
	if(hander_) delete hander_;
  if(decoder_) delete decoder_;
  if(encoder_) delete encoder_;
  if(mainevqueue_) delete mainevqueue_; // TODO: clean
}

uint64_t net::ezUUID::uuid()
{
	return suuid_.Add(1);
}

void net::ezEventLoop::OccerEvent(int tid,ezThreadEvent& ev)
{
  assert(tid<=threadnum_);
  evqueues_[tid]->Send(ev);
}

net::ezIoThread* net::ezEventLoop::GetThread(int idx)
{
  if(idx>0&&idx<=threadnum_)
    return threads_[idx-1];
  else
    return NULL;
}

net::ezIoThread* net::ezEventLoop::ChooseThread()
{
  int idx=0;
  int minload=threads_[idx]->GetLoad();
  for(int i=0;i<threadnum_;++i)
  {
    if(threads_[i]->GetLoad()<minload)
    {
      minload=threads_[i]->GetLoad();
      idx=i;
    }
  }
  return threads_[idx];
}

void net::ezEventLoop::Loop()
{
  ezThreadEvent ev;
  while(mainevqueue_->Recv(ev))
  {
    ev.hander_->ProcessEvent(ev);
  }
}

void net::ezEventLoop::AddConnection(ezConnection* con)
{
  assert(conns_.find(con)==conns_.end());
  conns_.insert(con);
}

void net::ezEventLoop::DelConnection(ezConnection* con)
{
  auto iter=conns_.find(con);
  assert(iter!=conns_.end());
  conns_.erase(iter);
}

int net::ezEventLoop::GetConnectionNum()
{
  return conns_.size();
}

net::ezThreadEventHander::ezThreadEventHander(ezEventLoop* loop,int tid):looper_(loop),tid_(tid)
{}

void net::ezThreadEventHander::OccurEvent(ezThreadEvent& ev)
{
  ev.hander_=this;
  looper_->OccerEvent(tid_,ev);
}
