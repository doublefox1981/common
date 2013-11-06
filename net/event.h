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
	ezNetErr=4,
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
class ezConnectionMgr;

// 网络io事件
struct ezFdData
{
	int fd_;
	uint64_t uuid_;
	int event_;
	ezFd* ezfd_;
	ezFdData();
	~ezFdData();
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
public:
	ezEventLoop();
	~ezEventLoop();
	int init(ezPoller* poller,ezHander* hander,ezConnectionMgr* mgr);
	int serveOnPort(int port);
	int shutdown();
	uint64_t add(int fd,uint64_t uuid,ezFd *ezfd,int event);
	int del(int fd);
	int mod(int fd,int event,bool set);
	int maxFd() {return maxfd_;}
	ezFdData* ezFdDatai(int i) {return events_[i];}
	void pushFired(ezFdData* ezD){fired_.push_back(ezD);}
	void netEventLoop();
	void crossEventLoop();

	// net->other
	void n2oCrossEvent(ezCrossEventData* ev);
	void n2oCloseFd(int fd,uint64_t uuid);
	void n2oNewFd(int fd,uint64_t uuid);
	void n2oError(int fd,uint64_t uuid);

	// other->net
	void o2nCloseFd(int fd,uint64_t uuid);
	void o2nConnectTo(uint64_t uuid,const char* toip,int toport);

	void sendMsg(int fd,ezNetPack* msg);

	ezConnectionMgr* getConnectionMgr() {return conMgr_;}
	ezHander* getHander() {return hander_;}
	ezPoller* getPoller() {return poller_;}
	uint64_t uuid();
private:
	void processEv();
	void processMsg();

	base::AtomicNumber suuid_;

	ezPoller* poller_;
	ezHander* hander_;
	ezConnectionMgr* conMgr_;

	std::vector<ezFdData*> events_;
	std::vector<ezFdData*> fired_;
	int maxfd_;
	
	// net->other
	base::Mutex mutexCrossEv_;
	list_head crossEv_;

	// other->net
	base::Mutex mutexO2NCrossEv_;
	list_head crossO2NEv_;

	base::Mutex mutexSendQueue_;
	list_head sendMsgQueue_;
};
}
#endif