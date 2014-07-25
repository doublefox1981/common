#include <cstring>
#include <limits>
#include <cassert>
#include <stdlib.h>
#include <new>
#include "netpack.h"
#include "../base/thread.h"
using namespace net;

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
  /*
    不单独构造和析构，方便moodycamel.ReaderWriterQueue 快速出入队列
    使用msg_init等和msg_free,进行初始化和释放资源
  */
  struct BigMsg
  {
    int8_t*          data_;
    uint16_t         capcity_;
    msg_free_fn*     ffn_;
    void*            hint_;
    base::AtomicNumber refcnt_;
  };
  struct InnerMsg
  {
    union
    {
      struct
      {
        int8_t   data_[max_vsm_size];
        uint16_t size_;
        int8_t   type_;
        int8_t   flags_;
      }stack_;
      struct
      {
        BigMsg*   ptr_;
        int8_t    pad_[max_vsm_size-sizeof(BigMsg*)];
        uint16_t  size_;
        int8_t    type_;
        int8_t    flags_;
      }heap_;
    }u_;
  };
}

void net::msg_init(Msg* msg)
{
  InnerMsg* inmsg=(InnerMsg*)msg;
  inmsg->u_.stack_.type_=type_vsm;
  inmsg->u_.stack_.size_=0;
  inmsg->u_.stack_.flags_=0;
}

void net::msg_init_size(Msg* msg,int size)
{
  InnerMsg* inmsg=(InnerMsg*)msg;
  if (size<=max_vsm_size) 
  {
    inmsg->u_.stack_.type_=type_vsm;
    inmsg->u_.stack_.flags_=0;
    inmsg->u_.stack_.size_=(uint16_t)size;
  }
  else if(size<std::numeric_limits<uint16_t>::max())
  {
    inmsg->u_.heap_.type_=type_lmsg;
    inmsg->u_.heap_.flags_=0;
    inmsg->u_.heap_.size_=(uint16_t)size;
    inmsg->u_.heap_.ptr_=(BigMsg*)malloc(sizeof(BigMsg)+size);
    inmsg->u_.heap_.ptr_->data_=(int8_t*)(inmsg->u_.heap_.ptr_+1);
    inmsg->u_.heap_.ptr_->capcity_=size;
    inmsg->u_.heap_.ptr_->ffn_=nullptr;
    inmsg->u_.heap_.ptr_->hint_=nullptr;
    new(&inmsg->u_.heap_.ptr_->refcnt_) base::AtomicNumber();
  }
  else {assert(false);}
}

void net::msg_init_data(Msg* msg,int8_t* data,int size,msg_free_fn* ffn,void* hint)
{
  InnerMsg* inmsg=(InnerMsg*)msg;
  inmsg->u_.heap_.type_=type_lmsg;
  inmsg->u_.heap_.flags_=0;
  inmsg->u_.heap_.size_=size;
  inmsg->u_.heap_.ptr_=(BigMsg*)malloc(sizeof(BigMsg));
  inmsg->u_.heap_.ptr_->data_=data;
  inmsg->u_.heap_.ptr_->capcity_=size;
  inmsg->u_.heap_.ptr_->ffn_=ffn;
  inmsg->u_.heap_.ptr_->hint_=hint;
  new(&inmsg->u_.heap_.ptr_->refcnt_) base::AtomicNumber();
}

void net::msg_init_delimiter(Msg* msg)
{
  InnerMsg* inmsg=(InnerMsg*)msg;
  inmsg->u_.stack_.type_=type_delimiter;
  inmsg->u_.stack_.flags_=0;
  inmsg->u_.stack_.size_=0;
}

bool net::msg_is_delimiter(Msg* msg)
{
  InnerMsg* inmsg=(InnerMsg*)msg;
  return inmsg->u_.stack_.type_==type_delimiter;
}

int  net::msg_size(Msg* msg)
{
  InnerMsg* inmsg=(InnerMsg*)msg;
  return inmsg->u_.stack_.size_;
}

int net::msg_capcity(Msg* msg)
{
  InnerMsg* inmsg=(InnerMsg*)msg;
  switch(inmsg->u_.stack_.type_)
  {
  case type_vsm:
    return max_vsm_size;
  case type_lmsg:
    return inmsg->u_.heap_.ptr_->capcity_;
  default:
    return 0;
  }
}

int8_t* net::msg_data(Msg* msg)
{
  InnerMsg* inmsg=(InnerMsg*)msg;
  switch(inmsg->u_.stack_.type_)
  {
  case type_vsm:
    return inmsg->u_.stack_.data_;
  case type_lmsg:
    return inmsg->u_.heap_.ptr_->data_;
  default:
    return nullptr;
  }
}

void net::msg_add_ref(Msg* msg,int ref)
{
  InnerMsg* inmsg=(InnerMsg*)msg;
  if(ref<=0)
    return;
  if(inmsg->u_.stack_.type_==type_lmsg)
  {
    if(inmsg->u_.stack_.flags_&shared)
      inmsg->u_.heap_.ptr_->refcnt_.Add(ref);
    else
    {
      inmsg->u_.heap_.ptr_->refcnt_.Add(ref+1);
      inmsg->u_.stack_.flags_|=shared;
    }
  }
}

void net::msg_sub_ref(Msg* msg,int ref)
{
  InnerMsg* inmsg=(InnerMsg*)msg;
  if(ref<=0)
    return;
  if(inmsg->u_.stack_.type_!=type_lmsg||!(inmsg->u_.stack_.flags_&shared))
  {
    msg_free(msg);
    return;
  }
  BigMsg* big=inmsg->u_.heap_.ptr_;
  if(!big->refcnt_.Sub(ref)) 
  {
    big->refcnt_.~AtomicNumber();
    if(big->ffn_)
      big->ffn_(big->data_,big->hint_);
    free (big);
  }
}

void net::msg_free(Msg* msg)
{
  InnerMsg* inmsg=(InnerMsg*)msg;
  if(inmsg->u_.heap_.type_==type_lmsg)
  {
    BigMsg* big=inmsg->u_.heap_.ptr_;
    if(!(inmsg->u_.heap_.flags_&shared) || !big->refcnt_.Sub(1)) 
    {
      big->refcnt_.~AtomicNumber();
      if(big->ffn_)
        big->ffn_(big->data_,big->hint_);
      free(big);
    }
  }
  inmsg->u_.heap_.type_=0;
}

void net::msg_copy(Msg* src,Msg* dst)
{  
  InnerMsg* inmsg_s=(InnerMsg*)src;
  InnerMsg* inmsg_d=(InnerMsg*)dst;
  msg_free(dst);
  if(inmsg_s->u_.heap_.type_==type_lmsg)
  {
    if(inmsg_s->u_.heap_.flags_&shared)
      inmsg_s->u_.heap_.ptr_->refcnt_.Add(1);
    else
    {
      inmsg_s->u_.heap_.ptr_->refcnt_.Set(2);
      inmsg_s->u_.heap_.flags_|=shared;
    }
  }
  *dst=*src;
}

void net::msg_move(Msg* src,Msg* dst)
{
  msg_free(dst);
  *dst=*src;
  msg_init(src);
}