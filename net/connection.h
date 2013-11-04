#ifndef _CONNECTION_H
#define _CONNECTION_H
#include "portable.h"
#include "../base/singleton.h"
#include <hash_map>

namespace net
{
class ezEventLoop;
struct ezNetPack;
class ezHander
{
public:
	virtual void onOpen(ezEventLoop* looper,int fd,uint64_t uuid)=0;
	virtual void onClose(ezEventLoop* looper,int fd,uint64_t uuid)=0;
	virtual void onError(ezEventLoop* looper,int fd,uint64_t uuid)=0;
	virtual void onData(ezEventLoop* looper,int fd,uint64_t uuid,ezNetPack* msg)=0;
	virtual int decode(ezEventLoop* looper,int fd,uint64_t uuid,char* buf,size_t s)=0;
};

// 服务器模型
class ezServerHander:public ezHander
{
public:
	explicit ezServerHander(uint16_t maxMsgSize);
	virtual void onOpen(ezEventLoop* looper,int fd,uint64_t uuid);
	virtual void onClose(ezEventLoop* looper,int fd,uint64_t uuid);
	virtual void onError(ezEventLoop* looper,int fd,uint64_t uuid);
	virtual void onData(ezEventLoop* looper,int fd,uint64_t uuid,ezNetPack* msg);
	virtual int decode(ezEventLoop* looper,int fd,uint64_t uuid,char* buf,size_t s);
private:
	uint16_t maxMsgSize_;
};

// 客户端模型
class ezClientHander:public ezHander
{
public:
	explicit ezClientHander(uint16_t maxMsgSize);
	virtual void onOpen(ezEventLoop* looper,int fd,uint64_t uuid);
	virtual void onClose(ezEventLoop* looper,int fd,uint64_t uuid);
	virtual void onError(ezEventLoop* looper,int fd,uint64_t uuid);
	virtual void onData(ezEventLoop* looper,int fd,uint64_t uuid,ezNetPack* msg);
	virtual int decode(ezEventLoop* looper,int fd,uint64_t uuid,char* buf,size_t s);
private:
	uint16_t maxMsgSize_;
};

class ezConnection;
class ezGameObject
{
public:
	ezGameObject();
	virtual ~ezGameObject();
	virtual void onRecvNetPack(ezNetPack* pack){}
	void setConnection(ezConnection* conn) {conn_=conn;}
	ezConnection* getConnection(){return conn_;}
	virtual void Close();
private:
	ezConnection* conn_;
};
class ezConnectToGameObject:public ezGameObject
{
public:
	virtual void Close();
};


class ezConnection
{
public:
	explicit ezConnection(ezEventLoop* looper);
	ezConnection(ezEventLoop* looper,int fd,uint64_t uuid);
	virtual ~ezConnection();
	void attachGameObject(ezGameObject* obj);
	void detachGameObject();
	ezGameObject* getGameObject(){return gameObj_;}
	void onRecvNetPack(ezNetPack* pack);
	void Close();
	ezEventLoop* getEventLooper() {return evLooper_;}
	uint64_t uuid() {return uuid_;}
	void setIpAddr(const char* ip){ip_=ip;}
	std::string getIpAddr() {return ip_;}
	void sendNetPack(ezNetPack* pack);
private:
	uint64_t uuid_;
	int fd_;
	std::string ip_;
	ezGameObject* gameObj_;
	ezEventLoop* evLooper_;
};

struct ezConnectToInfo
{
	ezConnectToInfo():uuid_(0),port_(0),connectOK_(false){}
	ezConnectToInfo(uint64_t uuid,const char* ip,int port):uuid_(uuid),ip_(ip),port_(port),connectOK_(false){}
	uint64_t uuid_;
	std::string ip_;
	int port_;
	bool connectOK_;
};

class ezConnectionMgr
{
public:
	ezConnection* addConnection(ezEventLoop* looper,int fd,uint64_t uuid);
	void delConnection(uint64_t uuid);
	ezConnection* findConnection(uint64_t uuid);
	ezConnectToInfo* findConnectToInfo(uint64_t uuid);
	void delConnectToInfo(uint64_t uuid);
	uint64_t connectTo(ezEventLoop* looper,const char* ip,int port);
	void reconnectAll(ezEventLoop* looper);
private:
	std::hash_map<uint64_t,ezConnection*> mapConns_;
	std::vector<ezConnectToInfo> vecConnectTo_;
};
}
#endif