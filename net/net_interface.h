#ifndef _EVNET_INTERFACE_H
#define _EVNET_INTERFACE_H

namespace net
{
  struct ezMsg;
  class ezIMessagePusher;
  class ezIMessagePuller;
  class ezBuffer;
  class ezConnection;
  class ezEventLoop;

  class ezIDecoder
  {
  public:
    virtual ~ezIDecoder(){}
    virtual int Decode(ezIMessagePusher* pusher,char* buf,size_t s)=0;
  };

  class ezIEncoder
  {
  public:
    virtual ~ezIEncoder(){}
    virtual bool Encode(ezIMessagePuller* puller,ezBuffer* buff)=0;
  };

  class ezMsgDecoder:public ezIDecoder
  {
  public:
    explicit ezMsgDecoder(uint16_t maxsize):maxMsgSize_(maxsize){}
    virtual int Decode(ezIMessagePusher* pusher,char* buf,size_t s);
  private:
    uint16_t maxMsgSize_;
  };

  class ezMsgEncoder:public ezIEncoder
  {
  public:
    virtual bool Encode(ezIMessagePuller* puller,ezBuffer* buffer);
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

  void           EzNetInitialize();
  ezEventLoop*   CreateEventLoop(ezIConnnectionHander* hander,ezIDecoder* decoder,ezIEncoder* encoder,int tnum);
  void           SetMsgBufferSize(ezEventLoop* loop,int size);
  void           DestroyEventLoop(ezEventLoop* ev);
  int            ServeOnPort(ezEventLoop* ev,int port);
  int            Connect(ezEventLoop* ev,const char* ip,int port,int64_t userdata,int32_t reconnect);
  void           EventProcess(ezEventLoop* ev);
  void           CloseConnection(ezConnection* conn);
  void           MsgSend(ezConnection* conn,ezMsg* msg);
  int64_t        ConnectionUserdata(ezConnection* conn);
  ezGameObject*  GetGameObject(ezConnection* conn);
  void           AttachGameObject(ezConnection* conn,ezGameObject* obj);
  void           DetachGameObject(ezConnection* conn);
  const char*    GetIpAddr(ezConnection* conn);
}
#endif