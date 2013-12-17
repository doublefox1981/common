#include "../net/event.h"
#include "../net/poller.h"
#include "../net/connection.h"
#include "../net/netpack.h"
#include "../net/socket.h"
#include "../base/memorystream.h"
#include "../base/scopeguard.h"
#include "../base/eztime.h"
#include "../base/eztimer.h"
#include "../base/thread.h"
#include "../base/util.h"
#include "../base/logging.h"
#include "../base/notifyqueue.h"

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
	virtual void process(ezConnection* conn,ezMsg* pack)
	{
// 		base::ezBufferReader reader(pack->data_,pack->size_);
// 		int seq=0;
// 		reader.Read(seq);
// 		if(seq_==0)
// 			seq_=seq;
// 		else
// 			assert(seq==++seq_);
// 		LOG_INFO("recv %d seq=%d",pack->size_,seq);
	}
private:
	int seq_;
};

class TestServerPackHander:public net::ezNetPackHander
{
public:
	virtual void process(ezConnection* conn,ezMsg* pack)
	{
// 		base::ezBufferReader reader(pack->data_,pack->size_);
// 		int seq=0;
// 		reader.Read(seq);
// 		//LOG_INFO("recv %d seq=%d",pack->size_,seq);
// 
// 		int s=pack->size_;
// 		ezNetPack* msg=new ezNetPack(s);
// 		msg->size_=s;
// 		base::ezBufferWriter writer(msg->data_,msg->capacity_);
// 		writer.Write(seq);
// 		conn->sendNetPack(msg);
	}
};

class NetThread:public base::Threads
{
public:
	explicit NetThread(ezEventLoop* l):looper_(l){}
	virtual void Run()
	{
// 		while(true)
// 		{
// 			looper_->netEventLoop();
// 			base::ezSleep(1);
// 		}
	}
	ezEventLoop* looper_;
};

std::unordered_set<uint64_t> gConnSet;
class TestClientHander:public net::ezClientHander
{
public:
	explicit TestClientHander(uint16_t t=15000):ezClientHander(t){}
	virtual void onOpen(ezEventLoop* looper,int fd,uint64_t uuid,int tid)
	{
		ezClientHander::onOpen(looper,fd,uuid,tid);
		ezConnection* conn=looper->getConnectionMgr()->findConnection(uuid);
		if(conn)
			gConnSet.insert(uuid);
	}
	virtual void onClose(ezEventLoop* looper,int fd,uint64_t uuid,int tid)
	{
		auto iter=gConnSet.find(uuid);
		if(iter!=gConnSet.end())
			gConnSet.erase(iter);
		ezClientHander::onClose(looper,fd,uuid,tid);
	}
};

int main()
{
  base::ezLogger::instance()->Start();
  const char* cc="hello world";
  LOG_INFO("%08x ",cc);
  std::string format;
  base::StringPrintf(&format,"%08x",cc);
  for(size_t len=0;len<strlen(cc);++len)
  {
    format+=" ";
    base::StringAppendf(&format,"%02x",char(cc[len]));
  }
  LOG_INFO("%s",format.c_str());
  
  net::InitNetwork();

	ezConnectionMgr* mgr=new ezConnectionMgr;
	mgr->setDefaultHander(new TestServerPackHander());
	ezEventLoop* ev=new ezEventLoop;
	ev->init(new ezServerHander((numeric_limits<uint16_t>::max)()),mgr,2);
	ev->serveOnPort(10010);

  ezConnectionMgr* mgr1=new ezConnectionMgr;
  mgr1->setDefaultHander(new TestPackHander());
  ezEventLoop* ev1=new ezEventLoop;
  ev1->init(new TestClientHander((numeric_limits<uint16_t>::max)()),mgr1,4);
  for(int i=0;i<16;++i)
  {
    ev1->getConnectionMgr()->connectTo(ev1,"127.0.0.1",10010);
  }
// 	base::ezTimer timer;
// 	base::ezTimerTask* task=new ezReconnectTimerTask(ev->getConnectionMgr());
// 	task->config(base::ezNowTick(),10*1000,base::ezTimerTask::TIMER_FOREVER);
// 	timer.addTimeTask(task);

	base::ScopeGuard guard([&](){delete ev;});
	int seq=0;
	while(true)
	{
// 		int64_t t=base::ezNowTick();
// 		string s;
// 		base::ezFormatTime(time(NULL),s);
// 		ev->crossEventLoop();
// 		timer.tick(t);
//     for(int i=0;i<128;++i)
//     {    
//       net::ezCrossEventData data;
//       data.uuid_=ev->uuid();
//       queue_.send(data);
//     }
    ev->loop();
    ev1->loop();
    base::ezSleep(1);

// 		for(auto iter=gConnSet.begin();iter!=gConnSet.end();++iter)
// 		{
// 			ezConnection* conn=ev->getConnectionMgr()->findConnection(*iter);
// 			if(!conn)
// 				continue;
// 			for(int i=0;i<4;++i)
// 			{
// 				int ss=rand()%15000+4;
// 				ezNetPack* msg=new ezNetPack(ss);
// 				msg->size_=ss;
// 				base::ezBufferWriter writer(msg->data_,msg->capacity_);
// 				writer.Write(++seq);
// 				conn->sendNetPack(msg);
// 			}
// 		}
	}
	return 1;
}