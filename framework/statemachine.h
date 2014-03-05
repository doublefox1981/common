#ifndef FRAMEWORK_STATEMACHINE_H
#define FRAMEWORK_STATEMACHINE_H

#include <vector>
#include <unordered_map>

namespace framework
{
  template<typename T>
  class State
  {
  public:
    typedef State<T> state_type;
    virtual ~State(){}
    virtual void OnEnter(const T* t)=0;
    virtual void OnExit (const T* t)=0;
    virtual void OnTick (const T* t)=0;
  public:
    void AddTransition(int ev,state_type* s)
    {
      transition_[ev]=s;
    }
    State<T>* GetTransition(int ev)
    {
      return transition_[ev];
    }
  private:
    std::unordered_map<int,state_type*> transition_;
  };

  template<typename T>
  class StateMachine
  {
  public:
    typedef State<T> state_type;
    StateMachine():startstate_(0),curstate_(0){}
    virtual ~StateMachine()
    {
      for(size_t s=0;s<statearray_.size();++s)
        delete statearray_[s];
      statearray_.clear();
    }
  public:
    void AddState(state_type* s)
    {
      statearray_.push_back(s);
    }
    void Start(T* t)
    {
      startstate_->OnEnter(t);
      curstate_=startstate_;
    }
    void Stop(T* t)
    {
      curstate_->OnExit(t);
      curstate_=0;
    }
    void SetStartState(state_type* s)
    {
      startstate_=s;
    }
    void PostEvent(T* t,int ev)
    {
      if(!curstate_)
        return;
      State<T>* nexts=curstate_->GetTransition(ev);
      if(!nexts)
        return;
      curstate_->OnExit(t);
      curstate_=nexts;
      curstate_->OnEnter(t);
    }
    void Tick(T* t,int delta)
    {
      if(!curstate_)
        return;
      curstate_->OnTick(t);
    }
  private:
    State<T>* startstate_;
    State<T>* curstate_;
    std::vector<state_type*> statearray_;
  };
}
#endif