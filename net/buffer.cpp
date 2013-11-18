#include "buffer.h"
#include "socket.h"
#include <string.h>

net::ezBuffer::ezBuffer(size_t initSize/*=ezInitSize*/)
{
	orig_buffer_=(char*)malloc(initSize);
	buffer_=orig_buffer_;
	totallen_=initSize;
	misalign_=0;
	off_=0;
}

net::ezBuffer::~ezBuffer()
{
	if(orig_buffer_)
		free(orig_buffer_);
}

void net::ezBuffer::drain(size_t len)
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

int net::ezBuffer::remove(void* data,size_t datlen)
{
	size_t nread=datlen;
	if (nread>=off_)
		nread=off_;
	memcpy(data,buffer_,nread);
	drain(nread);
	return (nread);
}

void net::ezBuffer::align()
{
	memmove(orig_buffer_,buffer_,off_);
	buffer_=orig_buffer_;
	misalign_=0;
}

int net::ezBuffer::canexpand()
{
	return (totallen_-off_);
}

int net::ezBuffer::expand(size_t datlen)
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

int net::ezBuffer::add(const void* data,size_t datlen)
{
	if(expand(datlen)<0)
		return -1;
	memcpy(buffer_+off_,data,datlen);
	off_+=datlen;
	return 0;
}

int net::ezBuffer::readfd(int fd)
{
	int canread=canexpand();
	if(canread<=0)
		return -1;
	char* p;
	int n = canread;

#if defined(FIONREAD)
#ifdef __linux__
	if (ioctl(fd,FIONREAD,&n)==-1||n==0){
#else
	u_long lng = n;
	if (ioctlsocket(fd,FIONREAD,&lng)==-1||(n=lng)==0){
#endif
		n = canread;
	}
#endif
	if(n>canread)
		n=canread;
	if (expand(n)<0)
		return -1;

	p=buffer_+off_;
	int retn=Read(fd,p,n);
	
	if(retn>0)
		off_+=n;
	return n;
}

int net::ezBuffer::writefd(int fd)
{
	while(off_>0)
	{
		int retn=Write(fd,buffer_,off_);
		if(retn<0)
		{
			if(errno==EWOULDBLOCK||errno==EAGAIN)
				return 0;
			else
				return -1;
		}
		drain(retn);
	}
	return 0;
}

int net::ezBuffer::readable(char*& pbuf)
{
	pbuf=buffer_;
	return off_;
}
