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
    virtual void on_open(Connection* conn){conn->active_close();}
    virtual void on_close(Connection* conn){}
    virtual void on_data(Connection* conn,Msg* msg){}
  };
}

namespace net
{
  // 关闭系统时使用
  void static CleanThreadEvQueue(ThreadEvQueue* evq)
  {
    ThreadEvent ev;
    while(evq->recv(ev))
    {
      switch(ev.type_)
      {
      case ThreadEvent::NEW_SERVICE:
      case ThreadEvent::NEW_CONNECTTO:
      case ThreadEvent::NEW_FD:
      case ThreadEvent::NEW_CONNECTION:
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

int net::EventLoop::initialize(IConnnectionHander* hander,IDecoder* decoder,IEncoder* encoder,int tnum)
{
	hander_=hander;
  decoder_=decoder;
  encoder_=encoder;
  threadnum_=tnum;
  threads_=new IoThread*[tnum];
  for(int i=0;i<tnum;++i)
  {
    threads_[i]=new IoThread(this,i+1);
    threads_[i]->start();
  }
  evqueues_=new ThreadEvQueue*[tnum+1];
  evqueues_[0]=mainevqueue_;
  for(int i=1;i<=tnum;++i)
  {
    evqueues_[i]=get_thread(i)->GetEvQueue();
  }
	return 0;
}

int net::EventLoop::serve_on_port(int port)
{
  SOCKET s=CreateTcpServer(port,nullptr);
  if(s==INVALID_SOCKET)
    return -1;
  IoThread* thread=choose_thread();
  ThreadEvent ev;
  ev.type_=ThreadEvent::NEW_SERVICE;
  ev.hander_=new ezListenerFd(this,thread,s);
  ev.hander_->occur_event(ev);
  return 0;
}

int net::EventLoop::connect_to(const std::string& ip,int port,int64_t userdata,int32_t reconnect)
{
  IoThread* thread=choose_thread();
  ThreadEvent ev;
  ev.type_=ThreadEvent::NEW_CONNECTTO;
  ezConnectToFd* conn=new ezConnectToFd(this,thread,userdata,reconnect);
  conn->SetIpPort(ip,port);
  ev.hander_=conn;
  ev.hander_->occur_event(ev);
  return 0;
}

int net::EventLoop::shutdown()
{
  shutdown_=true;
  hander_=closehander_;
  for(int i=0;i<threadnum_;++i)
  {
    ThreadEvent ev;
    ev.type_=ThreadEvent::STOP_FLASHEDFD;
    threads_[i]->occur_event(ev);
  }
  for(auto iter=conns_.begin();iter!=conns_.end();++iter)
  {
    (*iter)->active_close();
  }
  while(!conns_.empty())
  {
    loop();
    base::sleep(1);
  }
  for(int i=0;i<threadnum_;++i)
  {
    ThreadEvent ev;
    ev.type_=ThreadEvent::STOP_THREAD;
    threads_[i]->occur_event(ev);
  }
  for(int i=0;i<threadnum_;++i)
    threads_[i]->join();
  return 1;
}

uint64_t net::UUID::uuid()
{
	return suuid_.Add(1);
}

void net::EventLoop::occer_event(int tid,ThreadEvent& ev)
{
  assert(tid<=threadnum_);
  evqueues_[tid]->send(ev);
}

net::IoThread* net::EventLoop::get_thread(int idx)
{
  if(idx>0&&idx<=threadnum_)
    return threads_[idx-1];
  else
    return NULL;
}

net::IoThread* net::EventLoop::choose_thread()
{
  int idx=0;
  int minload=threads_[idx]->get_load();
  for(int i=0;i<threadnum_;++i)
  {
    if(threads_[i]->get_load()<minload)
    {
      minload=threads_[i]->get_load();
      idx=i;
    }
  }
  return threads_[idx];
}

void net::EventLoop::loop()
{
  ThreadEvent ev;
  while(mainevqueue_->recv(ev))
  {
    ev.hander_->process_event(ev);
  }
}

void net::EventLoop::add_connection(Connection* con)
{
  if(shutdown_)
    return;
  assert(conns_.find(con)==conns_.end());
  conns_.insert(con);
}

void net::EventLoop::del_connection(Connection* con)
{
  auto iter=conns_.find(con);
  assert(iter!=conns_.end());
  conns_.erase(iter);
}

int net::EventLoop::get_connection_num()
{
  return conns_.size();
}

int net::EventLoop::get_buffer_size()
{
  return buffersize_;
}

void net::EventLoop::set_buffer_size(int s)
{
  buffersize_=s;
}

net::ThreadEventHander::ThreadEventHander(EventLoop* loop,int tid):looper_(loop),tid_(tid)
{}

void net::ThreadEventHander::occur_event(ThreadEvent& ev)
{
  ev.hander_=this;
  looper_->occer_event(tid_,ev);
}

void  net::net_initialize()
{
  net::InitNetwork();
}

net::EventLoop* net::create_event_loop(IConnnectionHander* hander,IDecoder* decoder,IEncoder* encoder,int tnum)
{
  net::EventLoop* ev=new net::EventLoop;
  ev->initialize(hander,decoder,encoder,tnum);
  return ev;
}

void net::set_msg_buffer_size(EventLoop* loop,int size)
{
  loop->set_buffer_size(size);
}

void net::destroy_event_loop(EventLoop* ev)
{
  ev->shutdown();
  delete ev;
}

int net::serve_on_port(EventLoop* ev,int port)
{
  return ev->serve_on_port(port);
}

int net::connect(EventLoop* ev,const char* ip,int port,int64_t userdata,int32_t reconnect)
{
  return ev->connect_to(ip,port,userdata,reconnect);
}

void net::event_process(net::EventLoop* ev)
{
  ev->loop();
}

void net::close_connection(net::Connection* conn)
{
  conn->active_close();
}

void net::msg_send(Connection* conn,Msg* msg)
{
  conn->send_msg(*msg);
}

int64_t net::conection_user_data(Connection* conn)
{
  return conn->get_user_data();
}