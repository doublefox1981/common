#include "eztime.h"
#ifdef __linux__
#include <unistd.h>
#include <sys/time.h>
#else
#include <windows.h>
#endif

namespace
{
	bool time_initialized=false;
}
#ifdef __linux__
namespace {timeval ls;}
static void init_time()
{
	time_initialized=true;
	gettimeofday(&ls,0);
}
int64_t base::now_tick()
{
	if(!time_initialized)
		init_time();
	timeval le;
	gettimeofday(&le,0);
	return ((int64_t)le.tv_sec*1000+le.tv_usec/1000)-((int64_t)ls.tv_sec*1000+ls.tv_usec/1000);
}
#else
namespace
{
	LARGE_INTEGER l;
	LARGE_INTEGER ls;
}
static void init_time()
{
	time_initialized=true;
	QueryPerformanceFrequency(&l);
	QueryPerformanceCounter(&ls);
}

int64_t base::now_tick()
{
	if(!time_initialized)
		init_time();
	LARGE_INTEGER le;
	QueryPerformanceCounter(&le);
	return (int64_t)((double(le.QuadPart-ls.QuadPart))/double(l.QuadPart)*1000);
}
#endif

void base::sleep(int millisec)
{
#ifdef __linux__
	usleep(millisec*1000);
#else
	Sleep(millisec);
#endif
}

void base::format_time(std::time_t t,std::string& str)
{
	tm* timeinfo=NULL;
	char timestring[128];
	timeinfo=std::localtime(&t);
	if(!timeinfo) return;
	std::strftime(timestring,sizeof(timestring),"%Y-%m-%d %H:%M:%S",timeinfo);
	str=timestring;
}