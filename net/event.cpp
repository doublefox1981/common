#include "event.h"
#include "net_interface.h"
#include "socket.h"
#include "poller.h"
#include "connection.h"
#include "fd.h"
#include "iothread.h"
#include "../base/memorystream.h"
#include "../base/thread.h"
#include "../base/logging.h"
#include "../base/util.h"
#include "../base/eztime.h"
#include <assert.h>

namespace net
{
  class ezCloseHander:public IConnnectionHander
  {
  public:
    virtual void on_open(Connection* conn){conn->ActiveClose();}
    virtual void on_close(Connection* conn){}
    virtual void on_data(Connection* conn,Msg* msg){}
  };
}

namespace net
{
  // 关闭系统时使用
  void static CleanThreadEvQueue(ThreadEvQueue* evq)
  {
    ezThreadEvent ev;
    while(evq->Recv(ev))
    {
      switch(ev.type_)
      {
      case ezThreadEvent::NEW_SERVICE:
      case ezThreadEvent::NEW_CONNECTTO:
      case ezThreadEvent::NEW_FD:
      case ezThreadEvent::NEW_CONNECTION:
        delete ev.hander_;
        break;
      default: break;
      }
    }
  }
}

net::EventLoop::EventLoop()
{
  shutdown_=false;
	hander_=nullptr;
  closehander_=new ezCloseHander;
  threadnum_=0;
  mainevqueue_=new ThreadEvQueue;
  buffersize_=16*1024;
}

net::EventLoop::~EventLoop()
{
  /* 由应用层销毁,因hander,decoder,encoder可被应用继承
	if(hander_) delete hander_;
  if(decoder_) delete decoder_;
  if(encoder_) delete encoder_;
  */
  if(closehander_) delete closehander_;
  for(int i=0;i<=threadnum_;++i)
    CleanThreadEvQueue(evqueues_[i]);
  for(int i=0;i<threadnum_;++i)
    delete threads_[i];
  delete [] evqueues_;
  delete [] threads_;
  if(mainevqueue_) delete mainevqueue_;
}

int net::EventLoop::Initialize(IConnnectionHander* hander,IDecoder* decoder,IEncoder* encoder,int tnum)
{
	hander_=hander;
  decoder_=decoder;
  encoder_=encoder;
  threadnum_=tnum;
  threads_=new ezIoThread*[tnum];
  for(int i=0;i<tnum;++i)
  {
    threads_[i]=new ezIoThread(this,i+1);
    threads_[i]->start();
  }
  evqueues_=new ThreadEvQueue*[tnum+1];
  evqueues_[0]=mainevqueue_;
  for(int i=1;i<=tnum;++i)
  {
    evqueues_[i]=GetThread(i)->GetEvQueue();
  }
	return 0;
}

int net::EventLoop::serve_on_port(int port)
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

int net::EventLoop::ConnectTo(const std::string& ip,int port,int64_t userdata,int32_t reconnect)
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

int net::EventLoop::Shutdown()
{
  shutdown_=true;
  hander_=closehander_;
  for(int i=0;i<threadnum_;++i)
  {
    ezThreadEvent ev;
    ev.type_=ezThreadEvent::STOP_FLASHEDFD;
    threads_[i]->OccurEvent(ev);
  }
  for(auto iter=conns_.begin();iter!=conns_.end();++iter)
  {
    (*iter)->ActiveClose();
  }
  while(!conns_.empty())
  {
    Loop();
    base::sleep(1);
  }
  for(int i=0;i<threadnum_;++i)
  {
    ezThreadEvent ev;
    ev.type_=ezThreadEvent::STOP_THREAD;
    threads_[i]->OccurEvent(ev);
  }
  for(int i=0;i<threadnum_;++i)
    threads_[i]->join();
  return 1;
}

uint64_t net::ezUUID::uuid()
{
	return suuid_.Add(1);
}

void net::EventLoop::OccerEvent(int tid,ezThreadEvent& ev)
{
  assert(tid<=threadnum_);
  evqueues_[tid]->Send(ev);
}

net::ezIoThread* net::EventLoop::GetThread(int idx)
{
  if(idx>0&&idx<=threadnum_)
    return threads_[idx-1];
  else
    return NULL;
}

net::ezIoThread* net::EventLoop::ChooseThread()
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

void net::EventLoop::Loop()
{
  ezThreadEvent ev;
  while(mainevqueue_->Recv(ev))
  {
    ev.hander_->ProcessEvent(ev);
  }
}

void net::EventLoop::AddConnection(Connection* con)
{
  if(shutdown_)
    return;
  assert(conns_.find(con)==conns_.end());
  conns_.insert(con);
}

void net::EventLoop::DelConnection(Connection* con)
{
  auto iter=conns_.find(con);
  assert(iter!=conns_.end());
  conns_.erase(iter);
}

int net::EventLoop::GetConnectionNum()
{
  return conns_.size();
}

int net::EventLoop::GetBufferSize()
{
  return buffersize_;
}

void net::EventLoop::SetBufferSize(int s)
{
  buffersize_=s;
}

net::ezThreadEventHander::ezThreadEventHander(EventLoop* loop,int tid):looper_(loop),tid_(tid)
{}

void net::ezThreadEventHander::OccurEvent(ezThreadEvent& ev)
{
  ev.hander_=this;
  looper_->OccerEvent(tid_,ev);
}

void  net::net_initialize()
{
  net::InitNetwork();
}

net::EventLoop* net::create_event_loop(IConnnectionHander* hander,IDecoder* decoder,IEncoder* encoder,int tnum)
{
  net::EventLoop* ev=new net::EventLoop;
  ev->Initialize(hander,decoder,encoder,tnum);
  return ev;
}

void net::set_msg_buffer_size(EventLoop* loop,int size)
{
  loop->SetBufferSize(size);
}

void net::destroy_event_loop(EventLoop* ev)
{
  ev->Shutdown();
  delete ev;
}

int net::serve_on_port(EventLoop* ev,int port)
{
  return ev->serve_on_port(port);
}

int net::connect(EventLoop* ev,const char* ip,int port,int64_t userdata,int32_t reconnect)
{
  return ev->ConnectTo(ip,port,userdata,reconnect);
}

void net::event_process(net::EventLoop* ev)
{
  ev->Loop();
}

void net::close_connection(net::Connection* conn)
{
  conn->ActiveClose();
}

void net::msg_send(Connection* conn,Msg* msg)
{
  conn->SendMsg(*msg);
}

int64_t net::conection_user_data(Connection* conn)
{
  return conn->GetUserdata();
}