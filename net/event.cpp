#include "event.h"
#include "socket.h"
#include "poller.h"
#include <assert.h>

int net::ezEventLoop::init(ezPoller* poller)
{
	poller_=poller;
	maxfd_=-1;
	return 0;
}

int net::ezEventLoop::shutdown()
{
	return 0;
}

int net::ezEventLoop::serveOnPort(int port)
{
	SOCKET s=CreateTcpServer(port,NULL);
	if(s!=INVALID_SOCKET)
		return add(s,new ezListenerFd,ezNetRead);
	else
		return -1;
}

int net::ezEventLoop::add(int fd, ezFd *ezfd,int event)
{
	ezNetEventData *data = new ezNetEventData();
	data->fd_=fd;
	data->ezfd_=ezfd;
	data->event_|=event;
	if(fd>maxfd_)
	{
		events_.resize(fd+1);
		maxfd_ = fd;
	}
	events_[fd] = data;
	poller_->addFd(fd,event);
	return 0;
}

int net::ezEventLoop::del(int fd)
{
	assert(fd<=maxfd_);
	assert(fd>=0);
	assert(events_[fd]);
	int event=events_[fd]->event_;
	delete events_[fd];
	events_[fd] = NULL;
	poller_->delFd(fd,event);
	return 0;
}

int net::ezEventLoop::modr(int fd, bool set)
{
	assert(fd >= 0);
	assert(fd <= maxfd_);
	assert(events_[fd]);

	if (set)
		events_[fd]->event_ |= ezNetRead;
	else
		events_[fd]->event_ &= ~ezNetRead;
	poller_->modFd(fd,ezNetNone);
	return 0;
}

int net::ezEventLoop::modw(int fd, bool set)
{
	assert(fd >= 0);
	assert(fd <= maxfd_);
	assert(events_[fd]);

	if (set)
		events_[fd]->event_ |= ezNetWrite;
	else
		events_[fd]->event_ &= ~ezNetWrite;
	poller_->modFd(fd,ezNetNone);
	return 0;
}

void net::ezEventLoop::loop()
{
	poller_->poll();
	for(size_t s=0;s<fired_.size();++s)
	{
		ezNetEventData* ezD=fired_[s];
		events_[ezD->fd_]->ezfd_->onEvent(this,ezD->fd_,ezD->event_);
		delete ezD;
	}
	fired_.clear();
}

net::ezNetEventData::~ezNetEventData()
{
	if(ezfd_)
		delete ezfd_;
}

net::ezNetEventData::ezNetEventData():fd_(-1),event_(ezNetNone),ezfd_(NULL)
{

}
