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
using namespace net;
using namespace std;

int main()
{
	InitNetwork();
	ezEventLoop* ev1=new ezEventLoop;
	ev1->init(new ezSelectPoller(ev1),new ezServerHander((numeric_limits<uint16_t>::max)()));
	ev1->serveOnPort(10010);
	
	ezEventLoop* ev=new ezEventLoop;
	ev->init(new ezSelectPoller(ev),new ezClientHander((numeric_limits<uint16_t>::max)()));
	ev->getConnectionMgr()->connectTo(ev,"192.168.99.51",10010);

	base::ezTimer timer;
	base::ezTimerTask* task=new ezReconnectTimerTask(ev->getConnectionMgr());
	task->config(base::ezNowTick(),10*1000,base::ezTimerTask::TIMER_FOREVER);
	timer.addTimeTask(task);

	base::ScopeGuard guard([&](){delete ev1; delete ev;});

	while(true)
	{
		int64_t t=base::ezNowTick();
		string s;
		base::ezFormatTime(time(NULL),s);
		//printf("%s %I64d\n",s.c_str(),t);
		ev1->netEventLoop();
		ev->netEventLoop();
		ev1->crossEventLoop();
		ev->crossEventLoop();
		timer.tick(t);
		base::ezSleep(10);
	}
	delete ev;
	return 1;
}