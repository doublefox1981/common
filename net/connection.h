#ifndef _CONNECTION_H
#define _CONNECTION_H
#include "../base/portable.h"
#include "../base/singleton.h"
#include "../base/eztimer.h"
#include "event.h"
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
  class ezClientFd;

  class ezIDecoder
  {
  public:
    virtual ~ezIDecoder(){}
    virtual int Decode(ezIMessagePusher* pusher,uint64_t uuid,char* buf,size_t s)=0;
  };

  class ezIConnnectionHander
  {
  public:
    virtual void OnOpen(ezConnection* conn)=0;
    virtual void OnClose(ezConnection* conn)=0;
    virtual void OnData(ezConnection* conn,ezMsg* msg)=0;
  };

  class ezServerHander:public ezIConnnectionHander
  {
  public:
    virtual void OnOpen(ezConnection* conn);
    virtual void OnClose(ezConnection* conn);
    virtual void OnData(ezConnection* conn,ezMsg* msg);
  };

  class ezClientHander:public ezIConnnectionHander
  {
  public:
    virtual void OnOpen(ezConnection* conn);
    virtual void OnClose(ezConnection* conn);
    virtual void OnData(ezConnection* conn,ezMsg* msg);
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
    void SetConnection(ezConnection* conn) {conn_=conn;}
    ezConnection* GetConnection(){return conn_;}
    void SendNetpack(ezMsg& msg);
    virtual void Close();
  private:
    ezConnection* conn_;
  };

  class ezConnectToGameObject:public ezGameObject
  {
  public:
    virtual void Close();
  };

  class ezConnection:public ezThreadEventHander
  {
  public:
    ezConnection(ezEventLoop* looper,ezClientFd* client,int tid);
    virtual ~ezConnection();
    void AttachGameObject(ezGameObject* obj);
    void DetachGameObject();
    ezGameObject* GetGameObject(){return gameObj_;}
    void OnRecvNetPack(ezMsg* pack);
    void ActiveClose();
    void CloseClient();
    void SetIpAddr(const char* ip){ip_=ip;}
    std::string GetIpAddr() {return ip_;}
    void SendMsg(ezMsg& msg);
    bool RecvMsg(ezMsg& msg);
    virtual void ProcessEvent(ezThreadEvent& ev);
  private:
    ezClientFd* client_;
    std::string ip_;
    ezGameObject* gameObj_;
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
}
#endif