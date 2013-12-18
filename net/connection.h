#ifndef _CONNECTION_H
#define _CONNECTION_H
#include "../base/portable.h"
#include "../base/singleton.h"
#include "../base/eztimer.h"
#include <unordered_map>
#include <string>
#include <vector>

namespace net
{
class ezEventLoop;
struct ezMsg;
class ezConnection;
class ezIMessagePusher;
class ezIMessagePuller;
class ezIoThread;

class ezIDecoder
{
public:
  virtual ~ezIDecoder(){}
  virtual int decode(ezIMessagePusher* pusher,uint64_t uuid,char* buf,size_t s)=0;
};

class ezINetPackHander
{
public:
  virtual void process(ezConnection* conn,ezMsg* pack)=0;
};

class ezIHander
{
public:
	virtual void onOpen(ezIoThread* io,int fd,uint64_t uuid,int bindtid)=0;
	virtual void onClose(ezIoThread* io,int fd,uint64_t uuid)=0;
	virtual void onError(ezIoThread* io,int fd,uint64_t uuid)=0;
	virtual void onData(ezIoThread* io,int fd,uint64_t uuid,ezMsg* msg)=0;
  virtual ezIDecoder* getDecoder()=0;
};

class ezServerHander:public ezIHander
{
public:
	explicit ezServerHander(ezIDecoder* de);
	virtual void onOpen(ezIoThread* io,int fd,uint64_t uuid,int bindtid);
	virtual void onClose(ezIoThread* io,int fd,uint64_t uuid);
	virtual void onError(ezIoThread* io,int fd,uint64_t uuid);
	virtual void onData(ezIoThread* io,int fd,uint64_t uuid,ezMsg* msg);
  virtual ezIDecoder* getDecoder(){return decoder_;}
private:
  ezIDecoder* decoder_;
};

class ezClientHander:public ezIHander
{
public:
	explicit ezClientHander(ezIDecoder* de);
	virtual void onOpen(ezIoThread* io,int fd,uint64_t uuid,int bindtid);
	virtual void onClose(ezIoThread* io,int fd,uint64_t uuid);
	virtual void onError(ezIoThread* io,int fd,uint64_t uuid);
	virtual void onData(ezIoThread* io,int fd,uint64_t uuid,ezMsg* msg);
  virtual ezIDecoder* getDecoder(){return decoder_;}
private:
  ezIDecoder* decoder_;
};

class ezMsgDecoder:public ezIDecoder
{
public:
  explicit ezMsgDecoder(uint16_t maxsize):maxMsgSize_(maxsize){}
  virtual ~ezMsgDecoder(){}
  virtual int decode(ezIMessagePusher* pusher,uint64_t uuid,char* buf,size_t s);
private:
  uint16_t maxMsgSize_;
};

class ezGameObject
{
public:
	ezGameObject();
	virtual ~ezGameObject();
	void setConnection(ezConnection* conn) {conn_=conn;}
	ezConnection* getConnection(){return conn_;}
  void sendNetpack(ezMsg* pack);
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
	ezConnection(ezEventLoop* looper,int fd,uint64_t uuid,int tid);
	virtual ~ezConnection();
	void attachGameObject(ezGameObject* obj);
	void detachGameObject();
	ezGameObject* getGameObject(){return gameObj_;}
	void onRecvNetPack(ezMsg* pack);
	void Close();
	ezEventLoop* getEventLooper() {return evLooper_;}
	uint64_t uuid() {return uuid_;}
	void setIpAddr(const char* ip){ip_=ip;}
	std::string getIpAddr() {return ip_;}
	void sendNetPack(ezMsg* pack);
	ezINetPackHander* getHander(){return netpackHander_;}
	void setHander(ezINetPackHander* hander);
private:
	uint64_t uuid_;
	int fd_;
  int tid_;
	std::string ip_;
	ezGameObject* gameObj_;
	ezEventLoop* evLooper_;
	ezINetPackHander* netpackHander_;
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
	ezConnectionMgr():looper_(nullptr),defaultHander_(nullptr){}
	virtual ~ezConnectionMgr(){if(defaultHander_) delete defaultHander_;}
	void setDefaultHander(ezINetPackHander* hander){defaultHander_=hander;}
	ezConnection* addConnection(ezEventLoop* looper,int fd,uint64_t uuid,int tid);
	void delConnection(uint64_t uuid);
	ezConnection* findConnection(uint64_t uuid);
	ezConnectToInfo* findConnectToInfo(uint64_t uuid);
	void delConnectToInfo(uint64_t uuid);
	uint64_t connectTo(ezEventLoop* looper,const char* ip,int port);
	void reconnectAll();
	friend class ezEventLoop;
private:
	std::unordered_map<uint64_t,ezConnection*> mapConns_;
	std::vector<ezConnectToInfo> vecConnectTo_;
	ezEventLoop* looper_;
	ezINetPackHander* defaultHander_;
};

class ezReconnectTimerTask:public base::ezTimerTask
{
public:
	explicit ezReconnectTimerTask(ezConnectionMgr* mgr):mgr_(mgr){}
	virtual void run();
private:
	ezConnectionMgr* mgr_;
};
}
#endif