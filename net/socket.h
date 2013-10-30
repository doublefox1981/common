#ifndef _SOCKET_H
#define _SOCKET_H
#include "portable.h"

namespace net
{
#ifdef __linux__
	typedef int SOCKET;
	#define INVALID_SOCKET (SOCKET)(~0)
#else
	int inet_aton(register const char *cp, struct in_addr *addr);
	int inet_pton(int af, register const char *cp, struct in_addr *addr);
#endif
	void NonBlock(SOCKET s);
	SOCKET CreateNonBlockSocket();
	SOCKET CreateTcpServer(int port,char* bindaddr);
	int Connect(SOCKET sockfd, const struct sockaddr_in& addr);
	int Bind(SOCKET sockfd, const struct sockaddr_in& addr);
	int Listen(SOCKET sockfd);
	SOCKET Accept(SOCKET sockfd, struct sockaddr_in* addr);
	int Read(SOCKET sockfd, void *buf, size_t count);
	int Write(SOCKET sockfd, const void *buf, size_t count);
	void Close(int SOCKET);
	void ShutdownWrite(int SOCKET);

	void ToIpPort(char* buf, size_t size,const struct sockaddr_in& addr);
	void ToIp(char* buf, size_t size,const struct sockaddr_in& addr);
	void FromIpPort(const char* ip, uint16_t port,struct sockaddr_in* addr);

	int GetSocketError(int sockfd);

	struct sockaddr_in GetLocalAddr(SOCKET sockfd);
	struct sockaddr_in GetPeerAddr(SOCKET sockfd);
	bool IsSelfConnect(SOCKET sockfd);
}
#endif