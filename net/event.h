#ifndef _EVENT_H
#define _EVENT_H
#include "portable.h"
#include "netpack.h"
#include "../base/thread.h"
#include "../base/list.h"
#include <vector>

namespace net
{
enum ezNetEventType
{
	ezNetNone=0,
	ezNetRead=1,
	ezNetWrite=2,
	ezNetAll=ezNetRead|ezNetWrite,
};

enum ezCrossEventType
{
	ezCrossNone=0,
	ezCrossOpen=1,
	ezCrossClose=2,
	ezCrossError=3,
	ezCrossData=4,
};

class ezHander;
class ezPoller;
class ezFd;

// 网络io事件
struct ezNetEventData
{
	int fd_;
	uint64_t uuid_;
	int event_;
	ezFd* ezfd_;
	ezNetEventData();
	~ezNetEventData();
};

// 跨线程事件通知
struct ezCrossEventData
{
	struct list_head evlst_;
	int fd_;
	uint64_t uuid_;
	int event_;
	ezNetPack* msg_;
	ezCrossEventData();
	~ezCrossEventData();
};

class ezEventLoop
{
	static uint64_t suuid_;
public:
	ezEventLoop();
	~ezEventLoop();
	int init(ezPoller* poller,ezHander* hander);
	int serveOnPort(int port);
	int shutdown();
	uint64_t add(int fd, ezFd *ezfd,int event);
	int del(int fd);
	int modr(int fd, bool set);
	int modw(int fd, bool set);
	int maxFd() {return maxfd_;}
	ezNetEventData* ezNetEventDatai(int i) {return events_[i];}
	void pushFired(ezNetEventData* ezD){fired_.push_back(ezD);}
	void netEventLoop();
	void crossEventLoop();
	void postCrossEvent(ezCrossEventData* ev);
	void postCloseFd(int fd,uint64_t uuid);
	void postNewFd(int fd,uint64_t uuid);
	void postError(int fd,uint64_t uuid);
	void postActiveCloseFd(int fd,uint64_t uuid);
	ezHander* getHander() {return hander_;}
private:
	ezPoller* poller_;
	ezHander* hander_;

	std::vector<ezNetEventData*> events_;
	std::vector<ezNetEventData*> fired_;
	int maxfd_;
	
	// net->other
	base::Mutex mutexCrossEv_;
	list_head crossEv_;

	// other->net
	base::Mutex mutexO2NCrossEv_;
	list_head crossO2NEv_;
};
}
#endif