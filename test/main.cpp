// #include "../base/portable.h"
// #include "../base/memorystream.h"
// #include "../base/scopeguard.h"
// #include "../base/eztime.h"
// #include "../base/Timer.h"
// #include "../base/thread.h"
// #include "../base/util.h"
// #include "../base/logging.h"
// 
// #include "../net/net_interface.h"
// #include "../net/netpack.h"
// 
// #include <vector>
// #include <string>
// using namespace net;
// using namespace std;
// 
// enum EConnectToStatus
// {
//   ECTS_CONNECTING,
//   ECTS_CONNECTOK,
//   ECTS_DISCONNECT,
// };
// struct ConnectToInfo
// {
//   int64_t id_;
//   std::string ip_;
//   int port_;
//   int status_;
//   Connection* conn_;
// };
// 
// std::vector<ConnectToInfo> gConnSet;
// class TestClientHander:public net::IConnnectionHander
// {
// public:
//   virtual void on_open(Connection* conn)
//   {
//     for(size_t i=0;i<gConnSet.size();++i)
//     {
//       if(conection_user_data(conn)==gConnSet[i].id_)
//       {
//         gConnSet[i].status_=ECTS_CONNECTOK;
//         gConnSet[i].conn_=conn;
//       }
//     }
//   }
//   virtual void on_close(Connection* conn)
//   {
//     for(size_t i=0;i<gConnSet.size();++i)
//     {
//       if(conection_user_data(conn)==gConnSet[i].id_)
//       {
//         gConnSet[i].status_=ECTS_DISCONNECT;
//         gConnSet[i].conn_=NULL;
//       }
//     }
//   }
//   virtual void on_data(Connection* conn,Msg* msg){}
// };
// bool exit_=false;
// #ifdef __linux__
// #include <signal.h>
// void SignalExit(int no)
// {
//   exit_=true;
//   signal(no,SIG_DFL);
// }
// #else
// BOOL CtrlHandler(DWORD CtrlType)
// {
//   switch(CtrlType)
//   {
//   case CTRL_BREAK_EVENT:
//     {
//       exit_=true;
//       return TRUE;
//     }
//     break;
//   case  CTRL_C_EVENT:
//     {
//       exit_=true;
//       return TRUE;
//     }
//     break;
//   default: break;
//   }
//   return FALSE;
// }
// #endif
// 
// void ProcessSignal()
// {
// #ifdef __linux__
//   signal(SIGINT,SignalExit);
// #else
//   SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler,TRUE);
// #endif
// }
// 
// class A
// {
//   char a[5];
//   int i;
// };
// #include "../base/readerwriterqueue.h"
// #include <type_traits>
// int main()
// {
//   __declspec(align(64))
//   int a; 
//   __declspec(align(64))
//   int a1;
//   ProcessSignal();
//   size_t s1=std::alignment_of<A>::value;
//   moodycamel::ReaderWriterQueue<int> s;
//   s.enqueue(1);
//   base::Logger::instance()->Start();
//   const char* cc="hello world";
//   LOG_INFO("%08x ",cc);
//   std::string format;
//   base::string_printf(&format,"%08x",cc);
//   for(size_t len=0;len<strlen(cc);++len)
//   {
//     format+=" ";
//     base::string_appendf(&format,"%02x",char(cc[len]));
//   }
//   LOG_INFO("%s",format.c_str());
// 
//   net::net_initialize();
// 
//   net::IConnnectionHander* hander=new TestClientHander;
//   net::IDecoder* decoder=new net::MsgDecoder(20000);
//   net::IEncoder* encoder=new net::MsgEncoder;
//   EventLoop* ev=net::create_event_loop(hander,decoder,encoder,4);
//   for(int i=0;i<10;++i)
//   {
//     net::Connect(ev,"192.168.99.51",10011,i,10);
//     ConnectToInfo info={i,"192.168..99.51",10011,ECTS_CONNECTING,nullptr};
//     gConnSet.push_back(info);
//   }
// 
//   base::ScopeGuard guard([&](){net::destroy_event_loop(ev); delete hander; delete decoder; delete encoder;});
//   int seq=0;
//   while(!exit_)
//   {
//     net::event_process(ev);
//     base::sleep(1);
//     if((rand()%100)>95)
//     {
//       net::Connect(ev,"192.168.99.51",10011,0,10);
//       continue;
//     }
//     for(size_t s=0;s<gConnSet.size();++s)
//     {
//       Connection* conn=gConnSet[s].conn_;
//       if(!conn)
//         continue;
//       if((rand()%100)>90)
//       {
//         net::close_connection(conn);
//         continue;
//       }
//       for(int i=0;i<1;++i)
//       {
//         int ss=(rand()%15000+4);
//         Msg msg;
//         net::msg_init_size(&msg,ss);
//         base::BufferWriter writer((char*)net::msg_data(&msg),net::msg_size(&msg));
//         writer.Write(++seq);
//         net::msg_send(conn,&msg);
//       }
//     }
//   }
//   return 1;
// }

/*  Multi-producer/multi-consumer bounded queue
 *  Copyright (c) 2010-2011, Dmitry Vyukov
 *  Distributed under the terms of the GNU General Public License as published by the Free Software Foundation,
 *  either version 3 of the License, or (at your option) any later version.
 *  See: http://www.gnu.org/licenses
 */ 

#include <stdio.h>
#include <time.h>
#include <intrin.h>
#include <windows.h>
#include <process.h>
#include <iostream>
#include <string>
#include <assert.h>


enum memory_order
{
    memory_order_relaxed,
    memory_order_consume,
    memory_order_acquire,
    memory_order_release,
    memory_order_acq_rel,
    memory_order_seq_cst,
};

