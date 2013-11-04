#include "buffer.h"
#include "socket.h"
#include <string.h>

net::ezBuffer::ezBuffer(size_t initSize/*=ezInitSize*/)
{
	buffer_=new char[initSize];
	capacity_=initSize;
	readPos_=0;
	writePos_=0;
}

net::ezBuffer::~ezBuffer()
{
	if(buffer_)
		delete[] buffer_;
}

void net::ezBuffer::reset()
{
	readPos_=0;
	writePos_=0;
}

size_t net::ezBuffer::getReadable(char*& buff)
{
	if(readPos_<writePos_)
	{
		buff=buffer_+readPos_;
		return readableSize();
	}
	else
	{
		return 0;
	}
}

size_t net::ezBuffer::getWritable(char*& buff)
{
	buff=buffer_+writePos_;
	return capacity_-writePos_;
}

void net::ezBuffer::addReadPos(size_t s)
{
	if(s>readableSize())
		s=readableSize();
	readPos_+=s;
	if(readPos_==writePos_)
		reset();
}

void net::ezBuffer::addWritePos(size_t s)
{
	if(s<=capacity_-writePos_)
		writePos_+=s;
	else
		writePos_=capacity_;
}

bool net::ezBuffer::appendBuffer(char* data,size_t s)
{
	if(s>writableSize())
		return false;
	size_t writable=capacity_-writePos_;
	if(s<=writableSize()&&s>writable)
	{
		memmove(buffer_,buffer_+readPos_,writePos_-readPos_);
		writePos_-=readPos_;
		readPos_=0;
	}
	memcpy(buffer_+writePos_,data,s);
	writePos_+=s;
	return true;
}
