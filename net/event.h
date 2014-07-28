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
  class IoThread;
  class Connection;
  class IConnnectionHander;
  class IDecoder;
  class IEncoder;
  class ThreadEventHander;
  class Poller;
  struct ThreadEvent
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
    ThreadEventHander* hander_;
  };

  typedef base::NotifyQueue<ThreadEvent> ThreadEvQueue;
  class EventLoop
  {
  public:
    EventLoop();
    ~EventLoop();
    int initialize(IConnnectionHander* hander,IDecoder* decoder,IEncoder* encoder,int tnum);
    int serve_on_port(int port);
    int connect_to(const std::string& ip,int port,int64_t userdata,int32_t reconnect);
    int shutdown();
    IConnnectionHander* get_hander() {return hander_;}
    IDecoder* get_decoder() {return decoder_;}
    IEncoder* get_encoder() {return encoder_;}
    IoThread* choose_thread();
    IoThread* get_thread(int idx);
    void occer_event(int tid,ThreadEvent& ev);
    int  get_tid() {return 0;}
    void loop();
    void add_connection(Connection* con);
    void del_connection(Connection* con);
    int  get_connection_num();
    int  get_buffer_size();
    void set_buffer_size(int s);
  private:
    IConnnectionHander*             hander_;
    IConnnectionHander*             closehander_;
    IDecoder*                       decoder_;
    IEncoder*                       encoder_;
    IoThread**                      threads_;
    int                               threadnum_;
    ThreadEvQueue**                   evqueues_;
    ThreadEvQueue*                    mainevqueue_;
    std::unordered_set<Connection*> conns_;
    bool                              shutdown_;
    int                               buffersize_;
  };

  class UUID:public base::SingleTon<UUID>
  {
  public:
    UUID(){suuid_.Set(1);}
    uint64_t uuid();
  private:
    base::AtomicNumber suuid_;
  };

  class ThreadEventHander
  {
  public:
    ThreadEventHander(EventLoop* loop,int tid);
    virtual ~ThreadEventHander(){}
    EventLoop* get_looper(){return looper_;}
    int get_tid(){return tid_;}
    void occur_event(ThreadEvent& ev);
    virtual void process_event(ThreadEvent& ev)=0;
  private:
    EventLoop* looper_;
    int tid_;
  };
}

#endif