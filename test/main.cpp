#include "../net/event.h"
#include "../net/poller.h"
using namespace net;

static bool initNetwork(int major_version = 2) 
{
	WSADATA wsa_data;
	int minor_version = 0;
	if(major_version == 1) minor_version = 1;
	if(WSAStartup(MAKEWORD(major_version, minor_version), &wsa_data) != 0)
		return false;
	else
		return true;
}

int main()
{
	initNetwork();
	ezEventLoop ev;
	ev.init(new ezSelectPoller(&ev));
	ev.serveOnPort(10010);
	while(true)
	{
		ev.loop();
		Sleep(10);
	}
	return 1;
}