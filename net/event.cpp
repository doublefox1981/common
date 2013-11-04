#include "event.h"
#include "socket.h"
#include "poller.h"
#include "connection.h"
#include "../base/memorystream.h"
#include "../base/thread.h"
#include <assert.h>

int net::ezEventLoop::init(ezPoller* poller,ezHander* hander)
{
	poller_=poller;
	hander_=hander;
	return 0;
}

int net::ezEventLoop::shutdown()
{
	return 0;
}

int net::ezEventLoop::serveOnPort(int port)
{
	SOCKET s=CreateTcpServer(port,nullptr);
	if(s!=INVALID_SOCKET)
	{
		add(s,uuid(),new ezListenerFd,ezNetRead);
		return 0;
	}
	else
		return -1;
}

uint64_t net::ezEventLoop::add(int fd,uint64_t uuid,ezFd *ezfd,int event)
{
	ezFdData* data = new ezFdData();
	data->uuid_=uuid;
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
	events_[fd] = nullptr;
	poller_->delFd(fd,event);
	net::CloseSocket(fd);
	return 0;
}

int net::ezEventLoop::mod(int fd,int event,bool set)
{
	assert(fd>=0);
	assert(fd<=maxfd_);
	assert(events_[fd]);
	assert(events_[fd]->event_!=ezNetNone);
	if(set)
		events_[fd]->event_|=event;
	else
		events_[fd]->event_&=~event;
	poller_->modFd(fd,event,set);
	return 0;
}

void net::ezEventLoop::processEv()
{
	LIST_HEAD(tmplst);
	{
		base::Locker lock(&mutexO2NCrossEv_);
		list_splice_init(&crossO2NEv_,&tmplst);
	}
	list_head *iter,*next;
	list_for_each_safe(iter,next,&tmplst)
	{
		ezCrossEventData* ev=list_entry(iter,ezCrossEventData,evlst_);
		if(ev->event_==ezCrossClose)
		{
			n2oCloseFd(ev->fd_,ev->uuid_);
			del(ev->fd_);
		}
		else if(ev->event_==ezCrossOpen)
		{
			int port=0;
			base::ezBufferReader reader(ev->msg_->data_,ev->msg_->size_);
			reader.Read(port);
			uint16_t iplen=0;
			reader.Read(iplen);
			std::string ip;
			reader.ReadString(ip,iplen);
			int sock=ConnectTo(ip.c_str(),port);
			if(sock>0)
			{
				add(sock,ev->uuid_,new ezClientFd,ezNetRead);
			}
			ezCrossEventData* sendEv=new ezCrossEventData;
			sendEv->fd_=sock;
			sendEv->uuid_=ev->uuid_;
			sendEv->event_=ezCrossOpen;
			n2oCrossEvent(sendEv);
		}
		delete ev;
	}
}

void net::ezEventLoop::processMsg()
{
	LIST_HEAD(tmplst);
	{
		base::Locker lock(&mutexSendQueue_);
		list_splice_init(&sendMsgQueue_,&tmplst);
	}
	list_head *iter,*next;
	list_for_each_safe(iter,next,&tmplst)
	{
		ezSendBlock* blk=list_entry(iter,ezSendBlock,lst_);
		if(blk->fd_<(int)events_.size()&&blk->fd_>0)
		{
			ezFdData* evd=events_[blk->fd_];
			if(evd&&evd->ezfd_)
			{
				evd->ezfd_->sendMsg(blk);
				blk=nullptr;
			}
		}
		if(blk) delete blk;
	}
	for(size_t s=0;s<events_.size();++s)
	{
		ezFdData* evd=events_[s];
		if(evd&&evd->ezfd_)
		{
			if(evd->ezfd_->formatMsg()>0)
				mod(evd->fd_,ezNetWrite,true);
		}
	}
}

void net::ezEventLoop::netEventLoop()
{
	poller_->poll();
	for(size_t s=0;s<fired_.size();++s)
	{
		ezFdData* ezD=fired_[s];
		events_[ezD->fd_]->ezfd_->onEvent(this,ezD->fd_,ezD->event_,ezD->uuid_);
		delete ezD;
	}
	fired_.clear();
	processEv();
	processMsg();
}

