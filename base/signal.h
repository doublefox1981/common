#ifndef _BASE_SIGNAL_H
#define _BASE_SIGNAL_H

// from 0MQ signaler_t
namespace base{
  class Signaler
  {
  public:
    Signaler();
    ~Signaler();
    fd_t getfd();
    void send();
    void recv();
    int  wait(int tms);
  private:
    fd_t r_;
    fd_t w_;
    Signaler(const Signaler& s);
    const Signaler& operator=(const Signaler& s);
  };
}

#endif