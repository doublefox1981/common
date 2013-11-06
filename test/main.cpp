#include "../net/event.h"
#include "../net/poller.h"
#include "../net/connection.h"
#include "../net/netpack.h"
#include "../base/memorystream.h"
#include "../base/scopeguard.h"
#include "../base/eztime.h"
#include "../base/eztimer.h"
#include <limits>
#include <algorithm>
#include <queue>
#include <array>
using namespace net;
using namespace std;

class TestPackHander:public net::ezNetPackHander
{
public:
	virtual void process(ezConnection* conn,ezNetPack* pack)
	{
		for(int i=0;i<64;++i)
		{
			int s=rand()%15000+1;
			ezNetPack* msg=new ezNetPack(s);
			msg->size_=s;
			conn->sendNetPack(msg);
		}
	}
};

class TestClientHander:public ezClientHander
{
public:
	explicit TestClientHander(uint16_t t=15000):ezClientHander(t){}
	virtual void onOpen(ezEventLoop* looper,int fd,uint64_t uuid)
	{
		ezClientHander::onOpen(looper,fd,uuid);

		ezConnection* conn=looper->getConnectionMgr()->findConnection(uuid);
		if(conn)
		{
			int s=rand()%15000+1;
			ezNetPack* msg=new ezNetPack(s);
			msg->size_=s;
			conn->sendNetPack(msg);
		}
	}
};

int main()
{
	InitNetwork();
#ifdef __linux__
	ezConnectionMgr* mgr=new ezConnectionMgr;
	mgr->setDefaultHander(new TestPackHander());
	ezEventLoop* ev=new ezEventLoop;
	ev->init(new ezEpollPoller(ev),new ezServerHander((numeric_limits<uint16_t>::max)()),mgr);
	ev->serveOnPort(10010);
#else
	ezConnectionMgr* mgr=new ezConnectionMgr;
	mgr->setDefaultHander(new TestPackHander());
	ezEventLoop* ev=new ezEventLoop;
	ev->init(new ezSelectPoller(ev),new TestClientHander((numeric_limits<uint16_t>::max)()),mgr);
	ev->getConnectionMgr()->connectTo(ev,"192.168.99.51",10010);
#endif
	base::ezTimer timer;
	base::ezTimerTask* task=new ezReconnectTimerTask(ev->getConnectionMgr());
	task->config(base::ezNowTick(),10*1000,base::ezTimerTask::TIMER_FOREVER);
	timer.addTimeTask(task);

	base::ScopeGuard guard([&](){delete ev;});

	while(true)
	{
		int64_t t=base::ezNowTick();
		string s;
		base::ezFormatTime(time(NULL),s);
		ev->netEventLoop();
		ev->crossEventLoop();
		timer.tick(t);
		base::ezSleep(10);
	}
	delete ev;
	return 1;
}