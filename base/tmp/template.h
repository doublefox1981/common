#ifndef _CROSSENGINE_TEMPLATE_H_
#define _CROSSENGINE_TEMPLATE_H_

namespace Cross
{

// Static if
template<bool B,class T1,class T2> struct if_{typedef T1 type;};
template<class T1,class T2> struct if_<false,T1,T2>{typedef T2 type;};
// ¾²Ì¬¶ÏÑÔ
template<bool B> class sassert;
template<> class sassert<true>{};

template<class T> struct is_pointer{static const bool value=false;};
template<class T> struct is_pointer<T*>{static const bool value=true;};
struct true_{static const bool value=true;};
struct false_{static const bool value=false;};

template<class Src,class Dst>
struct can_implicit_convert
{
private:
    struct yes_{char m;};
    struct no_{char m[2];};
    template<class T> struct convertwrapper{convertwrapper(T);};
    static no_ convert(...);
    static yes_ convert(convertwrapper<Dst>);
    static Src* arg;
public:
    static const bool value=sizeof(convert(*arg))==sizeof(yes_);
};

template<class Src,class Dst>
Src* can_implicit_convert<Src,Dst>::arg;

//lazy instantiation to avoid warnnings
template<bool T,class Src,class Dst>
struct lazy_convert{typedef false_ type;};
template<class Src,class Dst>
struct lazy_convert<true,Src,Dst>{typedef can_implicit_convert<Src,Dst> type;};

template<class Src,class Dst>
struct lazy_convert_condition
{
    static const bool value=sizeof(Src)<=sizeof(Dst);
};
template<class Dst>
struct lazy_convert_condition<void,Dst>
{
    static const bool value=false;
};
template<>
struct lazy_convert_condition<void,void>;

template<class Src,class Dst>
struct can_convert
{
    typedef typename lazy_convert<lazy_convert_condition<Src,Dst>::value,Src,Dst>::type type;
    static const bool value=type::value;
};

// ¾²Ì¬Ñ­»·¡£²ÎÊý¿ÉÀ©Õ¹
template<int Beg,int End,template<int> class Cmd,class T1=void,class T2=void,class T3=void,class T4=void,class T5=void>
struct for_
{
  static void Do(T1* t1,T2* t2,T3* t3,T4* t4,T5* t5)
  {
    Cmd<Beg>::Do(t1,t2,t3,t4,t5);
    for_<Beg+1,End,Cmd,T1,T2,T3,T4,T5>::Do(t1,t2,t3,t4,t5);
  }
};

template<int End,template<int> class Cmd,class T1,class T2,class T3,class T4,class T5>
struct for_<End,End,Cmd,T1,T2,T3,T4,T5>
{
  static void Do(T1* t1,T2* t2,T3* t3,T4* t4,T5* t5){}
};

template<int Beg,int End,template<int> class Cmd,class T1,class T2,class T3,class T4>
struct for_<Beg,End,Cmd,T1,T2,T3,T4>
{
  static void Do(T1* t1,T2* t2,T3* t3,T4* t4)
  {
    Cmd<Beg>::Do(t1,t2,t3,t4);
    for_<Beg+1,End,Cmd,T1,T2,T3,T4>::Do(t1,t2,t3,t4);
  }
};
template<int End,template<int> class Cmd,class T1,class T2,class T3,class T4>
struct for_<End,End,Cmd,T1,T2,T3,T4>
{
  static void Do(T1* t1,T2* t2,T3* t3,T4* t4){}
};

template<int Beg,int End,template<int> class Cmd,class T1,class T2,class T3>
struct for_<Beg,End,Cmd,T1,T2,T3>
{
  static void Do(T1* t1,T2* t2,T3* t3)
  {
    Cmd<Beg>::Do(t1,t2,t3);
    for_<Beg+1,End,Cmd,T1,T2,T3>::Do(t1,t2,t3);
  }
};
template<int End,template<int> class Cmd,class T1,class T2,class T3>
struct for_<End,End,Cmd,T1,T2,T3>
{
  static void Do(T1* t1,T2* t2,T3* t3){}
};

template<int Beg,int End,template<int> class Cmd,class T1,class T2>
struct for_<Beg,End,Cmd,T1,T2>
{
  static void Do(T1* t1,T2* t2)
  {
    Cmd<Beg>::Do(t1,t2);
    for_<Beg+1,End,Cmd,T1,T2>::Do(t1,t2);
  }
};
template<int End,template<int> class Cmd,class T1,class T2>
struct for_<End,End,Cmd,T1,T2>
{
  static void Do(T1* t1,T2* t2){}
};

template<int Beg,int End,template<int> class Cmd,class T1>
struct for_<Beg,End,Cmd,T1>
{
  static void Do(T1* t1)
  {
    Cmd<Beg>::Do(t1);
    for_<Beg+1,End,Cmd,T1>::Do(t1);
  }
};
template<int End,template<int> class Cmd,class T1>
struct for_<End,End,Cmd,T1>
{
  static void Do(T1* t1){}
};

template<int Beg,int End,template<int> class Cmd>
struct for_<Beg,End,Cmd>
{
  static void Do()
  {
    Cmd<Beg>::Do();
    for_<Beg+1,End,Cmd>::Do();
  }
};
template<int End,template<int> class Cmd>
struct for_<End,End,Cmd>
{
  static void Do(){}
};

template<class T1,class T2,class T3=void,class T4=void,class T5=void>
struct Tuple{T1 t1;T2 t2;T3 t3;T4 t4;T5 t5;};

template<class T1,class T2,class T3,class T4>
struct Tuple<T1,T2,T3,T4>{T1 t1;T2 t2;T3 t3;T4 t4;};

template<class T1,class T2,class T3>
struct Tuple<T1,T2,T3>{T1 t1;T2 t2;T3 t3;};

template<class T1,class T2>
struct Tuple<T1,T2>{T1 t1;T2 t2;};

}

#endif//_CROSSENGINE_TEMPLATE_H_
