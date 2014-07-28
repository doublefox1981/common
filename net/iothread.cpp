#include "../base/portable.h"
#include "../base/memorystream.h"
#include "../base/eztime.h"
#include "iothread.h"

net::IoThread::IoThread(EventLoop* loop,int tid)
  :load_(0),
  ThreadEventHander(loop,tid)
{
  evqueue_=new ThreadEvQueue;
  poller_=create_poller();
  poller_->add_fd(evqueue_->get_fd(),this);
  poller_->set_poll_in(evqueue_->get_fd());
}

net::IoThread::~IoThread()
{
  if(poller_)
    delete poller_;
  if(evqueue_)
    delete evqueue_;
}

void net::IoThread::AddFlashedFd(ezIFlashedFd* ffd)
{
  flashedfd_.push_back(ffd);
}

void net::IoThread::DelFlashedFd(ezIFlashedFd* ffd)
{
  for(auto iter=flashedfd_.begin();iter!=flashedfd_.end();)
  {
    if(*iter==ffd)
      iter=flashedfd_.erase(iter);
    else
      ++iter;
  }
}

void net::IoThread::handle_in_event()
{
  ThreadEvent ev;
  while(evqueue_->recv(ev))
  {
    ev.hander_->process_event(ev);
  }
}

void net::IoThread::run()
{
  while(!exit_)
  {
    poller_->poll();
    base::sleep(1);
  }
}

void net::IoThread::process_event(ThreadEvent& ev)
{
  switch(ev.type_)
  {
  case ThreadEvent::STOP_FLASHEDFD:
    {
      for(size_t i=0;i<flashedfd_.size();++i)
        flashedfd_[i]->close();
      flashedfd_.clear();
    }
    break;
  case ThreadEvent::STOP_THREAD:
    stop();
    break;
  default: break;
  }
}
