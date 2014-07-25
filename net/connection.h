#ifndef _CONNECTION_H
#define _CONNECTION_H
#include <unordered_map>
#include <string>
#include <vector>
#include "../base/portable.h"
#include "../base/singleton.h"

#include "event.h"
#include "net_interface.h"

namespace net
{
  class ezClientFd;

  class Connection:public ezThreadEventHander
  {
  public:
    Connection(EventLoop* looper,ezClientFd* client,int tid,int64_t userdata);
    virtual ~Connection();
    void attach_game_object(GameObject* obj);
    void dettach_game_object();
    GameObject* get_game_object(){return gameObj_;}
    void ActiveClose();
    void SetIpAddr(const char* ip){ip_=ip;}
    const std::string& get_ip_addr() {return ip_;}
    int64_t GetUserdata();
    void SendMsg(Msg& msg);
    bool RecvMsg(Msg& msg);
    virtual void ProcessEvent(ezThreadEvent& ev);
  private:
    void CloseClient();
  private:
    ezClientFd* client_;
    std::string ip_;
    GameObject* gameObj_;
    int64_t userdata_;
  };
}
#endif