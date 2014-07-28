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
  class ClientFd;

  class Connection:public ThreadEventHander
  {
  public:
    Connection(EventLoop* looper,ClientFd* client,int tid,int64_t userdata);
    virtual ~Connection();
    void attach_game_object(GameObject* obj);
    void dettach_game_object();
    GameObject* get_game_object(){return gameObj_;}
    void active_close();
    void set_ip_addr(const char* ip){ip_=ip;}
    const std::string& get_ip_addr() {return ip_;}
    int64_t get_user_data();
    void send_msg(Msg& msg);
    bool recv_msg(Msg& msg);
    virtual void process_event(ThreadEvent& ev);
  private:
    void close_client();
  private:
    ClientFd* client_;
    std::string ip_;
    GameObject* gameObj_;
    int64_t userdata_;
  };
}
#endif