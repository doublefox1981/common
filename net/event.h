#ifndef _EVENT_H
#define _EVENT_H
#include "portable.h"
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
	ezCrossData=4,
};

class ezPoller;
class ezFd;

// 网络io事件
struct ezNetEventData
{
	int fd_;
	int event_;
	ezFd* ezfd_;
	ezNetEventData();
	~ezNetEventData();
};

// 跨线程事件通知
struct ezCrossEventData
{
	int fd_;
	int event_;
};

class ezEventLoop
{
public:
	int init(ezPoller* poller);
	int serveOnPort(int port);
	int shutdown();
	int add(int fd, ezFd *ezfd,int event);
	int del(int fd);
	int modr(int fd, bool set);
	int modw(int fd, bool set);
	int maxFd() {return maxfd_;}
	ezNetEventData* ezNetEventDatai(int i) {return events_[i];}
	void pushFired(ezNetEventData* ezD){fired_.push_back(ezD);}
	void loop();
private:
	std::vector<ezNetEventData*> events_;
	std::vector<ezNetEventData*> fired_;
	int maxfd_;
	ezPoller* poller_;

};
}
#endif