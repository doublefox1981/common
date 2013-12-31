#include "../base/portable.h"
#include "../base/memorystream.h"
#include "../base/scopeguard.h"
#include "../base/eztime.h"
#include "../base/eztimer.h"
#include "../base/thread.h"
#include "../base/util.h"
#include "../base/logging.h"

#include "../net/net_interface.h"
#include "../net/netpack.h"

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
class TestClientHander:public net::ezIConnnectionHander
{
public:
  virtual void OnOpen(ezConnection* conn)
  {
    for(size_t i=0;i<gConnSet.size();++i)
    {
      if(ConnectionUserdata(conn)==gConnSet[i].id_)
      {
        gConnSet[i].status_=ECTS_CONNECTOK;
        gConnSet[i].conn_=conn;
      }
    }
  }
  virtual void OnClose(ezConnection* conn)
  {
    for(size_t i=0;i<gConnSet.size();++i)
    {
      if(ConnectionUserdata(conn)==gConnSet[i].id_)
      {
        gConnSet[i].status_=ECTS_CONNECTOK;
        gConnSet[i].conn_=NULL;
      }
    }
  }
  virtual void OnData(ezConnection* conn,ezMsg* msg){}
};

#include "../base/readerwriterqueue.h"
int main()
{
  moodycamel::ReaderWriterQueue<int> s;
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

  net::EzNetInitialize();

  net::ezIConnnectionHander* hander=new TestClientHander;
  net::ezIDecoder* decoder=new net::ezMsgDecoder(20000);
  net::ezIEncoder* encoder=new net::ezMsgEncoder;
  ezEventLoop* ev=net::CreateEventLoop(hander,decoder,encoder,4);
  for(int i=0;i<10;++i)
  {
    net::Connect(ev,"192.168.99.51",10011,i,10);
    ConnectToInfo info={i,"192.168..99.51",10011,ECTS_CONNECTING,nullptr};
    gConnSet.push_back(info);
  }

  base::ScopeGuard guard([&](){net::DestroyEventLoop(ev); delete hander; delete decoder; delete encoder;});
  int seq=0;
  while(true)
  {
    net::EventProcess(ev);
    base::ezSleep(1);
    if((rand()%100)>95)
    {
      net::Connect(ev,"192.168.99.51",10011,0,10);
      continue;
    }
    for(size_t s=0;s<gConnSet.size();++s)
    {
      ezConnection* conn=gConnSet[s].conn_;
      if(!conn)
        continue;
      if((rand()%100)>90)
      {
        net::CloseConnection(conn);
        continue;
      }
      for(int i=0;i<1;++i)
      {
        int ss=(rand()%15000+4);
        ezMsg msg;
        net::ezMsgInitSize(&msg,ss);
        base::ezBufferWriter writer((char*)net::ezMsgData(&msg),net::ezMsgSize(&msg));
        writer.Write(++seq);
        net::MsgSend(conn,&msg);
      }
    }
  }
  return 1;
}