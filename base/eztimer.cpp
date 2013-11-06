#include "eztimer.h"
#include <cassert>

base::ezTimerTask::ezTimerTask():cancel_(false),
	firedtime_(-1),
	repeats_(0),
	duration_(0)
{}

bool base::ezTimerTask::willFire(int64_t now)
{
	if(firedtime_<=0)
	{
		cancel_=true;
		return false;
	}
	if(now>=firedtime_)
		return true;
	else
		return false;
}

bool base::ezTimerTask::afterFire(int64_t now)
{
	if(repeats_<=0||duration_<=0)
		return false;
	if(--repeats_<=0)
		return false;
	firedtime_=now+duration_;
	return true;
}

void base::ezTimerTask::config(int64_t now,int64_t duration,int64_t repeat/*=TIMER_FOREVER*/)
{
	firedtime_=now+duration;
	repeats_=repeat;
	duration_=duration;
}


void base::ezTimer::addTimeTask(ezTimerTask* task)
{
	assert(task);
	assert(task->firedtime_>=0);
	minHeap_.push(task);
}

void base::ezTimer::tick(int64_t now)
{
	while(!minHeap_.empty())
	{
		ezTimerTask* task=minHeap_.top();
		if(task->canceled())
		{
			minHeap_.pop();
			delete task;
			continue;
		}
		if(!task->willFire(now))
			break;
		else
		{
			task->run();
			minHeap_.pop();
			if(task->afterFire(now))
				minHeap_.push(task);
			else
				delete task;
		}
	}
}
