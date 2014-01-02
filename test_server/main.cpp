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

int main()
{
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

    net::EventProcess(ev);
    base::ezSleep(1);
  }
  return 0;
}