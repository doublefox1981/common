#include "../base/portable.h"
#include "../base/memorystream.h"
#include "event.h"
#include "fd.h"
#include "poller.h"
#include "iothread.h"
#include "socket.h"

net::ezIoThread::ezIoThread():tid_(0)
{
  poller_=new ezSelectPoller(this);
  ioqueue_=new CrossEvQueue;
  mainqueue_=new CrossEvQueue;
  add(ioqueue_->getfd(),ezUUID::instance()->uuid(),this,ezNetRead);
}

net::ezIoThread::~ezIoThread()
{
  if(poller_)
    delete poller_;
  if(ioqueue_)
    delete ioqueue_;
  if(mainqueue_)
    delete mainqueue_;
}

void net::ezIoThread::settid(int tid)
{
  tid_=tid;
}

void net::ezIoThread::notify(ezCrossEventData& ev)
{
  ioqueue_->send(ev);
}

void net::ezIoThread::onEvent(ezIoThread* io,int fd,int event,uint64_t uuid)
{
  if(event&ezNetRead)
  {
    ezCrossEventData data;
    while(ioqueue_->recv(data))
    {
      switch(event)
      {
      case ezCrossPollout:
        mod(fd,ezNetRead,true);
        break;
      case ezCrossOpen:
        {
          assert(data.msg_);
          int port=0;
          base::ezBufferReader reader((char*)ezMsgData(data.msg_),ezMsgSize(data.msg_));
          reader.Read(port);
          uint16_t iplen=0;
          reader.Read(iplen);
          std::string ip;
          reader.ReadString(ip,iplen);
          int sock=ConnectTo(ip.c_str(),port);
          if(sock>0)
          {
            add(sock,0,new ezClientFd,ezNetRead);
            ezCrossEventData sendEv;
            sendEv.fromtid_=gettid();
            sendEv.fd_=sock;
            sendEv.uuid_=data.uuid_;
            sendEv.event_=ezCrossOpen;
            mainqueue_->send(sendEv);
          }
        }
        break;
      default:
        break;
      }
    }
    ezCloseCrossEventData(data);
  }
}

uint64_t net::ezIoThread::add(int fd,uint64_t uuid,ezFd *ezfd,int event)
{
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
