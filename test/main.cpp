#include "../base/portable.h"
#include "../base/memorystream.h"
#include "../base/scopeguard.h"
#include "../base/eztime.h"
#include "../base/eztimer.h"
#include "../base/thread.h"
#include "../base/util.h"
#include "../base/logging.h"
#include "../base/notifyqueue.h"

#include "../net/netpack.h"
#include "../net/event.h"
#include "../net/poller.h"
#include "../net/iothread.h"
#include "../net/connection.h"
#include "../net/socket.h"


#include <limits>
#include <algorithm>
#include <queue>
#include <unordered_set>

using namespace net;
using namespace std;

class TestPackHander:public net::ezINetPackHander
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

class TestServerPackHander:public net::ezINetPackHander
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
	explicit TestClientHander(ezIDecoder* de):ezClientHander(de){}
	virtual void onOpen(ezIoThread* io,int fd,uint64_t uuid,int bindtid)
	{
		ezClientHander::onOpen(io,fd,uuid,bindtid);
		ezConnection* conn=io->getlooper()->getConnectionMgr()->findConnection(uuid);
		if(conn)
			gConnSet.insert(uuid);
	}
	virtual void onClose(ezIoThread* io,int fd,uint64_t uuid)
	{
		auto iter=gConnSet.find(uuid);
		if(iter!=gConnSet.end())
			gConnSet.erase(iter);
		ezClientHander::onClose(io,fd,uuid);
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
	ev->init(new ezServerHander(new net::ezMsgDecoder(10000)),mgr,8);
	ev->serveOnPort(10010);

  ezConnectionMgr* mgr1=new ezConnectionMgr;
  mgr1->setDefaultHander(new TestPackHander());
  ezEventLoop* ev1=new ezEventLoop;
  ev1->init(new TestClientHander(new net::ezMsgDecoder(10000)),mgr1,4);
  for(int i=0;i<1;++i)
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

    for(auto iter=gConnSet.begin();iter!=gConnSet.end();++iter)
    {
      ezConnection* conn=ev1->getConnectionMgr()->findConnection(*iter);
      if(!conn)
        continue;
      for(int i=0;i<4;++i)
      {
        int ss=4096/*(rand()%9000)+4*/;
        ezMsg msg;
        net::ezMsgInitSize(&msg,ss);
        base::ezBufferWriter writer((char*)net::ezMsgData(&msg),net::ezMsgSize(&msg));
        writer.Write(++seq);
        conn->sendNetPack(&msg);
      }
    }
	}
	return 1;
}