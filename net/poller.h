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
	virtual void addFd(int fd,int event)=0;
	virtual void delFd(int fd,int event)=0;
	virtual void modFd(int fd,int event)=0;
	virtual void poll()=0;
	~ezPoller(){}
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
	ezEventLoop* loop_;
};
}
#endif