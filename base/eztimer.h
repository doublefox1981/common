#ifndef _EZ_TIMER_H
#define _EZ_TIMER_H
#include <stdint.h>
#include <queue>
#include <limits>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace base
{
  static const int64_t TIMER_FOREVER=0x7fffffffffffffff;
  class TimerTask
  {
  public:
    explicit TimerTask(int64_t tid);
    virtual ~TimerTask(){}
    void config(int64_t now,int64_t duration,int64_t repeat=TIMER_FOREVER);
    bool will_fire(int64_t now);
    bool after_fire(int64_t now);
    void cancel(){cancel_=true;}
    bool canceled(){return cancel_;}
    virtual void run(){}
  private:
    bool    cancel_;
    int64_t fired_time_;
    int64_t repeats_;
    int64_t duration_;
    int64_t id_;
    friend class Timer;
    friend struct cmp;
  };

  template <typename ArgType>
  class FunctorTimerTask:public TimerTask
  {
  public:
    typedef const std::function<void(const ArgType&)> FUNC_TYPE;
    FunctorTimerTask(int64_t tid,FUNC_TYPE& func,const ArgType& arg):TimerTask(tid),functor_(func),arg_(arg){}
    virtual void run(){functor_(arg_);}
  private:
    ArgType arg_;
    FUNC_TYPE functor_;
  };

  class VoidFunctorTimerTask:public TimerTask
  {
  public:
    typedef const std::function<void()> FUNC_TYPE;
    VoidFunctorTimerTask(int64_t tid,FUNC_TYPE& func):TimerTask(tid),functor_(func){}
    virtual void run(){functor_();}
  private:
    FUNC_TYPE functor_;
  };
  /**
  *** 常用定时器分为时间轮算法(用于linux内核)，以及小顶堆(优先队列)
  *** 此处使用优先队列 lg(n)的时间复杂度
  **/
  struct cmp
  {
    bool operator()(const TimerTask* a,const TimerTask* b)
    {
      return a->fired_time_>b->fired_time_;
    }
  };
  typedef std::priority_queue<TimerTask*,std::vector<TimerTask*>,cmp> TIMER_HEAP;
  typedef std::unordered_map<int64_t,TimerTask*> TIMER_MAP;
  class Timer
  {
  public:
    Timer();
    virtual ~Timer();
    void add_timer_task(TimerTask* task);
    void del_timer_task(uint64_t id);
    void tick(int64_t now);
    int64_t gen_timer_uuid();
    int64_t run_after(const std::function<void()>& func,int64_t now,int later,int64_t repeat=TIMER_FOREVER)
    {    
      TimerTask* task=new VoidFunctorTimerTask(gen_timer_uuid(),func);
      task->config(now,later,repeat);
      add_timer_task(task);
      return task->id_;
    }
    template <typename ArgType>
    int64_t run_after(const std::function<void(const ArgType&)>& func,const ArgType& args,int64_t now,int later,int64_t repeat=TIMER_FOREVER)
    {
      TimerTask* task=new FunctorTimerTask<ArgType>(gen_timer_uuid(),func,args);
      task->config(now,later,repeat);
      add_timer_task(task);
      return task->id_;
    }
  private:
    void remove_timer_task(int64_t id);
    TIMER_HEAP min_heap_;
    TIMER_MAP  timer_map_;
    int64_t    s_timer_id_;
  };
}

#endif