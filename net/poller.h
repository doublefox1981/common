#ifndef _POLLER_H
#define _POLLER_H
#include "socket.h"
#include "buffer.h"
#include "netpack.h"
#include "../base/list.h"

namespace net
{
class ezEventLoop;
class ezPoller
{
public:
	ezPoller(ezEventLoop* loop):loop_(loop){}
	virtual ~ezPoller(){}
	virtual void addFd(int fd,int event)=0;
	virtual void delFd(int fd,int event)=0;
	virtual void modFd(int fd,int event)=0;
	virtual void poll()=0;
	ezEventLoop* getEventLooper(){return loop_;}
private:
	ezEventLoop* loop_;
};

class ezFd
{
public:
	virtual void onEvent(ezEventLoop* looper,int fd,int event,uint64_t uuid)=0;
	virtual void sendMsg(ezSendBlock* blk)=0;
	virtual size_t formatMsg()=0;
	virtual ~ezFd(){}
};

class ezListenerFd:public ezFd
{
public:
	virtual void onEvent(ezEventLoop* looper,int fd,int event,uint64_t uuid);
	virtual void sendMsg(ezSendBlock* blk);
	virtual size_t formatMsg();
};

class ezClientFd:public ezFd
{
public:
	ezClientFd();
	virtual ~ezClientFd();
	virtual void onEvent(ezEventLoop* looper,int fd,int event,uint64_t uuid);
	virtual void sendMsg(ezSendBlock* blk);
	virtual size_t formatMsg();
private:
	ezBuffer* inbuf_;
	ezBuffer* outbuf_;
	list_head sendqueue_;
};

class ezSelectPoller:public ezPoller
{
public:
	ezSelectPoller(ezEventLoop* loop);
	virtual void addFd(int fd,int mask);
	virtual void delFd(int fd,int mask);
	virtual void modFd(int fd,int mask);
	virtual void poll();
private:
	fd_set wfds_;
	fd_set rfds_;
	fd_set uwfds_;
	fd_set urfds_;
};

#ifdef __linux__
#include <sys/epoll.h>
#include <unistd.h>
class ezEpollPoller:public ezPoller
{
public:
	ezEpollPoller(ezEventLoop* loop);
	virtual ~ezEpollPoller();
	virtual void addFd(int fd,int mask);
	virtual void delFd(int fd,int mask);
	virtual void modFd(int fd,int mask);
	virtual void poll();
private:
	int epollFd_;
	struct epoll_event epollEvents_[1024];
};
#endif
}
#endif