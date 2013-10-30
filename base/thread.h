#ifndef _THREAD_H
#define _THREAD_H

#ifndef __linux__
#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#endif

namespace base
{
#ifndef __linux__
	class AtomicNumber
	{
	public:
		AtomicNumber(){mValue=0;}
		long Get(){return mValue;}
		void Set(int n){InterlockedExchange(&mValue,n);}
		void Inc(){InterlockedIncrement(&mValue);}
		void Dec(){InterlockedDecrement(&mValue);}
		void Add(int n){InterlockedExchangeAdd(&mValue,n);}
		void Sub(int n){InterlockedExchangeAdd(&mValue,-n);}
	private:
		volatile long mValue;
	};

	class Mutex
	{
	public:
		Mutex()
		{
			InitializeCriticalSection(&mutex);
		}
		~Mutex ()
		{
			DeleteCriticalSection(&mutex);
		}
		void Lock ()
		{
			EnterCriticalSection(&mutex);
		}
		void Unlock ()
		{
			LeaveCriticalSection(&mutex);
		}
	private:
		CRITICAL_SECTION mutex;
	};

	class Semaphore
	{
	public:
		Semaphore()
		{
			mSema=CreateSemaphore(NULL,0,MAXWORD,NULL);
		}
		~Semaphore()
		{
			CloseHandle(mSema);
		}
		void PostSignal()
		{
			ReleaseSemaphore(mSema,1,NULL);
		}
		void WaitSignal()
		{
			WaitForSingleObject(mSema,INFINITE);
		}
	private:
		HANDLE mSema;
	};

#else

	class AtomicNumber
	{
	public:
		AtomicNumber(){mValue=0;}
		long Get(){return mValue;}
		void Set(int n){__sync_lock_test_and_set(&mValue,n);}
		void Inc(){__sync_add_and_fetch(&mValue,1);}
		void Dec(){__sync_sub_and_fetch(&mValue,1);}
		void Add(int n){__sync_add_and_fetch(&mValue,n);}
		void Sub(int n){__sync_sub_and_fetch(&mValue,n);}
	private:
		volatile long mValue;
	};

	class Mutex
	{
	public:
		Mutex()
		{
			if(!attr_refcount++)
			{
				pthread_mutexattr_init (&attr);
				pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE_NP);
			}
			pthread_mutex_init (&mutex, &attr);
		}
		~Mutex ()
		{
			pthread_mutex_destroy (&mutex);
			if(!--attr_refcount)
				pthread_mutexattr_destroy (&attr);
		}
		void Lock ()
		{
			pthread_mutex_lock (&mutex);
		}
		void Unlock ()
		{
			pthread_mutex_unlock (&mutex);
		}
	private:
		pthread_mutex_t mutex;
		static pthread_mutexattr_t attr;
		static int attr_refcount;
	};

	class Semaphore
	{
	public:
		Semaphore()
		{
			sem_init(&mSema,0,0);
		}
		~Semaphore()
		{
			sem_destroy(&mSema);
		}
		void PostSignal()
		{
			sem_post(&mSema);
		}
		void WaitSignal()
		{
			sem_wait(&mSema);
		}
	private:
		sem_t mSema;
	};

#endif

	class SysEvent
	{
	public:
		void Set()
		{
			mSema.PostSignal();
		}
		void Wait()
		{
			mSema.WaitSignal();
		}
	private:
		Semaphore mSema;
	};

	class Locker
	{
	public:
		explicit Locker(Mutex *mutex){m_mutex = mutex;m_mutex->Lock();}
		~Locker(){m_mutex->Unlock();}
	private:
		Mutex *m_mutex;
	};

	enum ThreadPriority 
	{
		TP_IDLE,
		TP_LOWER,
		TP_LOW,
		TP_NORMAL,
		TP_HIGH,
		TP_HIGHER,
		TP_REALTIME
	};

#ifdef _WIN32
	typedef DWORD ThreadId;
#else
	typedef pthread_t ThreadId;
	inline ThreadId GetCurrentThreadId()
	{
		return pthread_self();
	}
#endif

// 	class Threads
// 	{
// 	public:
// 		virtual void Start();
// 		void setCurrentPriority( ThreadPriority priority );
// 		unsigned long getTick();
// 
// #ifdef _WIN32
// 		static DWORD WINAPI thread_func(void *data) 
// 		{
// #else
// 		static void * thread_func(void *data) 
// 		{
// #endif	
// 			Threads *thread = (Threads *)data;
// 			thread->Run();
// 			return 0;
// 		}
// 		virtual void Run() {}
// 
// 		Threads()
// 		{
// 			mbExit = false;
// 		}
// 		virtual ~Threads()
// 		{
// 		}
// 
// 		virtual void Stop()
// 		{
// 			mbExit = true;
// #ifndef _WIN32
// 			pthread_join(mthread,NULL);
// #else
// 			WaitForSingleObject(mthread,INFINITE);
// #endif
// 		}
// 
// 	private:
// 		int getPlatformPriority( ThreadPriority & priority );
// 		unsigned long mStartTick;
// 	protected:
// 		bool  mbExit;
// #ifndef _WIN32
// 		pthread_t mthread;
// #else
// 		HANDLE mthread;
// #endif
// 	};
}
#endif