#ifndef _BASE_MAILBOX_H
#define _BASE_MAILBOX_H
#include "../base/thread.h"
#include "signal.h"
#include "readerwriterqueue.h"

namespace base{
  template<typename T>
  class NotifyQueue
  {
  public:
    fd_t get_fd(){return notify_.getfd();}
    void send(const T& t)
    {
      mutex_.lock();
      pipe_.enqueue(t);
      notify_.send();
      mutex_.unlock();
    }
    bool recv(T& t)
    {
      notify_.recv();
      return pipe_.try_dequeue(t);
    }
  private:
    typedef moodycamel::ReaderWriterQueue<T> pipe_t;
    pipe_t pipe_;
    Signaler notify_;
    base::SpinLock mutex_;
  };
} 

#endif