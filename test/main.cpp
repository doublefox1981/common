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

std::unordered_set<ezConnection*> gConnSet;
class TestClientHander:public net::ezClientHander
{
public:
	virtual void OnOpen(ezConnection* conn)
	{
    ezClientHander::OnOpen(conn);
    gConnSet.insert(conn);
	}
  virtual void OnClose(ezConnection* conn)
  {
    ezClientHander::OnClose(conn);
    auto iter=gConnSet.find(conn);
    if(iter!=gConnSet.end())
      gConnSet.erase(iter);
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
#ifdef __linux__
  ezEventLoop* ev=new ezEventLoop;
  ev->Initialize(new ezServerHander,new ezMsgDecoder(20000),new ezMsgEncoder,4);
  ev->ServeOnPort(10011);
#else
  ezEventLoop* ev1=new ezEventLoop;
  ev1->Initialize(new TestClientHander,new ezMsgDecoder(20000),new ezMsgEncoder,10);
  for(int i=0;i<1;++i)
  {
    ev1->ConnectTo("192.168.99.51",10011,i,0);
  }
#endif

// 	base::ezTimer timer;
// 	base::ezTimerTask* task=new ezReconnectTimerTask(ev->getConnectionMgr());
// 	task->config(base::ezNowTick(),10*1000,base::ezTimerTask::TIMER_FOREVER);
// 	timer.addTimeTask(task);

	//base::ScopeGuard guard([&](){delete ev;});
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
#ifdef __linux__
    ev->Loop();
#else
    ev1->Loop();
//     if((rand()%100)<10)
//       ev1->ConnectTo("192.168.99.51",10011,0,0);
#endif
    base::ezSleep(1);
    for(auto iter=gConnSet.begin();iter!=gConnSet.end();++iter)
    {
      ezConnection* conn=*iter;
      if(!conn)
        continue;
      if((rand()%100)>90)
      {
        conn->ActiveClose();
        continue;
      }
      for(int i=0;i<1;++i)
      {
        int ss=(rand()%15000+4);
        ezMsg msg;
        net::ezMsgInitSize(&msg,ss);
        base::ezBufferWriter writer((char*)net::ezMsgData(&msg),net::ezMsgSize(&msg));
        writer.Write(++seq);
        conn->SendMsg(msg);
      }
    }
	}
	return 1;
}