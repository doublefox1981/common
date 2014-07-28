#include "logging.h"
#include "util.h"
#include "eztime.h"
#include <string>
#include <ctime>

namespace base
{
  enum ezLogType
  {
    ELOG_INFO,
    ELOG_WARNNING,
    ELOG_ERROR,
    ELOG_FATAL,
  };
  struct LogMessage
  {
    list_head next_;
    int       type_;
    std::string    str_;
    time_t time_;
  };
  LogMessage* create_log_message()
  {
    return new LogMessage;
  }
  void destroy_log_message(LogMessage* msg)
  {
    delete msg;
  }
}

void base::Logger::print(int type,const char* format,va_list args)
{
  LogMessage* msg=base::create_log_message();
  if(!msg)
    return;
  msg->time_=time(NULL);
  msg->type_=type;
  base::string_printf_impl(msg->str_,format,args);
  base::Locker locker(&mutex_);
  list_add_tail(&msg->next_,&lst_);
}

void base::Logger::info(const char* format,...)
{
  va_list va;
  va_start(va,format);
  print(base::ELOG_INFO,format,va);
  va_end(va);
}

void base::Logger::warn(const char* format,...)
{
  va_list va;
  va_start(va,format);
  print(base::ELOG_WARNNING,format,va);
  va_end(va);
}

void base::Logger::error(const char* format,...)
{
  va_list va;
  va_start(va,format);
  print(base::ELOG_ERROR,format,va);
  va_end(va);
}

void base::Logger::fatal(const char* format,...)
{
  va_list va;
  va_start(va,format);
  print(base::ELOG_FATAL,format,va);
  va_end(va);
}

base::Logger::~Logger()
{
  list_head* iter;
  list_head* next;
  list_for_each_safe(iter,next,&lst_)
  {
    LogMessage* log=list_entry(iter,LogMessage,next_);
    destroy_log_message(log);
  }
}

base::Logger::Logger() :log_level_(0)
{
  INIT_LIST_HEAD(&lst_);
}
#ifndef __linux__
static inline struct tm* localtime_r(const time_t* timep, struct tm* result) {
  localtime_s(result, timep);
  return result;
}
#ifndef snprintf
#define snprintf _snprintf
#endif
#endif
void base::Logger::run()
{
  while(!exit_)
  {
    LIST_HEAD(tmpqueue);
    {
      Locker lock(&mutex_);
      list_splice_init(&lst_,&tmpqueue);
    }
    int num=0;
    char prefix[16];
    char timeprefix[128];
    list_head* iter;
    list_head* next;
    list_for_each_safe(iter,next,&tmpqueue)
    {
      LogMessage* log=list_entry(iter,LogMessage,next_);
      if(log->type_>=log_level_)
      {
        EPrintColor color;
        switch(log->type_)
        {
        case ELOG_INFO:
          {
            color=COLOR_DEFAULT;
            snprintf(prefix,sizeof(prefix),"%s","INFO>>> ");
          }
          break;
        case ELOG_WARNNING:
          {
            color=COLOR_YELLOW;
            snprintf(prefix,sizeof(prefix),"%s","WARNNING>>> ");
          }
          break;
        case ELOG_ERROR:
          {
            color=COLOR_RED;
            snprintf(prefix,sizeof(prefix),"%s","ERROR>>> ");
          }
          break;
        case ELOG_FATAL:
          {
            color=COLOR_RED;
            snprintf(prefix,sizeof(prefix),"%s","FATAL>>> ");
          }
          break;
        default: break;
        }
        struct ::tm tm_time;
#ifdef __linux__
        localtime_r(&log->time_, &tm_time);
#else
        localtime_s(&tm_time,&log->time_);
#endif
        snprintf(timeprefix,sizeof(timeprefix),"%d-%02d-%02d %02d:%02d:%02d ",
        1900+tm_time.tm_year,
        1+tm_time.tm_mon,
        tm_time.tm_mday,
        tm_time.tm_hour,
        tm_time.tm_min,
        tm_time.tm_sec);
        printf("%s",timeprefix);
        colored_printf(color,"%s",prefix);
        printf("%s\n",log->str_.c_str());
        fflush(stdout);
      }
      destroy_log_message(log);
    }
    base::sleep(20);
  }
}
#undef snprintf