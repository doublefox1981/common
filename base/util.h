#ifndef _UTIL_H
#define _UTIL_H

namespace base
{

  enum EPrintColor 
  {
    COLOR_DEFAULT,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW
  };

  void ColoredPrintf(EPrintColor color,const char* fmt,...); 
}
#endif