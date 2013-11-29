#ifndef _BASE_LOGGING_H
#define _BASE_LOGGING_H
#include <cstdarg>
#include "list.h"
#include "thread.h"
#include "singleton.h"

namespace base
{
  class ezLogger:public Threads,public ezSingleTon<ezLogger> 
  {
  public:
    ezLogger();
    virtual ~ezLogger();
    void setLogLevel(int lvl){logLevel_=lvl;}
    void info(const char* format,...);
    void warn(const char* format,...);
    void error(const char* format,...);
    void fatal(const char* format,...);
    virtual void Run();
  private:
    void print(int type,const char* format,va_list args);
    base::SpinLock mutex_;
    list_head lst_;
    int logLevel_;
  };
}

#define LOG_INFO  base::ezLogger::instance()->info
#define LOG_WARN  base::ezLogger::instance()->warn
#define LOG_ERROR base::ezLogger::instance()->error
#define LOG_FATAL base::ezLogger::instance()->fatal
#endif