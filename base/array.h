#ifndef _BASE_ARRAY_H
#define _BASE_ARRAY_H

// from 0mq
namespace base
{

template<int ID=0>
class ArrayEntry
{
public:
  ArrayEntry():array_index_(-1){}
  virtual ~ArrayEntry(){}
  int  get_array_index(){return array_index_;}
  void set_array_index(int idx){array_index_=idx;}
  bool is_valid_index(){return array_index_>=0;}
private:
  int array_index_;
  ArrayEntry(const ArrayEntry&);
  const ArrayEntry& operator=(const ArrayEntry&);
};

template<typename T,int ID=0>
class Array
{
public:
  Array(){}
  virtual ~Array(){}
  typedef typename std::vector<T*>::size_type SIZE_T;
  SIZE_T size()
  {
    return vec_.size();
  }
  bool empty()
  {
    return vec_.empty();
  }
  void clear()
  {
    vec_.clear();
  }
  T* operator[](SIZE_T idx)
  {
    return vec_[idx];
  }
  void push_back(T* entry)
  {
    assert(!entry->is_valid_index());
    entry->set_array_index(vec_.size());
    vec_.push_back(entry);
  }
  void erase(SIZE_T idx)
  {
    T* e=vec_.back();
    e->set_array_index(idx);
    vec_[idx]->set_array_index(-1);
    vec_[idx]=e;
    vec_.pop_back();
  }
  void erase(T* entry)
  {
    if(!entry->is_valid_index())
      return;
    if(entry->get_array_index()>=(int)vec_.size())
      return;
    erase(entry->get_array_index());
  }
  void swap(SIZE_T idx1,SIZE_T idx2)
  {
    vec_[idx1]->set_array_index(idx2);
    vec_[idx2]->set_array_index(idx1);
    std::swap(vec_[idx1],vec_[idx2]);
  }
private:
  typedef std::vector<T*> VEC;
  VEC vec_;
  Array(const Array&);
  Array& operator=(const Array&);
};

}
#endif 