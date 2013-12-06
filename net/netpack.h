#ifndef _NETPACK_H
#define _NETPACK_H
#include "portable.h"
#include "../base/thread.h"

// from 0mq
extern "C"
{
  typedef void (msg_free_fn) (void *data, void *hint);
}
namespace net
{
  enum
  {
    more = 1,
    identity = 64,
    shared = 128
  };
  enum {max_vsm_size = 60};
  enum type_t
  {
    type_min = 101,
    type_vsm = 101,
    type_lmsg = 102,
    type_delimiter = 103,
    type_max = 103
  };
  struct ezMsg
  {
    struct ezBigMsg
    {
      int8_t*          data_;
      uint16_t         capcity_;
      msg_free_fn*     ffn_;
      void*            hint_;
      base::AtomicNumber refcnt_;
    };
    uint16_t size_;
    int8_t   type_;
    int8_t   flags_;
    union
    {
      struct
      {
        int8_t data_[max_vsm_size];
      }stack_;
      struct
      {
        ezBigMsg* ptr_;
      }heap_;
    };
  };

  void ezMsgInit(ezMsg* msg);
  void ezMsgInitSize(ezMsg* msg,int size);
  void ezMsgInitData(ezMsg* msg,int8_t* data,int size,msg_free_fn* ffn,void* hint);
  void ezMsgInitDelimiter(ezMsg* msg);

  int  ezMsgSize(ezMsg* msg);
  int  ezMsgCapcity(ezMsg* msg);
  int8_t* ezMsgData(ezMsg* msg);

  void ezMsgCopy(ezMsg* src,ezMsg* dst);
  void ezMsgAddRef(ezMsg* msg,int ref);
  void ezMsgSubRef(ezMsg* msg,int ref);
  void ezMsgFree(ezMsg* msg);
}

#endif