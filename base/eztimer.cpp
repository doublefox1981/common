#include "eztimer.h"
#include <cassert>

base::TimerTask::TimerTask(int64_t tid):cancel_(false),
	fired_time_(-1),
	repeats_(0),
	duration_(0),
  id_(tid)
{}

bool base::TimerTask::will_fire(int64_t now)
{
	if(fired_time_<=0)
	{
		cancel_=true;
		return false;
	}
	if(now>=fired_time_)
		return true;
	else
		return false;
}

bool base::TimerTask::after_fire(int64_t now)
{
	if(repeats_<=0||duration_<=0)
		return false;
	if(--repeats_<=0)
		return false;
	fired_time_=now+duration_;
	return true;
}

void base::TimerTask::config(int64_t now,int64_t duration,int64_t repeat/*=TIMER_FOREVER*/)
{
	fired_time_=now+duration;
	repeats_=repeat;
	duration_=duration;
}

void base::Timer::add_timer_task(TimerTask* task)
{
	assert(task);
	assert(task->fired_time_>=0);
  assert(task->id_>0);
	min_heap_.push(task);
}

void base::Timer::del_timer_task(uint64_t id)
{
  auto iter=timer_map_.find(id);
  if(iter!=timer_map_.end())
  {
    TimerTask* task=iter->second;
    task->cancel();
  }
}

void base::Timer::remove_timer_task(int64_t id)
{
  auto iter=timer_map_.find(id);
  if(iter!=timer_map_.end())
    timer_map_.erase(iter);
}

int64_t base::Timer::gen_timer_uuid()
{
  return ++s_timer_id_;
}

void base::Timer::tick(int64_t now)
{
	while(!min_heap_.empty())
	{
		TimerTask* task=min_heap_.top();
		if(task->canceled())
		{
			min_heap_.pop();
      remove_timer_task(task->id_);
			delete task;
			continue;
		}
		if(!task->will_fire(now))
			break;
		else
		{
      task->run();
      min_heap_.pop();
      if(task->after_fire(now))
        min_heap_.push(task);
      else
      {
        remove_timer_task(task->id_);
        delete task;
      }
		}
	}
}

base::Timer::Timer():s_timer_id_(0)
{

}

base::Timer::~Timer()
{
  while(!min_heap_.empty())
  {
    TimerTask* task=min_heap_.top();
    delete task;
    min_heap_.pop();
  }
  timer_map_.clear();
}
