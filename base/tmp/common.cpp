#include <fstream>
#include <signal.h>
#include "common.h"
#include "memalloc.h"
#include <time.h>
#include <stdlib.h>
#include <memory.h>
#include <algorithm>


// portable.h function

#ifdef _WIN32

#include <mmsystem.h>

#pragma comment(lib,"winmm")
#pragma comment(lib,"wldap32")

uint32 GetMiniSecond()
{
    return timeGetTime();
}
int snprintf(char* buf,size_t bufsize,const char* format,...)
{
    assert(bufsize>0);
    va_list ap;
    va_start(ap,format);
    int len=_vsnprintf(buf,bufsize-1,format,ap);
    va_end(ap);
    assert(len>=0&&len<=(int)(bufsize)-1);
    buf[len]='\0';
    return len;
}

#else

#include <sys/time.h>

uint32 GetMiniSecond()
{
  struct timeval tv;
  gettimeofday(&tv,0);
  return tv.tv_sec*1000 + tv.tv_usec/1000;
}

#endif

using std::fstream;
using std::ios_base;

namespace Cross
{

// 该方法太慢
//int GetBitsNumber(uint64 bits)
//{
//    int num=0;
//    while(bits!=0)
//    {
//        bits=((bits-1)&bits);
//        ++num;
//    }
//    return num;
//}

//归并法
int GetBitsNumber(uint64 bits)
{
    const uint64 b0=0x5555555555555555;
    const uint64 b1=0x3333333333333333;
    const uint64 b2=0x0f0f0f0f0f0f0f0f;
    const uint64 b3=0x00ff00ff00ff00ff;
    const uint64 b4=0x0000ffff0000ffff;
    const uint64 b5=0x00000000ffffffff;
    bits=(bits&b0)+((bits>>1)&b0);
    bits=(bits&b1)+((bits>>2)&b1);
    bits=(bits&b2)+((bits>>4)&b2);
    bits=(bits&b3)+((bits>>8)&b3);
    bits=(bits&b4)+((bits>>16)&b4);
    bits=(bits&b5)+((bits>>32)&b5);
    return static_cast<int>(bits);
}

// 返回r满足,r<=log(v)<r+1
//二分查找法,找到最高位1的位置
uint32 CalcLog2Base(uint32 v)
{
    uint32 r =     (v > 0xFFFF) << 4; v >>= r;
    uint32 shift = (v > 0xFF  ) << 3; v >>= shift; r |= shift;
    shift = (v > 0xF   ) << 2; v >>= shift; r |= shift;
    shift = (v > 0x3   ) << 1; v >>= shift; r |= shift;
                                            r |= (v >> 1);
    return r;
}

//size_t string2cstr(const std::string &str,char *buf,size_t bufsize)
//{
//  assert(bufsize>0);
//  size_t len=str.copy(buf,bufsize-1);
//  assert(len<=bufsize-1);
//  buf[len]='\0';
//  return len;
//}

//时间函数
uint64 GetCurrentSecond()
{
    return (uint64)time(0);
}
void SecondToDateTime(uint64 second,DateTime& dateTime)
{
    time_t tt=(time_t)second;
    struct tm tm1;
#ifndef _WIN32
    localtime_r(&tt,&tm1);
#elif defined(_MSC_VER)&& _MSC_VER>=1400
    localtime_s(&tm1,&tt);
#else
    {
        static UserMutex timeMutex;
        Locker lock(&timeMutex);
        memcpy(&tm1,localtime(&tt),sizeof(tm1));
    }
#endif
    dateTime.Year=tm1.tm_year+1900;
    dateTime.Month=tm1.tm_mon+1;
    dateTime.Date=tm1.tm_mday;//从1开始
    dateTime.Hour=tm1.tm_hour;
    dateTime.Minute=tm1.tm_min;
    dateTime.Second=tm1.tm_sec;
    dateTime.WeekDay=tm1.tm_wday;//0-6
    dateTime.YearDay=tm1.tm_yday;//从0开始
}
uint64 DateTimeToSecond(const DateTime& dateTime)
{
    struct tm tm1;
    tm1.tm_year=dateTime.Year-1900;
    tm1.tm_mon=dateTime.Month-1;
    tm1.tm_mday=dateTime.Date;
    tm1.tm_hour=dateTime.Hour;
    tm1.tm_min=dateTime.Minute;
    tm1.tm_sec=dateTime.Second;
    tm1.tm_wday=dateTime.WeekDay;
    tm1.tm_yday=dateTime.YearDay;
    tm1.tm_isdst=-1;
    return mktime(&tm1);
}

#ifdef _WIN32
static void(*GCtrlHandler)(int);
static BOOL CtrlHandler(DWORD ctrlType)
{
    if(ctrlType==CTRL_BREAK_EVENT)
    {
        (*GCtrlHandler)(CTRL_BREAK_EVENT);
        return true;
    }
    return false;
}
#endif

void SetSignal(void (*func)(int))
{
  signal(SIGINT,func);
  signal(SIGTERM,func);
#ifdef _WIN32
  //signal(SIGBREAK,func);
  GCtrlHandler=func;
  SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler,TRUE);
#else
  signal(SIGUSR1,func);
  signal(SIGHUP,SIG_IGN);
#endif
}

