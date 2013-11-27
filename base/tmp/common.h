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

// ���볣������
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

//ʱ�亯��
uint64 GetCurrentSecond();
void SecondToDateTime(uint64 second,DateTime& dateTime);
uint64 DateTimeToSecond(const DateTime& dateTime);

//�����źŴ�����
void SetSignal(void (*func)(int));
bool IsUserSignal(int sig);

#ifndef _WIN32
void SignalCrash(int no);
void SetCrashHandler(void (*crash)(int),const char* gdbName);
#endif

//�������
//rand ����[0,RAND_MAX]��Χ�������
//GetRandom ����ָ����Χ���������
// ��b>a���õ���հ뿪����[a,b)
// ��b==a���õ���һ����[a,a]
// ��b<a���õ��뿪�������(b,a]
int GetRandom(int a,int b);

//��Χ����,���ر�����[a,b]����v�������
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

// Ŀǰ����������stl�ķ���
inline char* string_as_array(std::string* str)
{
  // DO NOT USE const_cast<char*>(str->data())!
  return str->empty() ? NULL : &*str->begin();
}

//�������
void FillArray(int* arr,int arrNum,int startFigure);
void ShuffleArray(int* arr,int arrNum);

void RandomString(int num,int minChar,int maxChar,std::string* str);

// ����ִ��ʱ��
//void GetFuncRunTime(void (*func)(void*),void* param,uint64& userTime,uint64& kernelTime);
#ifndef SIMPLE_COMMON_HEADER
void GetFuncRunTime(std::function<void(void)> func,uint64& userTime,uint64& kernelTime);
#endif
//дLog�ļ�
void AppendFile(const char *buf,int buflen,const char *filename);
void FlushFile(const char *filename);

}

#endif//_NETWORK_COMMON_H_

