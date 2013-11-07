#include "poller.h"
#include "event.h"
#include "socket.h"
#include "netpack.h"
#include "connection.h"
#include "../base/memorystream.h"

void net::ezListenerFd::onEvent(ezEventLoop* looper,int fd,int mask,uint64_t uuid)
{
	if(mask&ezNetRead)
	{
		struct sockaddr_in si;
		SOCKET s=net::Accept(fd,&si);
		if(s==INVALID_SOCKET)
			return;
		uint64_t uuid=looper->add(s,looper->uuid(),new ezClientFd,ezNetRead);
		looper->n2oNewFd(s,uuid);
	}
}

void net::ezListenerFd::sendMsg( ezSendBlock* blk )
{
	assert(false);
}

size_t net::ezListenerFd::formatMsg()
{
	return 0;
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

void net::ezSelectPoller::modFd(int fd,int mask,int srcmask,bool set)
{
	if(!set)
		delFd(fd,mask);
	else
		addFd(fd,mask);
}

void net::ezSelectPoller::poll()
{
	struct timeval tm = {0,1000};
	memcpy(&urfds_,&rfds_,sizeof(fd_set));
	memcpy(&uwfds_,&wfds_,sizeof(fd_set));
	int retval=select(getEventLooper()->maxFd()+1,&urfds_,&uwfds_,nullptr,&tm);
	if(retval>0)
	{
		for(int j=0;j<=getEventLooper()->maxFd();++j)
		{
			int mask=0;
			ezFdData* d=getEventLooper()->ezFdDatai(j);
			if(!d||d->event_==ezNetNone) continue;
			if(d->event_&ezNetRead&&FD_ISSET(d->fd_,&urfds_))
				mask|=ezNetRead;
			if(d->event_&ezNetWrite&&FD_ISSET(d->fd_,&uwfds_))
				mask|=ezNetWrite;
			ezFdData* fireD=new ezFdData;
			fireD->event_=mask;
			fireD->fd_=d->fd_;
			fireD->uuid_=d->uuid_;
			getEventLooper()->pushFired(fireD);
		}
	}
}

net::ezSelectPoller::ezSelectPoller(ezEventLoop* loop):ezPoller(loop)
{
	FD_ZERO(&rfds_);
	FD_ZERO(&wfds_);
	FD_ZERO(&urfds_);
	FD_ZERO(&uwfds_);
}

// 暂时不支持epoll边缘触发的方式，因为有可能读事件未完全读数据
void net::ezClientFd::onEvent(ezEventLoop* looper,int fd,int event,uint64_t uuid)
{
	if(event&ezNetRead)
	{
		char* pbuf=nullptr;
		size_t s=inbuf_->getWritable(pbuf);
		if(s>0&&pbuf)
		{
			int retval=Read(fd,pbuf,s);
			if((retval==0)||(retval<0&&errno!=EAGAIN))
			{
				looper->n2oCloseFd(fd,uuid);
				looper->del(fd);
				return;
			}
			else
				inbuf_->addWritePos(retval);
		}
		ezHander* hander=looper->getHander();
		char* rbuf=nullptr;
		size_t rs=inbuf_->getReadable(rbuf);
		if(rs>0)
		{
			int rets=hander->decode(looper,fd,uuid,rbuf,rs);
			assert(rets<=(int)inbuf_->readableSize());
			if(rets>0)
				inbuf_->addReadPos(rets);
			else if(rets<0)
			{
				looper->n2oError(fd,uuid);
				looper->del(fd);
				return;
			}
		}
	}
	if(event&ezNetWrite)
	{
		char* pbuf=nullptr;
		size_t s=outbuf_->getReadable(pbuf);
		if(s>0&&pbuf)
		{
			int sendlen=0;
			while(sendlen<(int)s)
			{
				int retval=Write(fd,pbuf,s);
				if(retval<0)
				{
					if(errno==EWOULDBLOCK||errno==EAGAIN)
						break;
					else
					{
						looper->n2oError(fd,uuid);
						looper->del(fd);
						return;
					}
				}
				else
					sendlen+=retval;
			}
			outbuf_->addReadPos(sendlen);
		}
		if(outbuf_->readableSize()<=0&&list_empty(&sendqueue_))
		{
			looper->getPoller()->modFd(fd,ezNetWrite,ezNetWrite|ezNetRead,false);
			return;
		}
	}
	if(event&ezNetErr)
	{
		looper->n2oError(fd,uuid);
		looper->del(fd);
	}
}

net::ezClientFd::ezClientFd()
{
	inbuf_=new ezBuffer();
	outbuf_=new ezBuffer();
	INIT_LIST_HEAD(&sendqueue_);
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

void net::ezClientFd::sendMsg( ezSendBlock* blk )
{
	list_add_tail(&blk->lst_,&sendqueue_);
}

size_t net::ezClientFd::formatMsg()
{
	if(list_empty(&sendqueue_))
		return 0;
	char* pbuf=nullptr;
	size_t s=outbuf_->getWritable(pbuf);
	if(s<=0)
		return 0;
	base::ezBufferWriter writer(pbuf,s);
	LIST_HEAD(tmplist);
	list_splice_init(&sendqueue_,&tmplist);
	list_head *iter,*next;
	list_for_each_safe(iter,next,&tmplist)
	{
		ezSendBlock* blk=list_entry(iter,ezSendBlock,lst_);
		assert(blk->pack_);
		if(writer.CanIncreaseSize(sizeof(uint16_t)+blk->pack_->size_))
		{
			writer.Write(blk->pack_->size_);
			writer.WriteBuffer(blk->pack_->data_,blk->pack_->size_);
			delete blk;
		}
		else
			break;
	}
	outbuf_->addWritePos(writer.GetUsedSize());
	list_for_each_safe_from(iter,next,&tmplist)
		list_add_tail(iter,&sendqueue_);
	return outbuf_->readableSize();
}

#ifdef __linux__

net::ezEpollPoller::ezEpollPoller(ezEventLoop* loop):ezPoller(loop)
{
	epollFd_=epoll_create1(0);
}

net::ezEpollPoller::~ezEpollPoller()
{
	close(epollFd_);
}

void net::ezEpollPoller::addFd(int fd,int mask)
{
	struct epoll_event ee;
    ee.events|=EPOLLERR;
	ee.events|=EPOLLHUP;
    if(mask&ezNetRead) ee.events|=EPOLLIN;
    if(mask&ezNetWrite) ee.events|=EPOLLOUT;
    ee.data.u64=0;
    ee.data.fd=fd;
    if(epoll_ctl(epollFd_,EPOLL_CTL_ADD,fd,&ee)==-1) 
		printf("EPOLL_CTL_ADD fail\n");
}

void net::ezEpollPoller::delFd(int fd,int mask)
{
	struct epoll_event ee;
	ee.events=0;
	if(mask&ezNetRead) ee.events|=EPOLLIN;
	if(mask&ezNetWrite) ee.events|=EPOLLOUT;
	ee.data.u64=0;
	ee.data.fd=fd;
	if(epoll_ctl(epollFd_,EPOLL_CTL_DEL,fd,&ee)==-1) 
		printf("EPOLL_CTL_DEL fail\n");
}

void net::ezEpollPoller::modFd(int fd,int mask,int srcmask,bool set)
{
	int dstmask=mask|srcmask;
	if(!set)
		dstmask&=(~mask);
	struct epoll_event ee;
	ee.events=0;
	ee.events|=EPOLLERR;
	ee.events|=EPOLLHUP;
	if(dstmask&ezNetRead) ee.events|=EPOLLIN;
	if(dstmask&ezNetWrite) ee.events|=EPOLLOUT;
	ee.data.u64 = 0;
	ee.data.fd = fd;
	if(epoll_ctl(epollFd_,EPOLL_CTL_MOD,fd,&ee)==-1) 
		printf("EPOLL_CTL_MOD fail\n");
}

void net::ezEpollPoller::poll()
{
	int retval=0;
	retval = epoll_wait(epollFd_,epollEvents_,sizeof(epollEvents_)/sizeof(struct epoll_event),5);
	for (int j=0;j<retval;j++) 
	{
		struct epoll_event *e = &epollEvents_[j];
		ezFdData* fdData=getEventLooper()->ezFdDatai(e->data.fd);
		assert(fdData);

		int mask = 0;
		if (e->events&EPOLLIN)  mask|=ezNetRead;
		if (e->events&EPOLLOUT) mask|=ezNetWrite;
		if (e->events&EPOLLERR) mask|=ezNetErr;
		if (e->events&EPOLLHUP) mask| ezNetErr;

		ezFdData* fireD=new ezFdData;
		fireD->event_=mask;
		fireD->fd_=e->data.fd;
		fireD->uuid_=fdData->uuid_;
		getEventLooper()->pushFired(fireD);
	}
}

#endif