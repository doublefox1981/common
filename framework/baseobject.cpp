#include "baseobject.h"
using framework::BaseObject;
using framework::Command;
using framework::BaseObjectManager;

BaseObject::BaseObject()
{
  uuid_=suuid_++;
  BaseObjectManager::Instance()->AddBaseObject(this);
}

BaseObject::~BaseObject()
{
  BaseObjectManager::Instance()->RemoveBaseObject(this);
  uuid_=0;
}

BaseObjectManager* BaseObjectManager::sinstance_=NULL;
BaseObjectManager* BaseObjectManager::Instance()
{
  if(!sinstance_)
    sinstance_=new BaseObjectManager;
  return sinstance_;
}

void BaseObjectManager::DestroyInstance()
{
  if(sinstance_)
  {
    delete sinstance_;
    sinstance_=NULL;
  }
}

void BaseObjectManager::AddBaseObject(BaseObject* o)
{
  objmap_[o->GetUUID()]=o;
}

void BaseObjectManager::RemoveBaseObject(BaseObject* o)
{
  auto iter=objmap_.find(o->GetUUID());
  if(iter!=objmap_.end())
    objmap_.erase(iter);
}

void BaseObjectManager::PushCommand(uint64_t uuid,Command* cmd)
{
  framework::BaseObjectManager::CommandHelper helper;
  helper.uuid_=uuid;
  helper.cmd_=cmd;
  cmdqueue_.push_back(helper);
}

void BaseObjectManager::TickCommand(int64_t now)
{
  for(size_t s=0;s<cmdqueue_.size();++s)
  {
    framework::BaseObjectManager::CommandHelper& helper=cmdqueue_[s];
    BaseObject* o=FindBaseObject(helper.uuid_);
    if(o)
      helper.cmd_->OnCommand(o);
    else
      helper.cmd_->OnFailed();
    delete helper.cmd_;
  }
  cmdqueue_.clear();
  timer_.tick(now);
}

BaseObject* BaseObjectManager::FindBaseObject(uint64_t uuid)
{
  return objmap_[uuid];
}

namespace
{
  class DelayCommandTimer:public TimerTask
  {
  public:
    DelayCommandTimer(int64_t tid,uint64_t obj_uuid,Command* cmd):TimerTask(tid),obj_uuid_(obj_uuid),cmd_(cmd){}
    virtual void run()
    {
      BaseObject* o=BaseObjectManager::Instance()->FindBaseObject(obj_uuid_);
      if(o)
        cmd_->OnCommand(o);
      else
        cmd_->OnFailed();
      delete cmd_;
      cmd_=NULL;
    }
  private:
    uint64_t obj_uuid_;
    Command* cmd_;
  };
}

void BaseObjectManager::PushDelayCommand(int64_t now,uint64_t uuid,Command* cmd,int delay)
{
  TimerTask* t=new DelayCommandTimer(timer_.gen_timer_uuid(),uuid,cmd);
  t->config(now,delay,0);
  timer_.add_timer_task(t);
}
