#ifndef _EZTIME_H
#define _EZTIME_H
#include <stdint.h>
#include <limits>
#include <string>
#include <ctime>

namespace base
{
	void sleep(int millisec);
	int64_t now_tick();
	void format_time(std::time_t t,std::string& str);	
}
#endif