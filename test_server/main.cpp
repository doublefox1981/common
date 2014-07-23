#include "../base/portable.h"
#include "../base/memorystream.h"
#include "../base/scopeguard.h"
#include "../base/eztime.h"
#include "../base/eztimer.h"
#include "../base/thread.h"
#include "../base/util.h"
#include "../base/logging.h"

#include "../net/net_interface.h"
#include "../net/netpack.h"

#include "../framework/statemachine.h"

bool exit_=false;
#ifdef __linux__
#include <signal.h>
void SignalExit(int no)
{
  exit_=true;
  signal(no,SIG_DFL);
}
#else
BOOL CtrlHandler(DWORD CtrlType)
{
  switch(CtrlType)
  {
  case CTRL_BREAK_EVENT:
    {
      exit_=true;
      return TRUE;
    }
    break;
  case  CTRL_C_EVENT:
    {
      exit_=true;
      return TRUE;
    }
    break;
  default: break;
  }
  return FALSE;
}
#endif

void ProcessSignal()
{
#ifdef __linux__
  signal(SIGINT,SignalExit);
#else
  SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler,TRUE);
#endif
}

class Monster
{
public:
  enum
  {
    IDLE,
    STAND,
    MOVE,
    ATTACK,
  };
};

using namespace framework;
class IdleState:public State<Monster>
{
public:
  virtual void OnEnter(const Monster* t){}
  virtual void OnExit(const Monster* t){}
  virtual void OnTick(const Monster* t){}
};

class StandState:public State<Monster>
{
public:
  virtual void OnEnter(const Monster* t){}
  virtual void OnExit(const Monster* t){}
  virtual void OnTick(const Monster* t){}
};

class MoveState:public State<Monster>
{
public:
  virtual void OnEnter(const Monster* t){}
  virtual void OnExit(const Monster* t){}
  virtual void OnTick(const Monster* t){}
};

class AttackState:public State<Monster>
{
public:
  virtual void OnEnter(const Monster* t){}
  virtual void OnExit(const Monster* t){}
  virtual void OnTick(const Monster* t){}
};

struct SFuncArg
{
  int a;
  std::string s;
};
void func(const SFuncArg& a)
{

}

void func1()
{

}

void stdfunc(std::function<void()>& func)
{
  func();
}

int main()
{
  stdfunc(std::function<void()>(func1));
  Monster m;
  IdleState* idle=new IdleState;
  StandState* stand=new StandState;
  MoveState* mov=new MoveState;
  AttackState* attack=new AttackState;
  idle->AddTransition(Monster::MOVE,mov);
  idle->AddTransition(Monster::ATTACK,attack);
  attack->AddTransition(Monster::STAND,stand);
  framework::StateMachine<Monster> sm;
  sm.SetStartState(idle);
  sm.Start(&m);
  
  base::ezTimer gTimer;
  //gTimer.runAfter(base::ezNowTick(),10000,std::function<void()>(func1));
  SFuncArg arg;
  arg.a=10;
  arg.s="funcstruct";
  //gTimer.runAfter<SFuncArg>(base::ezNowTick(),10000,std::function<void(const SFuncArg&)>(func),arg);

  base::ezLogger::instance()->Start();
  net::EzNetInitialize();
  net::ezIConnnectionHander* hander=new net::ezServerHander;
  net::ezIDecoder* decoder=new net::ezMsgDecoder(20000);
  net::ezIEncoder* encoder=new net::ezMsgEncoder;
  net::ezEventLoop* ev=net::CreateEventLoop(hander,decoder,encoder,4);
  if(net::ServeOnPort(ev,10011)!=0)
  {
    LOG_ERROR("bind on port %d fail",10011);
    return -1;
  }
  else
    LOG_INFO("bind on port %d ok",10011);
  base::ScopeGuard guard([&](){net::DestroyEventLoop(ev); delete hander; delete decoder; delete encoder;});

  int seq=0;
  while(!exit_)
  {
    gTimer.tick(base::ezNowTick());
    sm.Tick(&m,0);
    net::EventProcess(ev);
    base::ezSleep(1);
  }
  return 0;
}