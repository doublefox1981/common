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

}
