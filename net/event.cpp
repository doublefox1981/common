#include "event.h"
#include "socket.h"
#include "poller.h"
#include "connection.h"
#include <assert.h>

uint64_t net::ezEventLoop::suuid_=0;

int net::ezEventLoop::init(ezPoller* poller,ezHander* hander)
{
	poller_=poller;
	hander_=hander;
	maxfd_=-1;
	INIT_LIST_HEAD(&crossEv_);
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
	{
		add(s,new ezListenerFd,ezNetRead);
		return 0;
	}
	else
		return -1;
}

uint64_t net::ezEventLoop::add(int fd, ezFd *ezfd,int event)
{
	ezNetEventData* data = new ezNetEventData();
	data->uuid_=++suuid_;
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
	return data->uuid_;
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

void net::ezEventLoop::netEventLoop()
{
	poller_->poll();
	for(size_t s=0;s<fired_.size();++s)
	{
		ezNetEventData* ezD=fired_[s];
		events_[ezD->fd_]->ezfd_->onEvent(this,ezD->fd_,ezD->event_,ezD->uuid_);
		delete ezD;
	}
	fired_.clear();
}

void net::ezEventLoop::crossEventLoop()
{
	LIST_HEAD(tmplst);
	{
		// 复制，只需少数指针赋值
		base::Locker lock(&mutexCrossEv_);
		list_splice_init(&crossEv_,&tmplst);
	}
	list_head *iter,*next;
	list_for_each_safe(iter,next,&tmplst)
	{
		ezCrossEventData* evd=list_entry(iter,ezCrossEventData,evlst_);
		switch(evd->event_)
		{
		case ezCrossOpen:
			{
				hander_->onOpen(this,evd->fd_,evd->uuid_);
			}
			break;
		case ezCrossClose:
			{
				hander_->onClose(this,evd->fd_,evd->uuid_);
			}
			break;
		case ezCrossData:
			{
				assert(evd->msg_);
				hander_->onData(this,evd->fd_,evd->uuid_,evd->msg_);
			}
			break;
		}
		delete evd;
	}
}

void net::ezEventLoop::postCrossEvent(ezCrossEventData* ev)
{
	base::Locker lock(&mutexCrossEv_);
	list_add_tail(&ev->evlst_,&crossEv_);
}

void net::ezEventLoop::postCloseFd(int fd,uint64_t uuid)
{
	ezCrossEventData* data=new ezCrossEventData;
	data->fd_=fd;
	data->event_=ezCrossClose;
	data->uuid_=uuid;
	postCrossEvent(data);
}

void net::ezEventLoop::postNewFd(int fd,uint64_t uuid)
{
	ezCrossEventData* data=new ezCrossEventData;
	data->fd_=fd;
	data->event_=ezCrossOpen;
	data->uuid_=uuid;
	postCrossEvent(data);
}

net::ezEventLoop::ezEventLoop()
{
	poller_=NULL;
	hander_=NULL;
	maxfd_=-1;
}

net::ezEventLoop::~ezEventLoop()
{
	if(poller_) delete poller_;
	if(hander_) delete hander_;
}

net::ezNetEventData::~ezNetEventData()
{
	if(ezfd_)
		delete ezfd_;
}

net::ezNetEventData::ezNetEventData():fd_(-1),event_(ezNetNone),ezfd_(NULL),uuid_(0)
{

}

net::ezCrossEventData::ezCrossEventData():fd_(-1),uuid_(0),event_(ezCrossNone),msg_(NULL)
{

}

net::ezCrossEventData::~ezCrossEventData()
{
	if(msg_)
		delete msg_;
}
