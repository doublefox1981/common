#ifndef _EZ_TIMER_H
#define _EZ_TIMER_H
#include <stdint.h>
#include <queue>
#include <limits>

namespace base
{
class ezTimerTask
{
public:
	static const int64_t TIMER_FOREVER=0x7fffffffffffffff;
	ezTimerTask();
	~ezTimerTask(){}
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

/**
*** 常用定时器分为时间轮算法(用于linux内核)，以及小顶堆(优先队列)
*** 此处使用优先队列 lg(n)的时间复杂度
**/
class ezTimer
{
public:
	void addTimeTask(ezTimerTask* task);
	void tick(int64_t now);
private:
	std::priority_queue<ezTimerTask*,std::vector<ezTimerTask*>,std::greater<std::vector<ezTimerTask*>::value_type>> minHeap_;
};
}

#endif