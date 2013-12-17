#include "netpack.h"
#include "buffer.h"
#include <string.h>
using namespace net;
using namespace base;

void net::ezMsgInit(ezMsg* msg)
{
  msg->type_=type_vsm;
  msg->size_=0;
  msg->flags_=0;
}

void net::ezMsgInitSize(ezMsg* msg,int size)
{
  if (size<=max_vsm_size) 
  {
    msg->type_=type_vsm;
    msg->flags_=0;
    msg->size_=(uint16_t)size;
  }
  else if(size<std::numeric_limits<uint16_t>::max())
  {
    msg->type_=type_lmsg;
    msg->flags_=0;
    msg->size_=0;
    msg->heap_.ptr_=(ezMsg::ezBigMsg*)malloc(sizeof(ezMsg::ezBigMsg)+size);
    msg->heap_.ptr_->data_=(int8_t*)(msg->heap_.ptr_+1);
    msg->heap_.ptr_->capcity_=size;
    msg->heap_.ptr_->ffn_=nullptr;
    msg->heap_.ptr_->hint_=nullptr;
    new(&msg->heap_.ptr_->refcnt_) base::AtomicNumber();
  }
  else {assert(false);}
}

void net::ezMsgInitData(ezMsg* msg,int8_t* data,int size,msg_free_fn* ffn,void* hint)
{
  msg->type_=type_lmsg;
  msg->flags_=0;
  msg->size_=size;
  msg->heap_.ptr_=(ezMsg::ezBigMsg*)malloc(sizeof(ezMsg::ezBigMsg));
  msg->heap_.ptr_->data_=data;
  msg->heap_.ptr_->capcity_=size;
  msg->heap_.ptr_->ffn_=ffn;
  msg->heap_.ptr_->hint_=hint;
  new(&msg->heap_.ptr_->refcnt_) base::AtomicNumber();
}

void net::ezMsgInitDelimiter(ezMsg* msg)
{
  msg->type_=type_delimiter;
  msg->flags_=0;
  msg->size_=0;
}

int  net::ezMsgSize(ezMsg* msg)
{
  return msg->size_;
}

int net::ezMsgCapcity(ezMsg* msg)
{
  switch(msg->type_)
  {
  case type_vsm:
    return max_vsm_size;
  case type_lmsg:
    return msg->heap_.ptr_->capcity_;
  default:
    return 0;
  }
}

int8_t* net::ezMsgData(ezMsg* msg)
{
  switch(msg->type_)
  {
  case type_vsm:
    return msg->stack_.data_;
  case type_lmsg:
    return msg->heap_.ptr_->data_;
  default:
    return nullptr;
  }
}

void net::ezMsgAddRef(ezMsg* msg,int ref)
{
  if(ref<=0)
    return;
  if(msg->type_==type_lmsg)
  {
    if(msg->flags_&shared)
      msg->heap_.ptr_->refcnt_.Add(ref);
    else
    {
      msg->heap_.ptr_->refcnt_.Add(ref+1);
      msg->flags_|=shared;
    }
  }
}

void net::ezMsgSubRef(ezMsg* msg,int ref)
{
  if(ref<=0)
    return;
  if(msg->type_!=type_lmsg||!(msg->flags_&shared))
  {
    ezMsgFree(msg);
    return;
  }
  ezMsg::ezBigMsg* big=msg->heap_.ptr_;
  if(!big->refcnt_.Sub(ref)) 
  {
    big->refcnt_.~AtomicNumber();
    if(big->ffn_)
      big->ffn_(big->data_,big->hint_);
    free (big);
  }
}

void net::ezMsgFree(ezMsg* msg)
{
  if(msg->type_==type_lmsg)
  {
    ezMsg::ezBigMsg* big=msg->heap_.ptr_;
    if(!(msg->flags_&shared) || !big->refcnt_.Sub(1)) 
    {
      big->refcnt_.~AtomicNumber();
      if(big->ffn_)
        big->ffn_(big->data_,big->hint_);
      free (big);
    }
  }
  msg->type_=0;
}

void net::ezMsgCopy(ezMsg* src,ezMsg* dst)
{  
  ezMsgFree(dst);
  if(src->type_==type_lmsg)
  {
    if(src->flags_&shared)
      src->heap_.ptr_->refcnt_.Add(1);
    else
    {
      src->heap_.ptr_->refcnt_.Set(2);
      src->flags_|=shared;
    }
  }
  *dst=*src;
}

void net::ezMsgMove(ezMsg* src,ezMsg* dst)
{
  ezMsgFree(dst);
  *dst=*src;
  ezMsgInit(src);
}