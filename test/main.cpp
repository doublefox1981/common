#include "../net/event.h"
#include "../net/poller.h"
#include "../net/connection.h"
#include "../net/netpack.h"
#include "../base/memorystream.h"
#include "../base/scopeguard.h"
#include "../base/eztime.h"
#include "../base/eztimer.h"
#include "../base/thread.h"
#include <limits>
#include <algorithm>
#include <queue>
#include <unordered_set>

using namespace net;
using namespace std;

class TestPackHander:public net::ezNetPackHander
{
public:
	TestPackHander():seq_(0){}
	virtual void process(ezConnection* conn,ezNetPack* pack)
	{
		base::ezBufferReader reader(pack->data_,pack->size_);
		int seq=0;
		reader.Read(seq);
		if(seq_==0)
			seq_=seq;
		else
			assert(seq==++seq_);
		printf("recv %d seq=%d\n",pack->size_,seq);
	}
private:
	int seq_;
};

class TestServerPackHander:public net::ezNetPackHander
{
public:
	virtual void process(ezConnection* conn,ezNetPack* pack)
	{
		base::ezBufferReader reader(pack->data_,pack->size_);
		int seq=0;
		reader.Read(seq);
		//printf("recv %d seq=%d\n",pack->size_,seq);

		int s=pack->size_;
		ezNetPack* msg=new ezNetPack(s);
		msg->size_=s;
		base::ezBufferWriter writer(msg->data_,msg->capacity_);
		writer.Write(seq);
		conn->sendNetPack(msg);
	}
};

class NetThread:public base::Threads
{
public:
	explicit NetThread(ezEventLoop* l):looper_(l){}
	virtual void Run()
	{
		while(true)
		{
			looper_->netEventLoop();
			base::ezSleep(5);
		}
	}
	ezEventLoop* looper_;
};

std::unordered_set<uint64_t> gConnSet;
class TestClientHander:public ezClientHander
{
public:
	explicit TestClientHander(uint16_t t=15000):ezClientHander(t){}
	virtual void onOpen(ezEventLoop* looper,int fd,uint64_t uuid)
	{
		ezClientHander::onOpen(looper,fd,uuid);
		ezConnection* conn=looper->getConnectionMgr()->findConnection(uuid);
		if(conn)
			gConnSet.insert(uuid);
	}
	virtual void onClose(ezEventLoop* looper,int fd,uint64_t uuid)
	{
		auto iter=gConnSet.find(uuid);
		if(iter!=gConnSet.end())
			gConnSet.erase(iter);
		ezClientHander::onClose(looper,fd,uuid);
	}
};
struct S
{
	list_head lst;
	int i;
};
int main()
{
	InitNetwork();
#ifdef __linux__
	ezConnectionMgr* mgr=new ezConnectionMgr;
	mgr->setDefaultHander(new TestServerPackHander());
	ezEventLoop* ev=new ezEventLoop;
	ev->init(new ezEpollPoller(ev),new ezServerHander(/*(numeric_limits<uint16_t>::max)()*/15003),mgr);
	ev->serveOnPort(10010);
#else
	ezConnectionMgr* mgr=new ezConnectionMgr;
	mgr->setDefaultHander(new TestPackHander());
	ezEventLoop* ev=new ezEventLoop;
	ev->init(new ezSelectPoller(ev),new TestClientHander(/*(numeric_limits<uint16_t>::max)()*/15003),mgr);
	ev->getConnectionMgr()->connectTo(ev,"192.168.99.51",10010);
#endif
	NetThread* thread=new NetThread(ev);
	thread->Start();

	base::ezTimer timer;
	base::ezTimerTask* task=new ezReconnectTimerTask(ev->getConnectionMgr());
	task->config(base::ezNowTick(),10*1000,base::ezTimerTask::TIMER_FOREVER);
	timer.addTimeTask(task);

	base::ScopeGuard guard([&](){delete ev;});
	int seq=0;
	while(true)
	{
		int64_t t=base::ezNowTick();
		string s;
		base::ezFormatTime(time(NULL),s);
		ev->crossEventLoop();
		timer.tick(t);
		base::ezSleep(10);

		for(auto iter=gConnSet.begin();iter!=gConnSet.end();++iter)
		{
			ezConnection* conn=ev->getConnectionMgr()->findConnection(*iter);
			if(!conn)
				continue;
			for(int i=0;i<1;++i)
			{
				int ss=rand()%15000+4;
				ezNetPack* msg=new ezNetPack(ss);
				msg->size_=ss;
				base::ezBufferWriter writer(msg->data_,msg->capacity_);
				writer.Write(++seq);
				conn->sendNetPack(msg);
			}
		}
	}
	return 1;
}