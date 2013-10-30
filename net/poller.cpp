#include "poller.h"
#include "event.h"
#include "socket.h"

void net::ezListenerFd::onEvent(ezEventLoop* looper,int fd,int mask)
{
	if(mask&ezNetRead)
	{
		struct sockaddr_in si;
		SOCKET s=net::Accept(fd,&si);
		if(s==INVALID_SOCKET)
			return;
		looper->add(s,new ezClientFd,ezNetRead);
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

void net::ezClientFd::onEvent(ezEventLoop* looper,int fd,int event)
{
	if(event&ezNetRead)
	{
		char* pbuf=NULL;
		size_t s=inbuf_->getWritable(pbuf);
		int retval=Read(fd,pbuf,s);
		if((retval==0)||(retval<0&&errno!=EAGAIN))
		{
			ezCrossEventData* fireD=new ezCrossEventData;
			fireD->event_=ezCrossClose;
			fireD->fd_=fd;
			looper->pushFired(fireD);
		}
		else
		{
			inbuf_->addWritePos(retval);

			// hander->onmessage();
			// test ...
			char* pr=NULL;
			size_t s=inbuf_->getReadable(pr);
			pr[s]=0;
			printf("%s\n",pr);
			inbuf_->reset();
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
}
