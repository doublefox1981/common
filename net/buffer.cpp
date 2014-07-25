#include <string.h>
#include <cstdint>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include "buffer.h"
#include "socket.h"

/**
*** 关于应用层的接收缓冲区，有两种方式可以考虑。1：可以使用固定大小缓冲区，如遇到缓冲区满，待处理完后再接收，
***这个时候最好使用epoll的水平触发方式，如果使用边缘触发需要一次性接收完所有数据，1这种方式好处是：简单
***2:使用固定大小块的队列，比如每次接收8K，存放于队列，这种方式是高效，解析时可以减少内存拷贝，但是结构更复杂，
***权衡后觉得这一步骤不会成为瓶颈，故采用方式1
**/
net::Buffer::Buffer(size_t initSize/*=ezInitSize*/)
{
	orig_buffer_=new char[initSize];
	buffer_=orig_buffer_;
	totallen_=initSize;
	misalign_=0;
	off_=0;
}

net::Buffer::~Buffer()
{
	if(orig_buffer_)
		delete [] orig_buffer_;
}

void net::Buffer::drain(size_t len)
{
	size_t oldoff=off_;
	if(len>=off_)
	{
		off_=0;
		buffer_=orig_buffer_;
		misalign_=0;
		return;
	}
	buffer_+=len;
	misalign_+=len;

	off_-=len;
}

int net::Buffer::remove(void* data,size_t datlen)
{
	size_t nread=datlen;
	if (nread>=off_)
		nread=off_;
	memcpy(data,buffer_,nread);
	drain(nread);
	return (nread);
}

void net::Buffer::align()
{
	memmove(orig_buffer_,buffer_,off_);
	buffer_=orig_buffer_;
	misalign_=0;
}

int net::Buffer::maxadd()
{
	return (totallen_-off_);
}

int net::Buffer::fastadd()
{
	return (totallen_-off_-misalign_);
}

int net::Buffer::expand(size_t datlen)
{
	size_t space=totallen_-misalign_-off_;
	if(space>=datlen)
		return 0;
	else if(totallen_-off_>datlen)
	{
		align();
		return 0;
	}
	else
		return -1;
}

int net::Buffer::add(const void* data,size_t datlen)
{
	if(expand(datlen)<0)
		return -1;
	memcpy(buffer_+off_,data,datlen);
	off_+=datlen;
	return 0;
}

int net::Buffer::readfd(int fd)
{
	int canread=fastadd();
	if(canread<=0)
	{
		align();
		canread=fastadd();
		if(canread<=0)
			return 0x7fffffff;
	}
	char* p=buffer_+off_;
	int retn=Read(fd,p,canread);
	if(retn>0)
		off_+=retn;
	return retn;
}

int net::Buffer::writefd(int fd)
{
	while(off_>0)
	{
		int retn=Write(fd,buffer_,off_);
		if(retn<0)
		{
			if(errno==EWOULDBLOCK||errno==EAGAIN)
				return 1;
			else
				return -1;
		}
		drain(retn);
	}
	return 0;
}

int net::Buffer::readable(char*& pbuf)
{
	pbuf=buffer_;
	return off_;
}
