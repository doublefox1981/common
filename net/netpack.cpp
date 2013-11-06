#include "netpack.h"
#include "buffer.h"

net::ezNetPack::ezNetPack(uint16_t s)
{
	assert(s<(ezBuffer::ezInitSize-2));
	capacity_=s;
	size_=0;
	data_=new char[s];
}

net::ezNetPack::~ezNetPack()
{
	if(data_)
		delete[] data_;
}


net::ezSendBlock::ezSendBlock():fd_(-1),pack_(nullptr)
{
}

net::ezSendBlock::~ezSendBlock()
{
	if(pack_)
		delete pack_;
}
