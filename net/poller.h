#ifndef _POLLER_H
#define _POLLER_H
#include "socket.h"
#include "buffer.h"

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
	virtual void onEvent(ezEventLoop* looper,int fd,int event)=0;
	virtual ~ezFd(){}
};

class ezHander
{
public:
	virtual void onOpen(ezPoller* poller,ezFd* fd)=0;
	virtual void onClose(ezPoller* poller,ezFd* fd)=0;
};

class ezListenerFd:public ezFd
{
public:
	virtual void onEvent(ezEventLoop* looper,int fd,int event);
};

class ezClientFd:public ezFd
{
public:
	ezClientFd();
	virtual ~ezClientFd();
	virtual void onEvent(ezEventLoop* looper,int fd,int event);
private:
	ezBuffer* inbuf_;
	ezBuffer* outbuf_;
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