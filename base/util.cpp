#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#ifdef __linux__
#include <unistd.h>
#else
#include <Windows.h>
#include <io.h>
#endif

#ifdef __linux__
typedef struct stat StatStruct;
inline int FileNo(FILE* file) { return fileno(file); }
inline int IsATTY(int fd) { return isatty(fd); }
inline int StrCaseCmp(const char* s1, const char* s2) {return strcasecmp(s1, s2);}
const char* GetAnsiColorCode(base::EPrintColor color) 
{
  switch (color) {
  case base::COLOR_RED:     return "1";
  case base::COLOR_GREEN:   return "2";
  case base::COLOR_YELLOW:  return "3";
  default:            return NULL;
  };
}
#else

typedef struct _stat StatStruct;
inline int IsATTY(int fd) { return _isatty(fd); }
inline int FileNo(FILE* file) { return _fileno(file); }
inline int StrCaseCmp(const char* s1, const char* s2) {return _stricmp(s1, s2);}
inline WORD GetColorAttribute(base::EPrintColor color) {
  switch (color) {
  case base::COLOR_RED:    return FOREGROUND_RED;
  case base::COLOR_GREEN:  return FOREGROUND_GREEN;
  case base::COLOR_YELLOW: return FOREGROUND_RED | FOREGROUND_GREEN;
  default:           return 0;
  }
}
#endif

inline const char* GetEnv(const char* name) {return getenv(name);}

bool ShouldUseColor(bool stdout_is_tty) 
{
#ifndef __linux__
  return stdout_is_tty;
#else
  const char* const term = GetEnv("TERM");
  const bool term_supports_color =
    !StrCaseCmp(term, "xterm") ||
    !StrCaseCmp(term, "xterm-color") ||
    !StrCaseCmp(term, "xterm-256color") ||
    !StrCaseCmp(term, "screen") ||
    !StrCaseCmp(term, "linux") ||
    !StrCaseCmp(term, "cygwin");
  return stdout_is_tty && term_supports_color;
#endif 
}

void base::ColoredPrintf(EPrintColor color, const char* fmt, ...) 
{
  va_list args;
  va_start(args, fmt);
  static const bool in_color_mode = ShouldUseColor(IsATTY(FileNo(stdout)) != 0);
  const bool use_color = in_color_mode && (color != COLOR_DEFAULT);
  if(!use_color) 
  {
    vprintf(fmt, args);
    va_end(args);
    return;
  }

#ifndef __linux__
  const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO buffer_info;
  GetConsoleScreenBufferInfo(stdout_handle, &buffer_info);
  const WORD old_color_attrs = buffer_info.wAttributes;
  fflush(stdout);
  SetConsoleTextAttribute(stdout_handle,GetColorAttribute(color) | FOREGROUND_INTENSITY);
  vprintf(fmt, args);
  fflush(stdout);
  SetConsoleTextAttribute(stdout_handle, old_color_attrs);
#else
  printf("\033[0;3%sm", GetAnsiColorCode(color));
  vprintf(fmt, args);
  printf("\033[m");
  fflush(stdout);
#endif
  va_end(args);
}