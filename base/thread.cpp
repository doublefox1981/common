#include "thread.h"
 
namespace base
{
#ifdef __linux__
	pthread_mutexattr_t Mutex::attr;
	int Mutex::attr_refcount = 0;
#endif


	void Threads::set_current_priority(ThreadPriority priority)
  {
#ifdef _WIN32
		SetThreadPriority(GetCurrentThread( ),get_platform_priority(priority));
#else
		sched_param schedparams;
		schedparams.sched_priority=get_platform_priority(priority);
		pthread_setschedparam(pthread_self(),SCHED_OTHER,&schedparams);
#endif
	}

	int Threads::get_platform_priority(ThreadPriority& priority) 
  {
		switch(priority) 
    {
#ifdef _WIN32
		case TP_IDLE:
			return THREAD_PRIORITY_IDLE;
		case TP_LOWER:
			return THREAD_PRIORITY_LOWEST;
		case TP_LOW:
			return THREAD_PRIORITY_BELOW_NORMAL;
		case TP_NORMAL:
			return THREAD_PRIORITY_NORMAL;
		case TP_HIGH:
			return THREAD_PRIORITY_ABOVE_NORMAL;
		case TP_HIGHER:
			return THREAD_PRIORITY_HIGHEST;
		case TP_REALTIME:
			return THREAD_PRIORITY_TIME_CRITICAL;
		default:
			return THREAD_PRIORITY_NORMAL;
#else
		case TP_IDLE:
			return 45;
		case TP_LOWER:
			return 51;

		case TP_LOW:
			return 57;
		case TP_NORMAL:
			return 63;
		case TP_HIGH:
			return 69;
		case TP_HIGHER:
			return 75;
		case TP_REALTIME:
			return 81;
		default:
			return 63; 
#endif
		}
	}

	unsigned long Threads::get_tick()
	{
#ifdef _WIN32
		return GetTickCount()-start_tick_;
#else
		timeval tv;
		gettimeofday(&tv,nullptr);
		return (tv.tv_sec*1000+tv.tv_usec/1000)-start_tick_;
#endif
	}

#ifdef _WIN32
  static DWORD WINAPI thread_func(void *data) 
  {
#else
  static void* thread_func(void *data) 
  {
#endif	
    Threads *thread = (Threads *)data;
    thread->run();
    return 0;
  }

	void Threads::start()
	{
#ifdef _WIN32
		thread_=CreateThread( nullptr, 0, (LPTHREAD_START_ROUTINE)thread_func, (void*)this, 0, 0 ) ;
		start_tick_=GetTickCount();
#else
		timeval tv;
		gettimeofday(&tv,nullptr);
		start_tick_ = tv.tv_sec*1000 + tv.tv_usec/1000;
		pthread_t thread;
		pthread_attr_t threadattributes;
		pthread_attr_init( &threadattributes );
		pthread_create( &thread_, &threadattributes, thread_func, (void*)this );
#endif
	}

  Threads::Threads()
  {
    exit_=false;
  }

  Threads::~Threads()
  {
  }

  void Threads::stop()
  {
    exit_=true;
  }

  void Threads::join()
  {
#ifndef _WIN32
    pthread_join(thread_,nullptr);
#else
    WaitForSingleObject(thread_,INFINITE);
#endif
  }

}