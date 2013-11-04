#include "netpack.h"

net::ezNetPack::ezNetPack(uint16_t s)
{
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
