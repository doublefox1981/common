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
#include <vector>
#include <string>
using namespace net;
using namespace std;

enum EConnectToStatus
{
  ECTS_CONNECTING,
  ECTS_CONNECTOK,
  ECTS_DISCONNECT,
};
struct ConnectToInfo
{
  int64_t id_;
  std::string ip_;
  int port_;
  int status_;
  ezConnection* conn_;
};

std::vector<ConnectToInfo> gConnSet;
class TestClientHander:public net::ezClientHander
{
public:
  virtual void OnOpen(ezConnection* conn)
  {
    ezClientHander::OnOpen(conn);
    for(size_t i=0;i<gConnSet.size();++i)
    {
      if(conn->GetUserdata()==gConnSet[i].id_)
      {
        gConnSet[i].status_=ECTS_CONNECTOK;
        gConnSet[i].conn_=conn;
      }
    }
  }
  virtual void OnClose(ezConnection* conn)
  {
    ezClientHander::OnClose(conn);
    for(size_t i=0;i<gConnSet.size();++i)
    {
      if(conn->GetUserdata()==gConnSet[i].id_)
      {
        gConnSet[i].status_=ECTS_CONNECTOK;
        gConnSet[i].conn_=NULL;
      }
    }
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
  for(int i=0;i<10;++i)
  {
    ev1->ConnectTo("192.168.99.51",10011,i,10);
    ConnectToInfo info={i,"192.168..99.51",10011,ECTS_CONNECTING,nullptr};
    gConnSet.push_back(info);
  }
#endif

  // 	base::ezTimer timer;
  // 	base::ezTimerTask* task=new ezReconnectTimerTask(ev->getConnectionMgr());
  // 	task->config(base::ezNowTick(),10*1000,base::ezTimerTask::TIMER_FOREVER);
  // 	timer.addTimeTask(task);
#ifdef __linux__
  base::ScopeGuard guard([&](){delete ev;});
#else
  base::ScopeGuard guard([&](){delete ev1;});
#endif
  int seq=0;
  while(true)
  {
#ifdef __linux__
    ev->Loop();
#else
    ev1->Loop();
    if((rand()%100)<10)
      ev1->ConnectTo("192.168.99.51",10011,0,0);
#endif
    base::ezSleep(1);
    for(size_t s=0;s<gConnSet.size();++s)
    {
      ezConnection* conn=gConnSet[s].conn_;
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