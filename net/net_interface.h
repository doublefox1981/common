#ifndef _EVNET_INTERFACE_H
#define _EVNET_INTERFACE_H

namespace net
{
  struct Msg;
  class IMessagePusher;
  class IMessagePuller;
  class Buffer;
  class Connection;
  class EventLoop;

  class IDecoder
  {
  public:
    virtual ~IDecoder(){}
    virtual int decode(IMessagePusher* pusher,char* buf,size_t s)=0;
  };

  class IEncoder
  {
  public:
    virtual ~IEncoder(){}
    virtual bool encode(IMessagePuller* puller,Buffer* buff)=0;
  };

  class MsgDecoder:public IDecoder
  {
  public:
    explicit MsgDecoder(uint16_t maxsize):maxMsgSize_(maxsize){}
    virtual int decode(IMessagePusher* pusher,char* buf,size_t s);
  private:
    uint16_t maxMsgSize_;
  };

  class MsgEncoder:public IEncoder
  {
  public:
    virtual bool encode(IMessagePuller* puller,Buffer* buffer);
  };

  class GameObject
  {
  public:
    GameObject();
    virtual ~GameObject();
    void SetConnection(Connection* conn) {conn_=conn;}
    Connection* GetConnection(){return conn_;}
    void SendNetpack(Msg& msg);
    virtual void close();
  private:
    Connection* conn_;
  };

  class IConnnectionHander
  {
  public:
    virtual void on_open(Connection* conn)=0;
    virtual void on_close(Connection* conn)=0;
    virtual void on_data(Connection* conn,Msg* msg)=0;
  };

  class ServerHander:public IConnnectionHander
  {
  public:
    virtual void on_open(Connection* conn);
    virtual void on_close(Connection* conn);
    virtual void on_data(Connection* conn,Msg* msg);
  };

  class ClientHander:public IConnnectionHander
  {
  public:
    virtual void on_open(Connection* conn);
    virtual void on_close(Connection* conn);
    virtual void on_data(Connection* conn,Msg* msg);
  };

  void         net_initialize();
  EventLoop*   create_event_loop(IConnnectionHander* hander,IDecoder* decoder,IEncoder* encoder,int tnum);
  void         set_msg_buffer_size(EventLoop* loop,int size);
  void         destroy_event_loop(EventLoop* ev);
  int          serve_on_port(EventLoop* ev,int port);
  int          connect(EventLoop* ev,const char* ip,int port,int64_t userdata,int32_t reconnect);
  void         event_process(EventLoop* ev);
  void         close_connection(Connection* conn);
  void         msg_send(Connection* conn,Msg* msg);
  int64_t      conection_user_data(Connection* conn);
  GameObject*  get_game_object(Connection* conn);
  void         attach_game_object(Connection* conn,GameObject* obj);
  void         dettach_game_object(Connection* conn);
  const char*  get_ip_addr(Connection* conn);
}
#endif