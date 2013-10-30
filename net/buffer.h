#ifndef _BUFFER_H
#define _BUFFER_H

#include <vector>
#include <string>
#include <cstdint>
#include <assert.h>
namespace net
{
	class ezBuffer
	{
	public:
		static const size_t ezInitSize=16*1024;
		ezBuffer(size_t initSize=ezInitSize);
		~ezBuffer();
		void reset();
		size_t readableSize() {return writePos_-readPos_;}
		size_t writableSize() {return capacity_-writePos_+readPos_;}
		size_t getReadable(char*& buff);
		size_t getWritable(char*& buff);
		void addReadPos(size_t s);
		void addWritePos(size_t s);
		bool appendBuffer(char* data,size_t s);
		void appendBuffer(size_t s);
	private:
		char* buffer_;
		size_t capacity_;
		size_t readPos_;
		size_t writePos_;
	};
}

#endif