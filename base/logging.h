#ifndef _BASE_LOGGING_H
#define _BASE_LOGGING_H
#include <cstdarg>
#include "list.h"
#include "thread.h"
#include "singleton.h"

namespace base
{
  class Logger:public Threads,public SingleTon<Logger> 
  {
  public:
    Logger();
    virtual ~Logger();
    void set_log_level(int lvl){log_level_=lvl;}
    void info(const char* format,...);
    void warn(const char* format,...);
    void error(const char* format,...);
    void fatal(const char* format,...);
    virtual void run();
  private:
    void print(int type,const char* format,va_list args);
    base::SpinLock mutex_;
    list_head lst_;
    int log_level_;
    AtomicNumber log_num_;
  };
}

#define LOG_INFO  base::Logger::instance()->info
#define LOG_WARN  base::Logger::instance()->warn
#define LOG_ERROR base::Logger::instance()->error
#define LOG_FATAL base::Logger::instance()->fatal
#endif