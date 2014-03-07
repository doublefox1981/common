#ifndef _EZ_TIMER_H
#define _EZ_TIMER_H
#include <stdint.h>
#include <queue>
#include <limits>
#include <functional>

namespace base
{
  class ezTimerTask
  {
  public:
    static const int64_t TIMER_FOREVER=0x7fffffffffffffff;
    ezTimerTask();
    virtual ~ezTimerTask(){}
    void config(int64_t now,int64_t duration,int64_t repeat=0);
    bool willFire(int64_t now);
    bool afterFire(int64_t now);
    void cancel(){cancel_=true;}
    bool canceled(){return cancel_;}
    virtual void run(){}
  private:
    bool cancel_;
    int64_t firedtime_;
    int64_t repeats_;
    int64_t duration_;

    friend class ezTimer;
  };

  template <typename ArgType>
  class ezFunctorTimerTask:public ezTimerTask
  {
  public:
    typedef std::function<void(const ArgType&)> FUNC_TYPE;
    ezFunctorTimerTask(FUNC_TYPE& func,const ArgType& arg):functor_(func),arg_(arg){}
    virtual void run(){functor_(arg_);}
  private:
    ArgType arg_;
    FUNC_TYPE functor_;
  };

  class ezVoidFunctorTimerTask:public ezTimerTask
  {
  public:
    typedef std::function<void()> FUNC_TYPE;
    ezVoidFunctorTimerTask(FUNC_TYPE& func):functor_(func){}
    virtual void run(){functor_();}
  private:
    FUNC_TYPE functor_;
  };
  /**
  *** 常用定时器分为时间轮算法(用于linux内核)，以及小顶堆(优先队列)
  *** 此处使用优先队列 lg(n)的时间复杂度
  **/
  class ezTimer
  {
  public:
    void addTimeTask(ezTimerTask* task);
    void tick(int64_t now);
    void runAfter(int64_t now,int later,std::function<void()>& func);
    template <typename ArgType>
    void runAfter(int64_t now,int later,std::function<void(const ArgType&)>& func,const ArgType& args);
  private:
    std::priority_queue<ezTimerTask*,std::vector<ezTimerTask*>,std::greater<std::vector<ezTimerTask*>::value_type>> minHeap_;
  };

  template <typename ArgType>
  void ezTimer::runAfter(int64_t now,int later,std::function<void(const ArgType&)>& func,const ArgType& args)
  {
    ezTimerTask* task=new ezFunctorTimerTask<ArgType>(func,args);
    task->config(now,later,0);
    addTimeTask(task);
  }

  void ezTimer::runAfter(int64_t now,int later,std::function<void()>& func)
  {
    ezTimerTask* task=new ezVoidFunctorTimerTask(func);
    task->config(now,later,0);
    addTimeTask(task);
  }
}

#endif