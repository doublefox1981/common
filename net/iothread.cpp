#include "../base/portable.h"
#include "../base/memorystream.h"
#include "iothread.h"

net::ezIoThread::ezIoThread():tid_(0),load_(0),maxfd_(-1)
{
  poller_=new ezSelectPoller(this);
  msgqueue_=new MsgWarperQueue;
  poller_->addFd(evqueue_->getfd(),ezNetRead);
}

net::ezIoThread::~ezIoThread()
{
  if(poller_)
    delete poller_;
  if(evqueue_)
    delete evqueue_;
  if(msgqueue_)
    delete msgqueue_;
}

void net::ezIoThread::settid(int tid)
{
  tid_=tid;
}

void net::ezIoThread::OnEvent(ezIoThread* io,int fd,int event,uint64_t uuid)
{
  if(event&ezNetRead)
  {
    ezThreadEvent ev;
    while(evqueue_->recv(ev))
    {
      ev.hander_->ProcessEvent(ev);
    }
  }
}

uint64_t net::ezIoThread::add(int fd,uint64_t uuid,ezFd *ezfd,int event)
{
  ++load_;
  ezFdData* data = new ezFdData();
  data->uuid_=uuid;
  data->fd_=fd;
  data->ezfd_=ezfd;
  data->event_|=event;
  if(fd>maxfd_)
  {
    fds_.resize(fd+1);
    maxfd_ = fd;
  }
  if(fds_[fd])
  {
    delete fds_[fd];
    fds_[fd]=nullptr;
  }
  fds_[fd] = data;
  poller_->addFd(fd,event);
  return data->uuid_;
}

int net::ezIoThread::del(int fd)
{
  assert(fd<=maxfd_);
  assert(fd>=0);
  if(!fds_[fd])
    return -1;
  --load_;
  int event=fds_[fd]->event_;
  delete fds_[fd];
  fds_[fd]=nullptr;
  poller_->delFd(fd,event);
  net::CloseSocket(fd);
  return 0;
}

int net::ezIoThread::mod(int fd,int event,bool set)
{
  assert(fd>=0);
  assert(fd<=maxfd_);
  assert(fds_[fd]);
  assert(fds_[fd]->event_!=ezNetNone);
  poller_->modFd(fd,event,fds_[fd]->event_,set);
  if(set)
    fds_[fd]->event_|=event;
  else
    fds_[fd]->event_&=~event;
  return 0;
}

void net::ezIoThread::Run()
{
  while(!mbExit)
  {
    poller_->poll();
    for(size_t s=0;s<firedfd_.size();++s)
    {
      ezFdData* ezD=firedfd_[s];
      ezFd* fd=fds_[ezD->fd_]->ezfd_;
      assert(fd);
      fd->onEvent(this,ezD->fd_,ezD->event_,ezD->uuid_);
      delete ezD;
    }
    firedfd_.clear();
  }
}

net::ezFdData* net::ezIoThread::getfd(int i)
{
  if(i<0||i>=int(fds_.size()))
    return nullptr;
  return fds_[i];
}

void net::ezIoThread::pushfired(ezFdData* fd)
{
  firedfd_.push_back(fd);
}

bool net::ezIoThread::mainpull(ezCrossEventData& data)
{
  return mainqueue_->recv(data);
}

void net::ezIoThread::pushmain(ezCrossEventData& data)
{
  return mainqueue_->send(data);
}

bool net::ezIoThread::pushmsg(ezMsgWarper* msg)
{
  msgqueue_->enqueue(*msg);
  return true;
}

void net::ezIoThread::sendMsg(int fd,ezMsg& msg)
{
  if(fd<=0||fd>=int(fds_.size())||!fds_[fd])
  {
    ezMsgFree(&msg);
    return;
  }

  fds_[fd]->ezfd_->sendMsg(msg);
}

bool net::ezIoThread::pullwarpmsg(ezMsgWarper& warpmsg)
{
  return msgqueue_->try_dequeue(warpmsg);
}

void net::ezIoThread::ProcessEvent(ezThreadEvent& ev)
{

}
