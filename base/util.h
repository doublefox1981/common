#ifndef _UTIL_H
#define _UTIL_H
#include <string>

#if defined(_MSC_VER)
#define MSVC_PUSH_DISABLE_WARNING(n) __pragma(warning(push)) \
  __pragma(warning(disable:n))
#define MSVC_POP_WARNING() __pragma(warning(pop))
#else
#define MSVC_PUSH_DISABLE_WARNING(n)
#define MSVC_POP_WARNING()
#endif


namespace base
{
  enum EPrintColor 
  {
    COLOR_DEFAULT,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW
  };

  extern void ColoredPrintf(EPrintColor color,const char* fmt,...);
  extern void StringPrintfImpl(std::string& output, const char* format,va_list args); 
  extern std::string StringPrintf(const char* format, ...);
  extern std::string& StringAppendf(std::string* output, const char* format, ...);
  extern void StringPrintf(std::string* output, const char* format, ...);
}
#endif