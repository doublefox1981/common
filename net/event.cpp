#include "event.h"
#include "socket.h"
#include "poller.h"
#include "connection.h"
#include "../base/memorystream.h"
#include "../base/thread.h"
#include "../base/logging.h"
#include "../base/util.h"
#include <assert.h>

int net::ezEventLoop::init(ezPoller* poller,ezHander* hander,ezConnectionMgr* mgr)
{
	poller_=poller;
	hander_=hander;
	conMgr_=mgr;
	conMgr_->looper_=this;
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

int net::ezEventLoop::del(int fd)
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
  delWriteFd(fd);
	return 0;
}

int net::ezEventLoop::mod(int fd,int event,bool set)
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

void net::ezEventLoop::addWriteFd(int fd)
{
  auto iter=writedfd_.find(fd);
  if(iter!=writedfd_.end())
    return;
  writedfd_.insert(fd);
}

void net::ezEventLoop::delWriteFd(int fd)
{
  auto iter=writedfd_.find(fd);
  if(iter!=writedfd_.end())
    writedfd_.erase(iter);
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
  MSVC_PUSH_DISABLE_WARNING(4018);
	LIST_HEAD(tmplst);
	{
		base::Locker lock(&mutexSendQueue_);
		list_splice_init(&sendMsgQueue_,&tmplst);
	}
	list_head *iter,*next;
	list_for_each_safe(iter,next,&tmplst)
	{
		ezSendBlock* blk=list_entry(iter,ezSendBlock,lst_);
		list_del(iter);
		if(blk->fd_<(int)fds_.size()&&blk->fd_>0)
		{
			ezFdData* evd=fds_[blk->fd_];
			if(evd&&evd->ezfd_)
			{
				evd->ezfd_->sendMsg(blk);
        addWriteFd(blk->fd_);
				blk=nullptr;
			}
		}
		if(blk) 
		{
			LOG_INFO("delete blk \n");
			delete blk;
		}
	}
	for(auto iter=writedfd_.begin();iter!=writedfd_.end();)
	{
    int fd=*iter;
    if(fd<=0||fd>=fds_.size())
      iter=writedfd_.erase(iter);
    else
      ++iter;
		ezFdData* evd=fds_[fd];
		if(evd&&evd->ezfd_)
		{
			if(evd->ezfd_->formatMsg()>0)
				mod(evd->fd_,ezNetWrite,true);
		}
	}
  MSVC_POP_WARNING();
}

void net::ezEventLoop::netEventLoop()
{
	poller_->poll();
	for(size_t s=0;s<firedfd_.size();++s)
	{
		ezFdData* ezD=firedfd_[s];
		fds_[ezD->fd_]->ezfd_->onEvent(this,ezD->fd_,ezD->event_,ezD->uuid_);
		delete ezD;
	}
	firedfd_.clear();
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
	conMgr_=nullptr;
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
