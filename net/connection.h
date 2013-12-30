#ifndef _CONNECTION_H
#define _CONNECTION_H
#include <unordered_map>
#include <string>
#include <vector>
#include "../base/portable.h"
#include "../base/singleton.h"
#include "../base/eztimer.h"
#include "event.h"
#include "net_interface.h"

namespace net
{
  class ezClientFd;

  class ezConnection:public ezThreadEventHander
  {
  public:
    ezConnection(ezEventLoop* looper,ezClientFd* client,int tid,int64_t userdata);
    virtual ~ezConnection();
    void AttachGameObject(ezGameObject* obj);
    void DetachGameObject();
    ezGameObject* GetGameObject(){return gameObj_;}
    void ActiveClose();
    void SetIpAddr(const char* ip){ip_=ip;}
    const std::string& GetIpAddr() {return ip_;}
    int64_t GetUserdata();
    void SendMsg(ezMsg& msg);
    bool RecvMsg(ezMsg& msg);
    virtual void ProcessEvent(ezThreadEvent& ev);
  private:
    void CloseClient();
  private:
    ezClientFd* client_;
    std::string ip_;
    ezGameObject* gameObj_;
    int64_t userdata_;
  };
}
#endif