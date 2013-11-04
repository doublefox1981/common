#include "portable.h"
#include "socket.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
namespace net
{
void ezSocketError(const char* err)
{
#ifndef __linux__
	errno=GetLastError();
#endif
	printf("%s:errno=%d\n",err,errno);
}

#ifndef __linux__
int inet_pton(int af, const char *cp, struct in_addr *addr)
{
	if(af != AF_INET) {
		WSASetLastError(WSAEPFNOSUPPORT);
		return -1;
	}
	return inet_aton(cp, addr);
}

int inet_aton( const char *cp, struct in_addr *addr)
{
	int val;
	int base, n;
	char c;
	unsigned int parts[4];
	unsigned int *pp = parts;

	c = *cp;
	for (;;) {
		if (!isdigit(c))
			return (0);
		val = 0; base = 10;
		if (c == '0') {
			c = *++cp;
			if (c == 'x' || c == 'X')
				base = 16, c = *++cp;
			else
				base = 8;
		}
		for (;;) {
			if (isascii(c) && isdigit(c)) {
				val = (val * base) + (c - '0');
				c = *++cp;
			} else if (base == 16 && isascii(c) && isxdigit(c)) {
				val = (val << 4) |
					(c + 10 - (islower(c) ? 'a' : 'A'));
				c = *++cp;
			} else
				break;
		}
		if (c == '.') {

			if (pp >= parts + 3)
				return (0);
			*pp++ = val;
			c = *++cp;
		} else
			break;
	}

	if (c != '\0' && (!isascii(c) || !isspace(c)))
		return (0);

	n = pp - parts + 1;
	switch (n) {

	case 0:
		return (0);        /* initial nondigit */

	case 1:                /* a -- 32 bits */
		break;

	case 2:                /* a.b -- 8.24 bits */
		if (val > 0xffffff)
			return (0);
		val |= parts[0] << 24;
		break;

	case 3:                /* a.b.c -- 8.8.16 bits */
		if (val > 0xffff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16);
		break;

	case 4:                /* a.b.c.d -- 8.8.8.8 bits */
		if (val > 0xff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
		break;
	}
	if (addr)
		addr->s_addr = htonl(val);
	return (1);
}

static const char *
	inet_ntop_v4 (const void *src, char *dst, size_t size)
{
	const char digits[] = "0123456789";
	int i;
	struct in_addr *addr = (struct in_addr *)src;
	u_long a = ntohl(addr->s_addr);
	const char *orig_dst = dst;

	if (size < INET_ADDRSTRLEN) {
		errno = ENOSPC;
		return nullptr;
	}
	for (i = 0; i < 4; ++i) {
		int n = (a >> (24 - i * 8)) & 0xFF;
		int non_zerop = 0;

		if (non_zerop || n / 100 > 0) {
			*dst++ = digits[n / 100];
			n %= 100;
			non_zerop = 1;
		}
		if (non_zerop || n / 10 > 0) {
			*dst++ = digits[n / 10];
			n %= 10;
			non_zerop = 1;
		}
		*dst++ = digits[n];
		if (i != 3)
			*dst++ = '.';
	}
	*dst++ = '\0';
	return orig_dst;
}

/*
* Convert IPv6 binary address into presentation (printable) format.
*/
static const char *inet_ntop_v6 (const u_char *src, char *dst, size_t size)
{
	return nullptr;
}

const char* inet_ntop(int af, const void *src, char *dst, size_t size)
{
	switch (af) {
	case AF_INET :
		return inet_ntop_v4 (src, dst, size);
	case AF_INET6:
		return inet_ntop_v6 ((const u_char*)src, dst, size);
	default :
		errno = EAFNOSUPPORT;
		return nullptr;
	}
}
#endif

typedef struct sockaddr SA;
const SA* sockaddr_cast(const struct sockaddr_in* addr)
{
	return static_cast<const SA*>((const void*)(addr));
}

SA* sockaddr_cast(struct sockaddr_in* addr)
{
	return static_cast<SA*>((void*)(addr));
}

void NonBlock(SOCKET sock)
{
#ifndef _WIN32
	int opts;
	opts = fcntl(sock, F_GETFL);
	opts = opts | O_NONBLOCK | O_ASYNC;
	fcntl(sock, F_SETFL, opts);
#else
	u_long iMode = 1;
	if(ioctlsocket(sock, FIONBIO, &iMode) == SOCKET_ERROR) 
		ezSocketError("ioctlsocket");
#endif
}

SOCKET CreateNonBlockSocket()
{
	SOCKET s=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(s==INVALID_SOCKET)
		return INVALID_SOCKET;
	char on=1;
	if (setsockopt(s,SOL_SOCKET,SO_REUSEADDR, &on, sizeof(on))==-1) 
		ezSocketError("setsockopt SO_REUSEADDR");
	NonBlock(s);
	return s;
}

SOCKET CreateTcpServer(int port,char* bindaddr)
{
	struct sockaddr_in so;
	SOCKET s=CreateNonBlockSocket();
	if(s==INVALID_SOCKET)
		return -1;
	memset(&so,0,sizeof(so));
	so.sin_family=AF_INET;
	so.sin_port = htons(port);
	so.sin_addr.s_addr = htonl(INADDR_ANY);
	if(Bind(s,so)==SOCKET_ERROR)
	{
		ezSocketError("bind fail");
		return SOCKET_ERROR;
	}
	if(Listen(s)==SOCKET_ERROR)
	{
		ezSocketError("listen error");
		return SOCKET_ERROR;
	}
	return s;
}

int Bind(SOCKET sockfd, const struct sockaddr_in& addr)
{
	return ::bind(sockfd, sockaddr_cast(&addr), static_cast<socklen_t>(sizeof addr));
}

int Listen(SOCKET sockfd)
{
	return ::listen(sockfd, SOMAXCONN);
}

SOCKET Accept(SOCKET sockfd, struct sockaddr_in* addr)
{
	socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
	SOCKET connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
	if(connfd==INVALID_SOCKET)
	{
		ezSocketError("accept fail");
		return INVALID_SOCKET;
	}
	NonBlock(connfd);
	return connfd;
}

int Connect(SOCKET sockfd, const struct sockaddr_in& addr)
{
	return ::connect(sockfd, sockaddr_cast(&addr), static_cast<socklen_t>(sizeof addr));
}

int Read(SOCKET sockfd, void *buf, size_t count)
{
	int retval=::recv(sockfd,static_cast<char*>(buf),count,0);
	if(retval<0)
	{
#ifndef __linux__
		errno=WSAGetLastError();
		if((errno==ENOENT)||(errno==WSAEWOULDBLOCK))
			errno=EAGAIN;
#endif
	}
	return retval;
}

int Write(SOCKET sockfd, const void *buf, size_t count)
{
	int retval=::send(sockfd,static_cast<const char*>(buf),count,0);
#ifndef __linux__
	if(retval<0)
		errno=WSAGetLastError();
#endif
	return retval;
}

void CloseSocket(SOCKET sockfd)
{
#ifndef __linux__
	::closesocket(sockfd);
#else
	::close(sockfd);
#endif
}

void ShutdownWrite(SOCKET sockfd)
{
	::shutdown(sockfd,0);
}

void ToIpPort(char* buf,size_t size,const struct sockaddr_in& addr)
{
	char host[INET_ADDRSTRLEN] = "INVALID";
	ToIp(host, sizeof host, addr);
	uint16_t port=ntohs(addr.sin_port);
	snprintf(buf,size,"%s:%u",host,port);
}

void ToIp(char* buf,size_t size,const struct sockaddr_in& addr)
{
	inet_ntop(AF_INET,(void*)&addr.sin_addr, buf, static_cast<socklen_t>(size));
}

void FromIpPort(const char* ip,uint16_t port,struct sockaddr_in* addr)
{
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	inet_pton(AF_INET, ip, &addr->sin_addr);
}

int GetSocketError(SOCKET sockfd)
{
	int optval;
	socklen_t optlen=static_cast<socklen_t>(sizeof optval);
	if(::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)(&optval), &optlen)<0)
	{
		return errno;
	}
	else
	{
		return optval;
	}
}

