#include "thread.h"
 
namespace base
{
// #ifdef __linux__
// pthread_mutexattr_t Mutex::attr;
// int Mutex::attr_refcount = 0;
// #endif
// 
// 
// void Threads::setCurrentPriority( ThreadPriority priority ) {
// #ifdef _WIN32
// 	SetThreadPriority( GetCurrentThread( ), getPlatformPriority( priority ) );
// #else
// 	sched_param schedparams;
// 	schedparams.sched_priority = getPlatformPriority( priority );
// 	pthread_setschedparam( pthread_self( ), SCHED_OTHER, &schedparams );
// #endif
// }
// 
// int Threads::getPlatformPriority( ThreadPriority &priority ) {
// 	switch( priority ) {
// #ifdef _WIN32
// 	case TP_IDLE:
// 		return THREAD_PRIORITY_IDLE;
// 	case TP_LOWER:
// 		return THREAD_PRIORITY_LOWEST;
// 	case TP_LOW:
// 		return THREAD_PRIORITY_BELOW_NORMAL;
// 	case TP_NORMAL:
// 		return THREAD_PRIORITY_NORMAL;
// 	case TP_HIGH:
// 		return THREAD_PRIORITY_ABOVE_NORMAL;
// 	case TP_HIGHER:
// 		return THREAD_PRIORITY_HIGHEST;
// 	case TP_REALTIME:
// 		return THREAD_PRIORITY_TIME_CRITICAL;
// 	default:
// 		return THREAD_PRIORITY_NORMAL;
// #else
// 	case TP_IDLE:
// 		return 45;
// 	case TP_LOWER:
// 		return 51;
// 
// 	case TP_LOW:
// 		return 57;
// 	case TP_NORMAL:
// 		return 63;
// 	case TP_HIGH:
// 		return 69;
// 	case TP_HIGHER:
// 		return 75;
// 	case TP_REALTIME:
// 		return 81;
// 	default:
// 		return 63; 
// #endif
// 	}
// }
// 
// unsigned long Threads::getTick()
// {
// #ifdef _WIN32
//   return GetTickCount() - mStartTick;
// #else
//   timeval tv;
//   gettimeofday(&tv,NULL);
//   return (tv.tv_sec*1000 + tv.tv_usec/1000) - mStartTick;
// #endif
// }
// 
// 
// void Threads::Start()
// {
// #ifdef _WIN32
//   mthread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)thread_func, (void*)this, 0, 0 ) ;
//   mStartTick = GetTickCount();
// #else
//   timeval tv;
//   gettimeofday(&tv,NULL);
//   mStartTick = tv.tv_sec*1000 + tv.tv_usec/1000;
//   pthread_t thread;
//   pthread_attr_t threadattributes;
//   pthread_attr_init( &threadattributes );
//   pthread_create( &mthread, &threadattributes, thread_func, (void*)this );
// #endif
// }
}