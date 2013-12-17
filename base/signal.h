#ifndef _BASE_SIGNAL_H
#define _BASE_SIGNAL_H

// from 0MQ signaler_t
namespace base{
  class ezSignaler
  {
  public:
    ezSignaler();
    ~ezSignaler();
    fd_t getfd();
    void send();
    void recv();
    int  wait(int tms);
  private:
    fd_t r_;
    fd_t w_;
    ezSignaler(const ezSignaler& s);
    const ezSignaler& operator=(const ezSignaler& s);
  };
}

#endif