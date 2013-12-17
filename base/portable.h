#ifndef _BASE_PORTABLE_H_
#define _BASE_PORTABLE_H_

typedef char int8;
typedef short int16;
typedef int int32;
typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef unsigned char byte;
#include <stdint.h>
#include <cstddef>

#ifdef __linux__

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#define LONGLONG(n) n##LL
#define ULONGLONG(n) n##ULL
#define LL_FORMAT "ll"
typedef long long          int64;
typedef unsigned long long uint64;
typedef int fd_t;
#define INVALID_SOCKET  (int)(~0)
#define SOCKET_ERROR    (-1)

inline uint32 GetMiniSecond()
{
  struct timeval tv;
  gettimeofday(&tv,0);
  return tv.tv_sec*1000 + tv.tv_usec/1000;
}

#else

#ifndef FD_SETSIZE
#define FD_SETSIZE 16000
#endif
#include <winsock2.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <mmsystem.h>
#include <io.h>

#define LONGLONG(n) n##I64
#define ULONGLONG(n) n##UI64
#define LL_FORMAT "I64"
typedef __int64          int64;
typedef unsigned __int64 uint64;
typedef int socklen_t;
typedef SOCKET fd_t;

#pragma comment(lib,"winmm")
#pragma comment(lib,"wldap32")
#define snprintf _snprintf
inline uint32 GetMiniSecond(){return timeGetTime();}
#define atoll(str) _atoi64(str)
#undef max
#undef min

#endif
#endif
