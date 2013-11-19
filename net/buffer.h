#ifndef _BUFFER_H
#define _BUFFER_H

#include <vector>
#include <string>
#include <cstdint>
#include <assert.h>
/**
*** 关于应用层的接收缓冲区，有两种方式可以考虑。1：可以使用固定大小缓冲区，如遇到缓冲区满，待处理完后再接收，
***这个时候最好使用epoll的水平触发方式，如果使用边缘触发需要一次性接收完所有数据，1这种方式好处是：简单
***2:使用固定大小块的队列，比如每次接收8K，存放于队列，这种方式是高效，解析时可以减少内存拷贝，但是结构更复杂，
***权衡后觉得这一步骤不会成为瓶颈，故采用方式1
**/
namespace net
{
	class ezBuffer
	{
	public:
		static const size_t ezInitSize=128*1024;
		explicit ezBuffer(size_t initSize=ezInitSize);
		~ezBuffer();
		void drain(size_t len);
		int remove(void* data,size_t datlen);
		void align();
		int maxadd();
		int fastadd();
		int expand(size_t datlen);
		int add(const void* data,size_t datlen);
		int readfd(int fd);
		int writefd(int fd);
		int readable(char*& pbuf);
		size_t off() {return off_;}
	private:
		char *buffer_;
		char *orig_buffer_;

		size_t misalign_;
		size_t totallen_;
		size_t off_;
	};
}

#endif