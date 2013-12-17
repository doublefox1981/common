#ifndef _EVENT_H
#define _EVENT_H
#include "netpack.h"
#include "../base/portable.h"
#include "../base/thread.h"
#include "../base/list.h"
#include "../base/readerwriterqueue.h"
#include "../base/notifyqueue.h"
#include "../base/singleton.h"
#include <vector>
#include <unordered_set>

namespace net
{
  enum ezNetEventType
  {
    ezNetNone=0,
    ezNetRead=1,
    ezNetWrite=2,
    ezNetErr=4,
    ezNetAll=ezNetRead|ezNetWrite,
  };

  enum ezCrossEventType
  {
    ezCrossNone=0,
    ezCrossOpen=1,
    ezCrossClose=2,
    ezCrossError=3,
    ezCrossData=4,
    ezCrossPollout=5,
  };

  class ezHander;
  class ezIoThread;
  class ezFd;
  class ezConnectionMgr;

  // TODO: Îö¹¹
  struct ezCrossEventData
  {
    int      fromtid_;
    int      fd_;
    uint64_t uuid_;
    int      event_;
    ezMsg*   msg_;
    ezCrossEventData();
  };
  void ezCloseCrossEventData(ezCrossEventData& ev);

  class ezEventLoop
  {
  public:
    ezEventLoop();
    ~ezEventLoop();
    int init(ezHander* hander,ezConnectionMgr* mgr,int tnum);
    int serveOnPort(int port);
    int shutdown();
    void sendMsg(int tid,int fd,ezMsg& msg);
    ezConnectionMgr* getConnectionMgr() {return conMgr_;}
    ezHander* getHander() {return hander_;}
    ezIoThread* chooseThread();
    void notify(ezIoThread* thread,ezCrossEventData& data);
    void o2nConnectTo(uint64_t uuid,const char* toip,int toport);
    void o2nCloseFd(int tid,int fd,uint64_t uuid);
    void loop();
  private:
    ezHander*          hander_;
    ezConnectionMgr*   conMgr_;
    ezIoThread*        threads_;
    int                threadnum_;
  };

  class ezUUID:public base::ezSingleTon<ezUUID>
  {
  public:
    ezUUID(){suuid_.Set(1);}
    uint64_t uuid();
  private:
    base::AtomicNumber suuid_;
  };
}
#endif