#ifndef _NETPACK_H
#define _NETPACK_H
#include "../base/portable.h"

// from 0mq
extern "C"
{
  typedef void (msg_free_fn) (void *data, void *hint);
}

namespace net
{
  struct ezMsg
  {
    int8_t data_[64];
  };
  void ezMsgInit(ezMsg* msg);
  void ezMsgInitSize(ezMsg* msg,int size);
  void ezMsgInitData(ezMsg* msg,int8_t* data,int size,msg_free_fn* ffn,void* hint);
  void ezMsgInitDelimiter(ezMsg* msg);

  int  ezMsgSize(ezMsg* msg);
  int  ezMsgCapcity(ezMsg* msg);
  int8_t* ezMsgData(ezMsg* msg);
  bool ezMsgIsDelimiter(ezMsg* msg);

  void ezMsgMove(ezMsg* src,ezMsg* dst);
  void ezMsgCopy(ezMsg* src,ezMsg* dst);
  void ezMsgAddRef(ezMsg* msg,int ref);
  void ezMsgSubRef(ezMsg* msg,int ref);
  void ezMsgFree(ezMsg* msg);
}

#endif