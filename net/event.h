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

  class ezIHander;
  class ezIoThread;
  class ezFd;
  class ezConnectionMgr;

  class ezThreadEventHander;
  struct ezThreadEvent
  {
    enum ThreadEventType
    {
      NEW_FD,
      NEW_CONNECTION,
    }type_;
    ezThreadEventHander* hander_;
  };

  struct ezMsgWarper
  {
    uint64_t uuid_;
    ezMsg    msg_;
  };

  typedef base::ezNotifyQueue<ezThreadEvent> ThreadEvQueue;
  class ezEventLoop
  {
  public:
    ezEventLoop();
    ~ezEventLoop();
    int init(ezIHander* hander,ezConnectionMgr* mgr,int tnum);
    int serveOnPort(int port);
    int shutdown();
    void sendMsg(int tid,int fd,ezMsg& msg);
    ezConnectionMgr* getConnectionMgr() {return conMgr_;}
    ezIHander* getHander() {return hander_;}
    ezIoThread* ChooseThread();
    ezIoThread* GetThread(int idx);
    void OccerEvent(int tid,ezThreadEvent& ev);
    int gettid() {return 0;}
    void notify(ezIoThread* thread,ezCrossEventData& data);
    void o2nConnectTo(uint64_t uuid,const char* toip,int toport);
    void o2nCloseFd(int tid,int fd,uint64_t uuid);
    void loop();
  private:
    ezIHander*         hander_;
    ezConnectionMgr*   conMgr_;
    ezIoThread*        threads_;
    int                threadnum_;
    ThreadEvQueue**    evqueues_;
    ThreadEvQueue*     mainevqueue_;
  };

  class ezUUID:public base::ezSingleTon<ezUUID>
  {
  public:
    ezUUID(){suuid_.Set(1);}
    uint64_t uuid();
  private:
    base::AtomicNumber suuid_;
  };

  class ezThreadEventHander
  {
  public:
    ezThreadEventHander(ezEventLoop* loop,int tid);
    virtual ~ezThreadEventHander(){}
    ezEventLoop* GetLooper(){return looper_;}
    int GetTid(){return tid_;}
    void OccurEvent(ezThreadEvent& ev);
    virtual void ProcessEvent(ezThreadEvent& ev)=0;
  private:
    ezEventLoop* looper_;
    int tid_;
  };
}
#endif