#ifndef _WIN32

static char SCrashGdbName[8];

void SetCrashHandler(void (*crash)(int),const char* gdbName)
{
    signal(SIGSEGV,crash);
    signal(SIGABRT,crash);
    signal(SIGFPE,crash);
    if(gdbName)
        snprintf(SCrashGdbName,sizeof(SCrashGdbName),"%s",gdbName);
    else
        memset(SCrashGdbName,0,sizeof(SCrashGdbName));
}

void SignalCrash(int no)
{
  char Exe[128];
  char FileName[128];
  char ShellCmd[1024];
  int Pid=getpid();

  snprintf(ShellCmd,sizeof(ShellCmd),"/proc/%d/exe",Pid);
  int Len = readlink(ShellCmd,Exe,sizeof(Exe));
  Exe[Len] = '\0';

  fprintf(stderr,"\n\n\n********Server crashed********\n\n\n");
  fprintf(stderr,"Please find crash dump at %s",getcwd(NULL,0));

  Cross::DateTime date;
  uint64 second;
  second=Cross::GetCurrentSecond();
  SecondToDateTime(second,date);

  snprintf(FileName,sizeof(FileName),"CrashInfo-%04d-%02d-%02d_%02d-%02d-%02d.txt",
      date.Year, date.Month, date.Date, date.Hour, date.Minute, date.Second);
  snprintf(ShellCmd,sizeof(ShellCmd),"echo '--------------------------------Server CrashDump------------------------------' >> %s",FileName);
  system(ShellCmd);
  snprintf(ShellCmd,sizeof(ShellCmd),"/sbin/ifconfig | grep 'inet addr' >> %s \n\n\n\n  cat /proc/meminfo >> %s \n\n\n\n cat /proc/cpuinfo >> %s \n\n\n\n",FileName,FileName,FileName);
  system(ShellCmd);
  snprintf(ShellCmd,sizeof(ShellCmd),"echo '---------------------------------Process status-------------------------------' >> %s",FileName);
  system(ShellCmd);
  snprintf(ShellCmd,sizeof(ShellCmd),"cat /proc/%d/status >> %s",Pid,FileName);
  system(ShellCmd);
  snprintf(ShellCmd,sizeof(ShellCmd),"echo '----------------------------Process opend file status-------------------------' >> %s",FileName);
  system(ShellCmd);
  snprintf(ShellCmd,sizeof(ShellCmd),"find /proc/%d/fd -exec readlink {} \\; >> %s",Pid,FileName);
  system(ShellCmd);
  snprintf(ShellCmd,sizeof(ShellCmd),"echo '---------------------------------GDB dump stack-------------------------------' >> %s",FileName);
  system(ShellCmd);
  if(strlen(SCrashGdbName)==0)
    snprintf(ShellCmd,sizeof(ShellCmd),"gdb %s %d -x gdbbatchfile >> %s",Exe,Pid,FileName);
  else
    snprintf(ShellCmd,sizeof(ShellCmd),"%s %s %d -x gdbbatchfile >> %s",SCrashGdbName,Exe,Pid,FileName);
  system(ShellCmd);

  exit(0);
}

#endif

bool IsUserSignal(int sig)
{
#ifdef _WIN32
    return sig==CTRL_BREAK_EVENT;
#else
    return sig==SIGUSR1;
#endif
}

int GetRandom(int a,int b)
{
    return a+int(double(rand())*(double(b)-double(a))/(double(RAND_MAX)+1));
}

//struct ShuffleElem
//{
//    int num;
//    int rand;
//};
//struct ShuffleLess
//{
//    bool operator()(const ShuffleElem& e1,const ShuffleElem& e2)
//    {
//        return e1.rand<e2.rand;
//    }
//};
//
//void ShuffleArray(int* arr,int arrNum,int startFigure)
//{
//    StackMemory mem(sizeof(ShuffleElem)*arrNum);
//    ShuffleElem* elem=mem.Get<ShuffleElem>();
//    for(int i=0;i<arrNum;++i)
//    {
//        elem[i].num=i+startFigure;
//        elem[i].rand=rand();
//    }
//    std::sort(&elem[0],&elem[arrNum],ShuffleLess());
//    for(int i=0;i<arrNum;++i)
//    {
//        arr[i]=elem[i].num;
//    }
//}

