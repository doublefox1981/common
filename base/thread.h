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
  class MutexInterface
  {
  public:
    virtual ~MutexInterface(){}
    virtual void Lock()=0;
    virtual void Unlock()=0;
  };
#ifndef __linux__
	class AtomicNumber
	{
	public:
		AtomicNumber(){mValue=0;}
		long Get(){return mValue;}
		long Set(int n){return InterlockedExchange(&mValue,n);}
		long Inc(){return InterlockedIncrement(&mValue);}
		long Dec(){return InterlockedDecrement(&mValue);}
		long Add(int n){return InterlockedExchangeAdd(&mValue,n);}
		long Sub(int n){return InterlockedExchangeAdd(&mValue,-n);}
    bool Cas(long cmp,long newv){return InterlockedCompareExchange(&mValue,newv,cmp)==cmp;}
	private:
		volatile long mValue;
	};

  class AtomicPtr
  {
  public:
    AtomicPtr():ptr_(nullptr){}
    void Set(void* p){ptr_=p;} // non-threadsafe
    void* Xchg(void* p){return InterlockedExchangePointer (&ptr_,p);}
    void* Cas(void* cmp,void* p){return InterlockedCompareExchangePointer((volatile PVOID*)&ptr_,p, cmp);}
  private:
    AtomicPtr(const AtomicPtr&);
    AtomicPtr& operator=(const AtomicPtr&);
    volatile void* ptr_;
  };

	class Mutex:public MutexInterface
	{
	public:
		Mutex()
		{
			InitializeCriticalSection(&mutex);
		}
		virtual ~Mutex ()
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
			mSema=CreateSemaphore(nullptr,0,MAXWORD,nullptr);
		}
		~Semaphore()
		{
			CloseHandle(mSema);
		}
		void PostSignal()
		{
			ReleaseSemaphore(mSema,1,nullptr);
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
		long Set(int n){return __sync_lock_test_and_set(&mValue,n);}
		long Inc(){return __sync_add_and_fetch(&mValue,1);}
		long Dec(){return __sync_sub_and_fetch(&mValue,1);}
		long Add(int n){return __sync_add_and_fetch(&mValue,n);}
		long Sub(int n){return __sync_sub_and_fetch(&mValue,n);}
    bool Cas(long cmp,long newv) {return __sync_bool_compare_and_swap(&mValue,cmp,newv);}
	private:
		volatile long mValue;
	};

  class AtomicPtr
  {
  public:
    AtomicPtr():ptr_(nullptr){}
    void Set(void* p){ptr_=p;} // non-threadsafe
    void* Xchg(void* p){return (void*)__sync_lock_test_and_set((long long*)ptr_,p);}
    void* Cas(void* cmp,void* p){return (void*)__sync_val_compare_and_swap((long long*)ptr_,cmp,p);}
  private:
    AtomicPtr(const AtomicPtr&);
    AtomicPtr& operator=(const AtomicPtr&);
    volatile void* ptr_;
  };

	class Mutex:public MutexInterface
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
		virtual ~Mutex ()
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

  class Sleeper 
  {
    static const unsigned kMaxActiveSpin = 4000;
    unsigned spinCount;
  public:
    Sleeper() : spinCount(0) {}
    void wait() {
#ifndef __linux__
      if(spinCount < kMaxActiveSpin)
      {
        ++spinCount;
        __asm nop;
      }else
      {
        // windows temp
        Sleep(0);
      }
#else
      if (spinCount < kMaxActiveSpin) 
      {
        ++spinCount;
        asm volatile("pause");
      } else 
      {
        /*
        * Always sleep 0.5ms, assuming this will make the kernel put
        * us down for whatever its minimum timer resolution is (in
        * linux this varies by kernel version from 1ms to 10ms).
        */
        struct timespec ts = { 0, 500000 };
        nanosleep(&ts, NULL);
      }
#endif
    }
  };

  class SpinLock:public MutexInterface
  {
  public:
    SpinLock(){lock_.Set(FREE);}
    virtual void Lock()
    {
      Sleeper sleeper;
      do 
      {
        sleeper.wait();
      } while(!lock_.Cas(FREE,LOCKED));
    }
    virtual void Unlock()
    {
      lock_.Set(FREE);
    }
  private:
    enum { FREE = 0, LOCKED = 1 };
    AtomicNumber lock_;
  };

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
		explicit Locker(MutexInterface *mutex){mutex_ = mutex;mutex_->Lock();}
		~Locker(){mutex_->Unlock();}
	private:
		MutexInterface* mutex_;
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

#ifdef _WIN32
  typedef HANDLE pthread_t;
#endif

	class Threads
	{
	public:
    Threads();
    virtual ~Threads();
		virtual void  start();
    virtual void  run(){}
		void          set_current_priority(ThreadPriority priority);
		unsigned long get_tick();
		virtual void  stop();
    virtual void  join();
  protected:
    bool      exit_;
    pthread_t thread_;
  private:
    int get_platform_priority(ThreadPriority& priority);
    unsigned long start_tick_;
	};
}
#endif