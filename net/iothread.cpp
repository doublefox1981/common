#include "../base/portable.h"
#include "../base/memorystream.h"
#include "../base/eztime.h"
#include "iothread.h"

net::ezIoThread::ezIoThread(ezEventLoop* loop,int tid)
  :load_(0),
  ezThreadEventHander(loop,tid)
{
  evqueue_=new ThreadEvQueue;
  poller_=CreatePoller();
  poller_->AddFd(evqueue_->GetFd(),this);
  poller_->SetPollIn(evqueue_->GetFd());
}

net::ezIoThread::~ezIoThread()
{
  // clear evqueue_
  if(poller_)
    delete poller_;
  if(evqueue_)
    delete evqueue_;
}

void net::ezIoThread::AddFlashedFd(ezIFlashedFd* ffd)
{
  flashedfd_.push_back(ffd);
}

void net::ezIoThread::DelFlashedFd(ezIFlashedFd* ffd)
{
  for(auto iter=flashedfd_.begin();iter!=flashedfd_.end();)
  {
    if(*iter=ffd)
      iter=flashedfd_.erase(iter);
    else
      ++iter;
  }
}

void net::ezIoThread::HandleInEvent()
{
  ezThreadEvent ev;
  while(evqueue_->Recv(ev))
  {
    ev.hander_->ProcessEvent(ev);
  }
}

void net::ezIoThread::Run()
{
  while(!mbExit)
  {
    poller_->Poll();
    base::ezSleep(1);
  }
}

void net::ezIoThread::ProcessEvent(ezThreadEvent& ev)
{
  switch(ev.type_)
  {
  case ezThreadEvent::STOP_FLASHEDFD:
    {
      for(size_t i=0;i<flashedfd_.size();++i)
        flashedfd_[i]->Close();
      flashedfd_.clear();
    }
    break;
  case ezThreadEvent::STOP_THREAD:
    Stop();
    break;
  default: break;
  }
}
