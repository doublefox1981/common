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
  class Connection;
  class IConnnectionHander;
  class IDecoder;
  class IEncoder;
  class ezThreadEventHander;
  class Poller;
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
  class EventLoop
  {
  public:
    EventLoop();
    ~EventLoop();
    int Initialize(IConnnectionHander* hander,IDecoder* decoder,IEncoder* encoder,int tnum);
    int serve_on_port(int port);
    int ConnectTo(const std::string& ip,int port,int64_t userdata,int32_t reconnect);
    int Shutdown();
    IConnnectionHander* GetHander() {return hander_;}
    IDecoder* GetDecoder() {return decoder_;}
    IEncoder* GetEncoder() {return encoder_;}
    ezIoThread* ChooseThread();
    ezIoThread* GetThread(int idx);
    void OccerEvent(int tid,ezThreadEvent& ev);
    int  GetTid() {return 0;}
    void Loop();
    void AddConnection(Connection* con);
    void DelConnection(Connection* con);
    int  GetConnectionNum();
    int  GetBufferSize();
    void SetBufferSize(int s);
  private:
    IConnnectionHander*             hander_;
    IConnnectionHander*             closehander_;
    IDecoder*                       decoder_;
    IEncoder*                       encoder_;
    ezIoThread**                      threads_;
    int                               threadnum_;
    ThreadEvQueue**                   evqueues_;
    ThreadEvQueue*                    mainevqueue_;
    std::unordered_set<Connection*> conns_;
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
    ezThreadEventHander(EventLoop* loop,int tid);
    virtual ~ezThreadEventHander(){}
    EventLoop* GetLooper(){return looper_;}
    int GetTid(){return tid_;}
    void OccurEvent(ezThreadEvent& ev);
    virtual void ProcessEvent(ezThreadEvent& ev)=0;
  private:
    EventLoop* looper_;
    int tid_;
  };
}

#endif