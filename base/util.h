#ifndef _UTIL_H
#define _UTIL_H
#include <string>

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