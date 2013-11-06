#ifndef _EZTIME_H
#define _EZTIME_H
#include <stdint.h>
#include <limits>
#include <string>
#include <ctime>

namespace base
{
	void ezSleep(int millisec);
	int64_t ezNowTick();
	void ezFormatTime(std::time_t t,std::string& str);	
}
#endif