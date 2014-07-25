#ifndef _FRAMEWORK_BASEOBJECT_H
#define _FRAMEWORK_BASEOBJECT_H
#include <vector>
#include <unordered_map>
#include <stdint.h>
#include "../base/eztimer.h"
using base::Timer;
using base::TimerTask;

namespace framework
{
  class BaseObject
  {
  public:
    BaseObject();
    virtual ~BaseObject();
    uint64_t GetUUID() {return uuid_;}
  private:
    BaseObject(const BaseObject&);
    BaseObject& operator=(const BaseObject&);

    uint64_t uuid_;
    static uint64_t suuid_;
  };

  class Command
  {
  public:
    virtual void OnCommand(BaseObject* o)=0;
    virtual void OnFailed()=0;
  };

  class BaseObjectManager
  {
  public:
    void AddBaseObject(BaseObject* o);
    void RemoveBaseObject(BaseObject* o);
    BaseObject* FindBaseObject(uint64_t uuid);
    void PushCommand(uint64_t uuid,Command* cmd);
    void PushDelayCommand(int64_t now,uint64_t uuid,Command* cmd,int delay);
    void TickCommand(int64_t now);
  public:
    static BaseObjectManager* Instance();
    static void DestroyInstance();
  private:
    struct CommandHelper
    {
      uint64_t uuid_;
      Command* cmd_;
    };
    std::unordered_map<uint64_t,BaseObject*> objmap_;
    std::vector<CommandHelper>               cmdqueue_;
    Timer                                  timer_;
    static BaseObjectManager*                sinstance_;
  };


}

#endif