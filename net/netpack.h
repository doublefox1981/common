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
  struct Msg
  {
    int8_t data_[64];
  };
  void msg_init(Msg* msg);
  void msg_init_size(Msg* msg,int size);
  void msg_init_data(Msg* msg,int8_t* data,int size,msg_free_fn* ffn,void* hint);
  void msg_init_delimiter(Msg* msg);

  int  msg_size(Msg* msg);
  int  msg_capcity(Msg* msg);
  bool msg_is_delimiter(Msg* msg);
  int8_t* msg_data(Msg* msg);

  void msg_move(Msg* src,Msg* dst);
  void msg_copy(Msg* src,Msg* dst);
  void msg_add_ref(Msg* msg,int ref);
  void msg_sub_ref(Msg* msg,int ref);
  void msg_free(Msg* msg);
}

#endif