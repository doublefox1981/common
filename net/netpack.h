#ifndef _NETPACK_H
#define _NETPACK_H
#include "portable.h"
#include "../base/list.h"

namespace net
{
	struct ezNetPack 
	{
		uint16_t capacity_;
		uint16_t size_;
		char* data_;

		explicit ezNetPack(uint16_t s);
		virtual ~ezNetPack();
	};

	struct ezSendBlock
	{
		list_head lst_;
		int fd_;
		ezNetPack* pack_;

		ezSendBlock();
		virtual ~ezSendBlock();
	};
}

#endif