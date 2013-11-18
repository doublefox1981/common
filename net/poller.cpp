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
#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#else
#include <io.h>
#endif
// 暂时不支持epoll边缘触发的方式，因为有可能读事件未完全读数据
void net::ezClientFd::onEvent(ezEventLoop* looper,int fd,int event,uint64_t uuid)
{
	if(event&ezNetRead)
	{
		int retval=inbuf_->readfd(fd);
		if((retval==0)||(retval<0&&errno!=EAGAIN&&errno!=EINTR))
		{
			looper->n2oCloseFd(fd,uuid);
			looper->del(fd);
			return;
		}
		ezHander* hander=looper->getHander();
		char* rbuf=nullptr;
		int rs=inbuf_->readable(rbuf);
		if(rs>0)
		{
			int rets=hander->decode(looper,fd,uuid,rbuf,rs);
			if(rets>0)
				inbuf_->drain(rets);
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
		int s=outbuf_->readable(pbuf);
		if(s>0)
		{
			int retval=outbuf_->writefd(fd);
			if(retval<0)
			{
				looper->n2oError(fd,uuid);
				looper->del(fd);
				return;
			}
		}
		else if(list_empty(&sendqueue_))
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
	outbuf_=new ezBuffer(16*1024);
	INIT_LIST_HEAD(&sendqueue_);
	fp_=fopen("data.bin","w");
	insize_=0;
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
		list_del(iter);
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
	list_head *iter,*next;
	list_for_each_safe(iter,next,&sendqueue_)
	{
		ezSendBlock* blk=list_entry(iter,ezSendBlock,lst_);
		assert(blk->pack_);
		int canadd=outbuf_->fastadd();
		if(sizeof(uint16_t)+blk->pack_->size_<=canadd)
		{
			list_del(iter);
			outbuf_->add(&(blk->pack_->size_),sizeof(blk->pack_->size_));
			outbuf_->add(blk->pack_->data_,blk->pack_->size_);
			// test
			base::ezBufferReader reader(blk->pack_->data_,blk->pack_->size_);
			int seq=0;
			reader.Read(seq);
			printf("send %d seq=%d\n",blk->pack_->size_,seq);
			delete blk;
		}
		else
			break;
	}
	return outbuf_->off();
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