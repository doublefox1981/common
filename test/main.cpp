#include "../net/event.h"
#include "../net/poller.h"
#include "../net/connection.h"
#include "../net/netpack.h"
#include "../base/memorystream.h"
#include "../base/scopeguard.h"

#include <limits>
using namespace net;
using namespace std;

int main()
{
	InitNetwork();

	ezEventLoop* ev1=new ezEventLoop;
	ev1->init(new ezSelectPoller(ev1),new ezServerHander((numeric_limits<uint16_t>::max)()));
	ev1->serveOnPort(10010);
	Sleep(1000);

	ezEventLoop* ev=new ezEventLoop;
	ev->init(new ezSelectPoller(ev),new ezClientHander((numeric_limits<uint16_t>::max)()));
	ev->getConnectionMgr()->connectTo(ev,"127.0.0.1",10010);
	Sleep(1000);

	base::ScopeGuard guard([&](){delete ev1; delete ev;});

	while(true)
	{
		ev1->netEventLoop();
		ev->netEventLoop();
		ev1->crossEventLoop();
		ev->crossEventLoop();
		Sleep(10);
	}
	delete ev;
	return 1;
}