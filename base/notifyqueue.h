#ifndef _BASE_MAILBOX_H
#define _BASE_MAILBOX_H
#include "../base/thread.h"
#include "signal.h"
#include "readerwriterqueue.h"

namespace base{
  // notify_ 用于唤醒io线程,ezNotifyQueue单进单出
  template<typename T>
  class ezNotifyQueue
  {
  public:
    fd_t GetFd(){return notify_.getfd();}
    void Send(const T& t)
    {
      mutex_.Lock();
      pipe_.enqueue(t);
      notify_.send();
      mutex_.Unlock();
    }
    bool Recv(T& t)
    {
      notify_.recv();
      return pipe_.try_dequeue(t);
    }
  private:
    typedef moodycamel::ReaderWriterQueue<T> pipe_t;
    pipe_t pipe_;
    ezSignaler notify_;
    base::SpinLock mutex_;
  };
} 

#endif