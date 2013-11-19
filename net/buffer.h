#ifndef _BUFFER_H
#define _BUFFER_H

#include <vector>
#include <string>
#include <cstdint>
#include <assert.h>
/**
*** ����Ӧ�ò�Ľ��ջ������������ַ�ʽ���Կ��ǡ�1������ʹ�ù̶���С����������������������������������ٽ��գ�
***���ʱ�����ʹ��epoll��ˮƽ������ʽ�����ʹ�ñ�Ե������Ҫһ���Խ������������ݣ�1���ַ�ʽ�ô��ǣ���
***2:ʹ�ù̶���С��Ķ��У�����ÿ�ν���8K������ڶ��У����ַ�ʽ�Ǹ�Ч������ʱ���Լ����ڴ濽�������ǽṹ�����ӣ�
***Ȩ��������һ���費���Ϊƿ�����ʲ��÷�ʽ1
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