class atomic_uint
{
public:
    unsigned load(memory_order mo) const volatile
    {
        (void)mo;
        assert(mo == memory_order_relaxed
            || mo == memory_order_consume
            || mo == memory_order_acquire
            || mo == memory_order_seq_cst);
        unsigned v = val_;
        _ReadWriteBarrier();
        return v;
    }

    void store(unsigned v, memory_order mo) volatile
    {
        assert(mo == memory_order_relaxed
            || mo == memory_order_release
            || mo == memory_order_seq_cst);

        if (mo == memory_order_seq_cst)
        {
            _InterlockedExchange((long volatile*)&val_, (long)v);
        }
        else
        {
            _ReadWriteBarrier();
            val_ = v;
        }
    }

    bool compare_exchange_weak(unsigned& cmp, unsigned xchg, memory_order mo) volatile
    {
        unsigned prev = (unsigned)_InterlockedCompareExchange((long volatile*)&val_, (long)xchg, (long)cmp);
        if (prev == cmp)
            return true;
        cmp = prev;
        return false;
    }

private:
    unsigned volatile           val_;
};

template<typename T>
class atomic;

template<>
class atomic<unsigned> : public atomic_uint
{
};

namespace std
{
    using ::memory_order;
    using ::memory_order_relaxed;
    using ::memory_order_consume;
    using ::memory_order_acquire;
    using ::memory_order_release;
    using ::memory_order_acq_rel;
    using ::memory_order_seq_cst;
    using ::atomic_uint;
    using ::atomic;
};

template<typename T>
class mpmc_bounded_queue
{
public:
    mpmc_bounded_queue(size_t buffer_size)
        : buffer_(new cell_t [buffer_size])
        , buffer_mask_(buffer_size - 1)
    {
        assert((buffer_size >= 2) && ((buffer_size & (buffer_size - 1)) == 0));
        for (size_t i = 0; i != buffer_size; i += 1)
            buffer_[i].sequence_.store(i, std::memory_order_relaxed);
        enqueue_pos_.store(0, std::memory_order_relaxed);
        dequeue_pos_.store(0, std::memory_order_relaxed);
    }

    ~mpmc_bounded_queue()
    {
        delete [] buffer_;
    }

    bool enqueue(T const& data)
    {
        cell_t* cell;
        size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
        for (;;)
        {
            cell = &buffer_[pos & buffer_mask_];
            size_t seq = cell->sequence_.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t)seq - (intptr_t)pos;
            if (dif == 0)
            {
                if (enqueue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    break;
            }
            else if (dif < 0)
                return false;
            else
                pos = enqueue_pos_.load(std::memory_order_relaxed);
        }

        cell->data_ = data;
        cell->sequence_.store(pos + 1, std::memory_order_release);

        return true;
    }

    bool dequeue(T& data)
    {
        cell_t* cell;
        size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
        for (;;)
        {
            cell = &buffer_[pos & buffer_mask_];
            size_t seq = cell->sequence_.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
            if (dif == 0)
            {
                if (dequeue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    break;
            }
            else if (dif < 0)
                return false;
            else
                pos = dequeue_pos_.load(std::memory_order_relaxed);
        }

        data = cell->data_;
        cell->sequence_.store(pos + buffer_mask_ + 1, std::memory_order_release);

        return true;
    }

private:
    struct cell_t
    {
        std::atomic<size_t>     sequence_;
        T                       data_;
    };

    static size_t const         cacheline_size = 64;
    typedef char                cacheline_pad_t [cacheline_size];

    cacheline_pad_t             pad0_;
    cell_t* const               buffer_;
    size_t const                buffer_mask_;
    cacheline_pad_t             pad1_;
    std::atomic<size_t>         enqueue_pos_;
    cacheline_pad_t             pad2_;
    std::atomic<size_t>         dequeue_pos_;
    cacheline_pad_t             pad3_;

    mpmc_bounded_queue(mpmc_bounded_queue const&);
    void operator = (mpmc_bounded_queue const&);
};




size_t const thread_count = 4;
size_t const batch_size = 1;
size_t const iter_count = 2000000;

bool volatile g_start = 0;

typedef mpmc_bounded_queue<int> queue_t;


unsigned __stdcall thread_func(void* ctx)
{
    queue_t& queue = *(queue_t*)ctx;
    int data;

    srand((unsigned)time(0) + GetCurrentThreadId());
    size_t pause = rand() % 1000;

    while (g_start == 0)
        SwitchToThread();

    for (size_t i = 0; i != pause; i += 1)
        _mm_pause();

    for (int iter = 0; iter != iter_count; ++iter)
    {
        for (size_t i = 0; i != batch_size; i += 1)
        {
            while (!queue.enqueue(i))
                SwitchToThread();
        }
        for (size_t i = 0; i != batch_size; i += 1)
        {
            while (!queue.dequeue(data))
            SwitchToThread();
        }
    }

    return 0;
}



int main()
{
    queue_t queue (1024);

    HANDLE threads [thread_count];
    for (int i = 0; i != thread_count; ++i)
    {
        threads[i] = (HANDLE)_beginthreadex(0, 0, thread_func, &queue, 0, 0);
    }

    Sleep(1);

    unsigned __int64 start = __rdtsc();
    g_start = 1;

    WaitForMultipleObjects(thread_count, threads, 1, INFINITE);

    unsigned __int64 end = __rdtsc();
    unsigned __int64 time = end - start;
    std::cout << "cycles/op=" << time / (batch_size * iter_count * 2 * thread_count) << std::endl;
}