void FillArray(int* arr,int arrNum,int startFigure)
{
    for(int i=0;i<arrNum;++i)
    {
        arr[i]=i+startFigure;
    }
}
void ShuffleArray(int* arr,int arrNum)
{
    for(int i=arrNum-1;i>=1;--i)
    {
        int r=GetRandom(0,i+1);//[0,i]的下标
        std::swap(arr[r],arr[i]);
    }
}

void RandomString(int num,int minChar,int maxChar,std::string* str)
{
    str->reserve(str->size()+num);
    for(int i=0;i<num;++i)
    {
        int r=GetRandom(minChar,maxChar);
        str->push_back(r);
    }
}

//void GetFuncRunTime(void (*func)(void*),void* param,uint64& userTime,uint64& kernelTime)
//{
//    FILETIME createTime;
//    FILETIME exitTime;
//    FILETIME beginKernelTime;
//    FILETIME endKernelTime;
//    FILETIME beginUserTime;
//    FILETIME endUserTime;
//    GetThreadTimes(GetCurrentThread(),&createTime,&exitTime,&beginKernelTime,&beginUserTime);
//    func(param);
//    GetThreadTimes(GetCurrentThread(),&createTime,&exitTime,&endKernelTime,&endUserTime);
//    userTime=((uint64)(endUserTime.dwHighDateTime-beginUserTime.dwHighDateTime)<<32) | (endUserTime.dwLowDateTime-beginUserTime.dwLowDateTime);
//    kernelTime=((uint64)(endKernelTime.dwHighDateTime-beginKernelTime.dwHighDateTime)<<32) | (endKernelTime.dwLowDateTime-beginKernelTime.dwLowDateTime);
//}
#if defined(_WIN32)&&!defined(SIMPLE_COMMON_HEADER)
void GetFuncRunTime(std::function<void(void)> func,uint64& userTime,uint64& kernelTime)
{
    FILETIME createTime;
    FILETIME exitTime;
    FILETIME beginKernelTime;
    FILETIME endKernelTime;
    FILETIME beginUserTime;
    FILETIME endUserTime;
    GetThreadTimes(GetCurrentThread(),&createTime,&exitTime,&beginKernelTime,&beginUserTime);
    func();
    GetThreadTimes(GetCurrentThread(),&createTime,&exitTime,&endKernelTime,&endUserTime);
    userTime=((uint64)(endUserTime.dwHighDateTime-beginUserTime.dwHighDateTime)<<32) | (endUserTime.dwLowDateTime-beginUserTime.dwLowDateTime);
    kernelTime=((uint64)(endKernelTime.dwHighDateTime-beginKernelTime.dwHighDateTime)<<32) | (endKernelTime.dwLowDateTime-beginKernelTime.dwLowDateTime);
}
#endif

//写文件操作
class FileLog
{
public:
  ~FileLog();
  void Append(const char *buf,int buflen,const char *filename);
  void Flush(const char *filename);
private:
  typedef STLMap<String,fstream *>::type FileMap;
  FileMap mfile;
};

void FileLog::Append(const char *buf,int buflen,const char *filename)
{
  FileMap::iterator i=mfile.find(filename);
  if(i!=mfile.end())
  {
    i->second->write(buf,buflen);
    i->second->write("\n",1);
  }
  else
  {
    fstream *f=new fstream(filename,ios_base::out|ios_base::app);
    if(!(*f))
    {
      assert(0);
      throw String("open file fail!");
    }
    mfile.insert(FileMap::value_type(filename,f));
    f->write(buf,buflen);
    f->write("\n",1);
  }
}

void FileLog::Flush(const char *filename)
{
  FileMap::iterator i=mfile.find(filename);
  if(i!=mfile.end())
  {
    i->second->flush();
  }
}

FileLog::~FileLog()
{
  for(FileMap::iterator iter=mfile.begin();iter!=mfile.end();++iter)
  {
    delete iter->second;
  }
  mfile.clear();
}

static FileLog *GetFileLog()
{
  static FileLog flog;
  return &flog;
}

void AppendFile(const char *buf,int buflen,const char *filename)
{
  GetFileLog()->Append(buf,buflen,filename);
}

void FlushFile(const char *filename)
{
  GetFileLog()->Flush(filename);
}

}