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
	virtual void onOpen(ezConnection* conn)
	{
// 		ezClientHander::onOpen(io,fd,uuid,bindtid);
//     ezConnection* conn=io->getlooper()->getConnectionMgr()->findConnection(uuid);
//     if(conn)
//       gConnSet.insert(uuid);
	}
	virtual void onClose(ezConnection* conn)
	{
// 		auto iter=gConnSet.find(uuid);
// 		if(iter!=gConnSet.end())
// 			gConnSet.erase(iter);
// 		ezClientHander::onClose(io,fd,uuid);
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

	ezEventLoop* ev=new ezEventLoop;
  ev->Initialize(new ezServerHander,new ezMsgDecoder(10000),1);
	ev->ServeOnPort(10010);
  base::ezSleep(10000);

  ezEventLoop* ev1=new ezEventLoop;
  ev1->Initialize(new ezClientHander,new ezMsgDecoder(10000),1);
  for(int i=0;i<1;++i)
  {
    ev1->ConnectTo("127.0.0.1",10010,i);
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
    ev->Loop();
    ev1->Loop();
    base::ezSleep(1);

//     for(auto iter=gConnSet.begin();iter!=gConnSet.end();++iter)
//     {
//       ezConnection* conn=ev1->getConnectionMgr()->findConnection(*iter);
//       if(!conn)
//         continue;
//       for(int i=0;i<4;++i)
//       {
//         int ss=4096/*(rand()%9000)+4*/;
//         ezMsg msg;
//         net::ezMsgInitSize(&msg,ss);
//         base::ezBufferWriter writer((char*)net::ezMsgData(&msg),net::ezMsgSize(&msg));
//         writer.Write(++seq);
//         conn->sendNetPack(&msg);
//       }
//     }
	}
	return 1;
}