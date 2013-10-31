#include "poller.h"
#include "event.h"
#include "socket.h"
#include "netpack.h"
#include "connection.h"

void net::ezListenerFd::onEvent(ezEventLoop* looper,int fd,int mask,uint64_t uuid)
{
	if(mask&ezNetRead)
	{
		struct sockaddr_in si;
		SOCKET s=net::Accept(fd,&si);
		if(s==INVALID_SOCKET)
			return;
		uint64_t uuid=looper->add(s,new ezClientFd,ezNetRead);
		looper->postNewFd(s,uuid);
	}
}

void net::ezSelectPoller::addFd(int fd,int mask)
{
	if(mask&ezNetRead)
		FD_SET(fd,&rfds_);
	if(mask&ezNetWrite)
		FD_SET(fd,&wfds_);
}

void net::ezSelectPoller::delFd(int fd,int mask)
{
	if(mask&ezNetRead)
		FD_CLR(fd,&rfds_);
	if(mask&ezNetWrite)
		FD_CLR(fd,&wfds_);
}

void net::ezSelectPoller::modFd(int fd,int mask)
{

}

void net::ezSelectPoller::poll()
{
	struct timeval tm = {0,0};
	memcpy(&urfds_,&rfds_,sizeof(fd_set));
	memcpy(&uwfds_,&wfds_,sizeof(fd_set));
	int retval=select(loop_->maxFd(),&urfds_,&uwfds_,NULL,&tm);
	if(retval>0)
	{
		for(int j=0;j<=loop_->maxFd();++j)
		{
			int mask=0;
			ezNetEventData* d=loop_->ezNetEventDatai(j);
			if(!d||d->event_==ezNetNone) continue;
			if(d->event_&ezNetRead&&FD_ISSET(d->fd_,&urfds_))
				mask|=ezNetRead;
			if(d->event_&ezNetWrite&&FD_ISSET(d->fd_,&uwfds_))
				mask|=ezNetWrite;
			ezNetEventData* fireD=new ezNetEventData;
			fireD->event_=mask;
			fireD->fd_=d->fd_;
			loop_->pushFired(fireD);
		}
	}
}

net::ezSelectPoller::ezSelectPoller( ezEventLoop* loop ) :loop_(loop)
{
	FD_ZERO(&rfds_);
	FD_ZERO(&wfds_);
	FD_ZERO(&urfds_);
	FD_ZERO(&uwfds_);
}

void net::ezClientFd::onEvent(ezEventLoop* looper,int fd,int event,uint64_t uuid)
{
	if(event&ezNetRead)
	{
		char* pbuf=NULL;
		size_t s=inbuf_->getWritable(pbuf);
		int retval=Read(fd,pbuf,s);
		if((retval==0)||(retval<0&&errno!=EAGAIN))
		{
			looper->postCloseFd(fd,uuid);
			looper->del(fd);
		}
		else
		{
			ezHander* hander=looper->getHander();
			inbuf_->addWritePos(retval);
			char* rbuf=NULL;
			size_t rs=inbuf_->getReadable(rbuf);
			int rets=hander->decode(looper,rbuf,rs);
			assert(rets<=inbuf_->readableSize());
			if(rets>0)
				inbuf_->addReadPos(rets);
			else if(rets<0)
			{
				looper->postError(fd,uuid);
				looper->del(fd);
			}
		}
	}
	if(event&ezNetWrite)
	{

	}
}

net::ezClientFd::ezClientFd()
{
	inbuf_=new ezBuffer();
	outbuf_=new ezBuffer();
}

net::ezClientFd::~ezClientFd()
{
	if(inbuf_)
		delete inbuf_;
	if(outbuf_)
		delete outbuf_;
	list_head *iter,*next;
	list_for_each_safe(iter,next,&sendqueue_)
	{
		net::ezSendBlock* blk=list_entry(iter,net::ezSendBlock,lst_);
		delete blk;
	}
}
