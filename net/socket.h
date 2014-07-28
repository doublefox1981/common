#ifndef _SOCKET_H
#define _SOCKET_H
#include "../base/portable.h"

namespace net
{
#ifdef __linux__
	typedef int SOCKET;
#ifndef INVALID_SOCKET
	#define INVALID_SOCKET (SOCKET)(~0)
#endif
#ifndef SOCKET_ERROR
	#define SOCKET_ERROR (SOCKET)(~0)
#endif
#else
	int inet_aton(register const char *cp, struct in_addr *addr);
	int inet_pton(int af, register const char *cp, struct in_addr *addr);
  int wsa_error_to_errno (int errcode);
#endif
	bool InitNetwork(int version=2);
	void NonBlock(SOCKET s);
	SOCKET CreateNonBlockSocket();
	SOCKET CreateTcpServer(int port,char* bindaddr);
	int Connect(SOCKET sockfd, const sockaddr_in& addr);
	int Bind(SOCKET sockfd, const sockaddr_in& addr);
	int Listen(SOCKET sockfd);
	SOCKET Accept(SOCKET sockfd, sockaddr_in* addr);
	int Read(SOCKET sockfd, void *buf, size_t count);
	int Write(SOCKET sockfd, const void *buf, size_t count);
	void CloseSocket(SOCKET s);
	void ShutdownWrite(SOCKET s);

	void ToIpPort(char* buf, size_t size,const sockaddr_in& addr);
	void ToIp(char* buf, size_t size,const sockaddr_in& addr);
	void FromIpPort(const char* ip, uint16_t port,sockaddr_in* addr);

	int GetSocketError(int sockfd);

	sockaddr_in GetLocalAddr(SOCKET sockfd);
	sockaddr_in GetPeerAddr(SOCKET sockfd);
	bool IsSelfConnect(SOCKET sockfd);
	int ConnectNoBlock(const char* ip,int port);
	int connect_to(const char* ip,int port,int& s);
}
#endif