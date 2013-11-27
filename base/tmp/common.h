#ifndef _NETWORK_COMMON_H_
#define _NETWORK_COMMON_H_

#include "portable.h"
#include <string>
#include <memory.h>
#include <assert.h>
#include <functional>

#define for_each_iter(Type,container,iter) \
  for(Type::iterator iter=container.begin();iter!=container.end();++iter)

#define sizeofa(array) (sizeof(array)/sizeof(array[0]))

#define ClassNoneCopyable(type) \
    type& operator=(const type&); \
    type(const type&);

#define LL_FORMAT_D  "%"LL_FORMAT"d"
#define LL_FORMAT_U  "%"LL_FORMAT"u"

#ifndef NULL
#define NULL 0
#endif

// 导入常用容器
#include <algorithm>
#include <vector>
#include "list.h"

using std::vector;
using std::string;

namespace Cross
{
template<class T>
inline void delete_safe(T*& p)
{
    if(p)
    {
        delete p;
        p=NULL;
    }
}

#ifndef SIMPLE_COMMON_HEADER
template<class T>
class AutoDestroy
{
public:
    ~AutoDestroy()
    {
        _func(_data);
    }
    explicit AutoDestroy(T* data,std::function<void (T*)> func)
        :_data(data)
        ,_func(func)
    {}
private:
    T* _data;
    std::function<void (T*)> _func;
};
#define AUTO_DESTROY(type,var,func) Cross::AutoDestroy<type> auto_destroy_##var(var,func)

template<class T>
inline void destroy_safe(T*& obj,std::function<void (T*)> func)
{
    if(obj)
    {
        func(obj);
        obj=nullptr;
    }
}

#endif//SIMPLE_COMMON_HEADER

int GetBitsNumber(uint64 bits);
uint32 CalcLog2Base(uint32 v);

struct DateTime
{
    int16 Year;
    int16 Month;
    int16 Date;
    int16 Hour;
    int16 Minute;
    int16 Second;
    int16 WeekDay;
    int16 YearDay;
};

//时间函数
uint64 GetCurrentSecond();
void SecondToDateTime(uint64 second,DateTime& dateTime);
uint64 DateTimeToSecond(const DateTime& dateTime);

//设置信号处理函数
void SetSignal(void (*func)(int));
bool IsUserSignal(int sig);

#ifndef _WIN32
void SignalCrash(int no);
void SetCrashHandler(void (*crash)(int),const char* gdbName);
#endif

//随机函数
//rand 产生[0,RAND_MAX]范围的随机数
//GetRandom 产生指定范围的随机数。
// 若b>a，得到半闭半开区间[a,b)
// 若b==a，得到单一的数[a,a]
// 若b<a，得到半开半闭区间(b,a]
int GetRandom(int a,int b);

//范围设置,返回闭区间[a,b]中离v最近的数
template<class T>
T GetRange(T a,T b,T v)
{
    if(a<=b)
    {
        T r=v>=a?v:a;
        return r<=b?r:b;
    }
    return GetRange(b,a,v);
}

// 目前适用于所有stl的方法
inline char* string_as_array(std::string* str)
{
  // DO NOT USE const_cast<char*>(str->data())!
  return str->empty() ? NULL : &*str->begin();
}

//随机数组
void FillArray(int* arr,int arrNum,int startFigure);
void ShuffleArray(int* arr,int arrNum);

void RandomString(int num,int minChar,int maxChar,std::string* str);

// 函数执行时间
//void GetFuncRunTime(void (*func)(void*),void* param,uint64& userTime,uint64& kernelTime);
#ifndef SIMPLE_COMMON_HEADER
void GetFuncRunTime(std::function<void(void)> func,uint64& userTime,uint64& kernelTime);
#endif
//写Log文件
void AppendFile(const char *buf,int buflen,const char *filename);
void FlushFile(const char *filename);

}

#endif//_NETWORK_COMMON_H_

