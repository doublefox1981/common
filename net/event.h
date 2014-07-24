#ifndef _EVENT_H
#define _EVENT_H
#include <vector>
#include <string>
#include <unordered_set>
#include "../base/portable.h"
#include "../base/thread.h"
#include "../base/readerwriterqueue.h"
#include "../base/notifyqueue.h"
#include "../base/singleton.h"
#include "netpack.h"

namespace net
{
  class ezIoThread;
  class ezConnection;
  class ezIConnnectionHander;
  class ezIDecoder;
  class ezIEncoder;
  class ezThreadEventHander;
  class ezPoller;
  struct ezThreadEvent
  {
    enum ThreadEventType
    {
      NEW_SERVICE,
      NEW_CONNECTTO,
      NEW_FD,
      NEW_CONNECTION,
      CLOSE_PASSIVE, 
      CLOSE_ACTIVE,
      CLOSE_FD,
      CLOSE_CONNECTION,
      CLOSE_CONNECTTO,
      NEW_MESSAGE,
      ENABLE_POLLOUT,
      STOP_FLASHEDFD,
      STOP_THREAD,
    }type_;
    ezThreadEventHander* hander_;
  };

  typedef base::ezNotifyQueue<ezThreadEvent> ThreadEvQueue;
  class ezEventLoop
  {
  public:
    ezEventLoop();
    ~ezEventLoop();
    int Initialize(ezIConnnectionHander* hander,ezIDecoder* decoder,ezIEncoder* encoder,int tnum);
    int ServeOnPort(int port);
    int ConnectTo(const std::string& ip,int port,int64_t userdata,int32_t reconnect);
    int Shutdown();
    ezIConnnectionHander* GetHander() {return hander_;}
    ezIDecoder* GetDecoder() {return decoder_;}
    ezIEncoder* GetEncoder() {return encoder_;}
    ezIoThread* ChooseThread();
    ezIoThread* GetThread(int idx);
    void OccerEvent(int tid,ezThreadEvent& ev);
    int  GetTid() {return 0;}
    void Loop();
    void AddConnection(ezConnection* con);
    void DelConnection(ezConnection* con);
    int  GetConnectionNum();
    int  GetBufferSize();
    void SetBufferSize(int s);
  private:
    ezIConnnectionHander*             hander_;
    ezIConnnectionHander*             closehander_;
    ezIDecoder*                       decoder_;
    ezIEncoder*                       encoder_;
    ezIoThread**                      threads_;
    int                               threadnum_;
    ThreadEvQueue**                   evqueues_;
    ThreadEvQueue*                    mainevqueue_;
    std::unordered_set<ezConnection*> conns_;
    bool                              shutdown_;
    int                               buffersize_;
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