struct sockaddr_in GetLocalAddr(SOCKET sockfd)
{
	struct sockaddr_in localaddr;
	memset(&localaddr,sizeof localaddr,0);
	socklen_t addrlen=static_cast<socklen_t>(sizeof localaddr);
	::getsockname(sockfd,sockaddr_cast(&localaddr),&addrlen);
	return localaddr;
}

struct sockaddr_in GetPeerAddr(SOCKET sockfd)
{
	struct sockaddr_in peeraddr;
	memset(&peeraddr, sizeof peeraddr,0);
	socklen_t addrlen=static_cast<socklen_t>(sizeof(peeraddr));
	::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen);
	return peeraddr;
}

bool IsSelfConnect(SOCKET sockfd)
{
	struct sockaddr_in localaddr = GetLocalAddr(sockfd);
	struct sockaddr_in peeraddr = GetPeerAddr(sockfd);
	return localaddr.sin_port == peeraddr.sin_port
		&& localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr;
}

int ConnectNoBlock(const char* ip,int port)
{
	SOCKET s=CreateNonBlockSocket();
	if(s==INVALID_SOCKET)
		return INVALID_SOCKET;
	struct sockaddr_in sa;
	unsigned long inAddress;

	sa.sin_family = AF_INET;
	sa.sin_port = htons((u_short)port);
	inAddress = inet_addr(ip);
	sa.sin_addr.s_addr = inAddress;

	if(connect(s,(struct sockaddr*)&sa,sizeof(sa))==-1) 
	{
#ifndef __linux__
		DWORD e=WSAGetLastError();
#endif
		if (errno==EINPROGRESS)
			return s;
		CloseSocket(s);
		return INVALID_SOCKET;
	}
	return s;
}

int ConnectTo(const char* ip,int port)
{
	SOCKET s=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(s==INVALID_SOCKET)
		return INVALID_SOCKET;
	char on=1;
	if (setsockopt(s,SOL_SOCKET,SO_REUSEADDR, &on, sizeof(on))==-1) 
	{
		ezSocketError("setsockopt SO_REUSEADDR");
		CloseSocket(s);
		return INVALID_SOCKET;
	}
	struct sockaddr_in sa;
	unsigned long inAddress;

	sa.sin_family = AF_INET;
	sa.sin_port = htons((u_short)port);
	inAddress = inet_addr(ip);
	sa.sin_addr.s_addr = inAddress;
	if(Connect(s,sa)==0)
		return s;
	else
	{
		CloseSocket(s);
		return INVALID_SOCKET;
	}
}

bool InitNetwork(int version) 
{
#ifndef __linux__
	WSADATA wsa_data;
	int minor_version = 0;
	if(version == 1) minor_version = 1;
	if(WSAStartup(MAKEWORD(version, minor_version), &wsa_data) != 0)
		return false;
	else
		return true;
#else
	return true;
#endif
}
}