void net::ezEventLoop::crossEventLoop()
{
	LIST_HEAD(tmplst);
	{
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
			hander_->onOpen(this,evd->fd_,evd->uuid_);
			break;
		case ezCrossClose:
			hander_->onClose(this,evd->fd_,evd->uuid_);
			break;
		case ezCrossError:
			hander_->onError(this,evd->fd_,evd->uuid_);
			break;
		case ezCrossData:
			{
				assert(evd->msg_);
				hander_->onData(this,evd->fd_,evd->uuid_,evd->msg_);
			}
			break;
		default: break;
		}
		delete evd;
	}
}

void net::ezEventLoop::n2oCrossEvent(ezCrossEventData* ev)
{
	base::Locker lock(&mutexCrossEv_);
	list_add_tail(&ev->evlst_,&crossEv_);
}

void net::ezEventLoop::n2oCloseFd(int fd,uint64_t uuid)
{
	ezCrossEventData* data=new ezCrossEventData;
	data->fd_=fd;
	data->event_=ezCrossClose;
	data->uuid_=uuid;
	n2oCrossEvent(data);
}

void net::ezEventLoop::n2oNewFd(int fd,uint64_t uuid)
{
	ezCrossEventData* data=new ezCrossEventData;
	data->fd_=fd;
	data->event_=ezCrossOpen;
	data->uuid_=uuid;
	n2oCrossEvent(data);
}

void net::ezEventLoop::n2oError(int fd,uint64_t uuid)
{
	ezCrossEventData* data=new ezCrossEventData;
	data->fd_=fd;
	data->event_=ezCrossError;
	data->uuid_=uuid;
	n2oCrossEvent(data);
}

net::ezEventLoop::ezEventLoop()
{
	poller_=nullptr;
	hander_=nullptr;
	conMgr_=new ezConnectionMgr;
	maxfd_=-1;
	suuid_.Set(1);
	INIT_LIST_HEAD(&crossEv_);
	INIT_LIST_HEAD(&crossO2NEv_);
	INIT_LIST_HEAD(&sendMsgQueue_);
}

net::ezEventLoop::~ezEventLoop()
{
	if(poller_) delete poller_;
	if(hander_) delete hander_;
	if(conMgr_) delete conMgr_;
}

uint64_t net::ezEventLoop::uuid()
{
	return suuid_.Add(1);
}

void net::ezEventLoop::o2nCloseFd(int fd,uint64_t uuid)
{
	ezCrossEventData* ev=new ezCrossEventData;
	ev->fd_=fd;
	ev->uuid_=uuid;
	ev->event_=ezCrossClose;

	base::Locker lock(&mutexO2NCrossEv_);
	list_add_tail(&ev->evlst_,&crossO2NEv_);
}

void net::ezEventLoop::o2nConnectTo(uint64_t uuid,const char* toip,int toport)
{
	ezCrossEventData* ev=new ezCrossEventData;
	ev->uuid_=uuid;
	ev->event_=ezCrossOpen;
	ezNetPack* msg=new ezNetPack(128);
	ev->msg_=msg;
	base::ezBufferWriter writer(msg->data_,msg->capacity_);
	writer.Write(toport);
	writer.Write(uint16_t(strlen(toip)));
	writer.WriteBuffer(toip,int(strlen(toip)));
	msg->size_=writer.GetUsedSize();

	base::Locker lock(&mutexO2NCrossEv_);
	list_add_tail(&ev->evlst_,&crossO2NEv_);
}

void net::ezEventLoop::sendMsg(int fd,ezNetPack* msg)
{
	ezSendBlock* blk=new ezSendBlock;
	blk->fd_=fd;
	blk->pack_=msg;

	base::Locker lock(&mutexSendQueue_);
	list_add_tail(&blk->lst_,&sendMsgQueue_);
}

net::ezFdData::~ezFdData()
{
	if(ezfd_)
		delete ezfd_;
}

net::ezFdData::ezFdData():fd_(-1),event_(ezNetNone),ezfd_(nullptr),uuid_(0)
{

}

net::ezCrossEventData::ezCrossEventData():fd_(-1),uuid_(0),event_(ezCrossNone),msg_(nullptr)
{

}

net::ezCrossEventData::~ezCrossEventData()
{
	if(msg_)
		delete msg_;
}
