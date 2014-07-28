#ifndef _BASE_UTIL_H
#define _BASE_UTIL_H
#include <string>

#if defined(_MSC_VER)
#define MSVC_PUSH_DISABLE_WARNING(n) __pragma(warning(push)) \
  __pragma(warning(disable:n))
#define MSVC_POP_WARNING() __pragma(warning(pop))
#else
#define MSVC_PUSH_DISABLE_WARNING(n)
#define MSVC_POP_WARNING()
#endif

// steal from google protobuf
#define NO_COPYABLE(TypeName) \
  private: \
    TypeName(const TypeName&); \
    void operator=(const TypeName&)

#if defined(_MSC_VER) 
  #if defined(EZ_LIB_USE_DLLS)
    #define EZ_LIB_EXPORTS __declspec(dllexport)
  #else
    #define EZ_LIB_EXPORTS __declspec(dllimport)
  #endif
#else
#define EZ_LIB_EXPORTS
#endif

#ifdef _MSC_VER
#define EZ_LIB_LONGLONG(x) x##I64
#define EZ_LIB_ULONGLONG(x) x##UI64
#define EZ_LIB_LL_FORMAT_D "I64""d"
#define EZ_LIB_LL_FORMAT_U "I64""u"
#else
#define EZ_LIB_LONGLONG(x) x##LL
#define EZ_LIB_ULONGLONG(x) x##ULL
#define EZ_LIB_LL_FORMAT_D "ll""d"
#define EZ_LIB_LL_FORMAT_U "ll""u"
#endif

#define EZ_LIB_ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
  static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

template<typename To,typename From>
inline To implicit_cast(From const& f){return f;}

template<typename To,typename From>
inline To down_cast(From* f)
{
  if (false) 
  {
    implicit_cast<From*,To>(0);
  }
#if defined(_DEBUG)
  assert(f == NULL || dynamic_cast<To>(f) != NULL);
#endif
  return static_cast<To>(f);
}

template<bool>
struct CompileAssert {};

#define COMPILE_ASSERT(expr, msg) \
  typedef ::CompileAssert<(bool(expr))> \
  msg[bool(expr) ? 1 : -1]

namespace base
{
  enum EPrintColor 
  {
    COLOR_DEFAULT,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW
  };

  extern int float_to_int(float);
  extern float int_to_float(int);
  extern void colored_printf(EPrintColor color,const char* fmt,...);
  extern void string_printf_impl(std::string& output, const char* format,va_list args); 
  extern std::string string_printf(const char* format, ...);
  extern std::string& string_appendf(std::string* output, const char* format, ...);
  extern void string_printf(std::string* output, const char* format, ...);

  inline char* string_as_array(std::string* str)
  {
    return str->empty()?NULL:&*str->begin();
  }

  template <class ForwardIterator>
  void stl_delete_container_pointers(ForwardIterator begin,ForwardIterator end) 
  {
    while (begin!=end) 
    {
      ForwardIterator temp=begin;
      ++begin;
      delete *temp;
    }
  }

  inline void stl_string_resize_uninitialized(std::string* s,size_t new_size)
  {
    s->resize(new_size);
  }

  template <class T>
  void stl_delete_elements(T *container) 
  {
    if(!container) return;
    stl_delete_container_pointers(container->begin(),container->end());
    container->clear();
  }

  template <class T>
  void stl_delete_values(T *v) 
  {
    if(!v) return;
    for(typename T::iterator i = v->begin();i != v->end();++i) 
    {
      delete i->second;
    }
    v->clear();
  }

  template <class C> class scoped_ptr;
  template <class C> class scoped_array;

  template <class C>
  class scoped_ptr 
  {
  public:
    typedef C element_type;
    explicit scoped_ptr(C* p = NULL) : ptr_(p) { }
    ~scoped_ptr() 
    {
      enum { type_must_be_complete = sizeof(C) };
      delete ptr_;
    }
    void reset(C* p = NULL) 
    {
      if (p != ptr_) 
      {
        enum { type_must_be_complete = sizeof(C) };
        delete ptr_;
        ptr_ = p;
      }
    }
    C& operator*() const 
    {
      assert(ptr_ != NULL);
      return *ptr_;
    }
    C* operator->() const  
    {
      assert(ptr_ != NULL);
      return ptr_;
    }
    C* get() const { return ptr_; }
    bool operator==(C* p) const { return ptr_ == p; }
    bool operator!=(C* p) const { return ptr_ != p; }
    void swap(scoped_ptr& p2) {
      C* tmp = ptr_;
      ptr_ = p2.ptr_;
      p2.ptr_ = tmp;
    }
    C* release() 
    {
      C* retVal = ptr_;
      ptr_ = NULL;
      return retVal;
    }

  private:
    C* ptr_;
    template <class C2> bool operator==(scoped_ptr<C2> const& p2) const;
    template <class C2> bool operator!=(scoped_ptr<C2> const& p2) const;
    scoped_ptr(const scoped_ptr&);
    void operator=(const scoped_ptr&);
  };

  template <class C>
  class scoped_array
  {
  public:
    typedef C element_type;
    explicit scoped_array(C* p = NULL) : array_(p) { }
    ~scoped_array() 
    {
      enum { type_must_be_complete = sizeof(C) };
      delete[] array_;
    }
    void reset(C* p = NULL) 
    {
      if (p != array_) 
      {
        enum { type_must_be_complete = sizeof(C) };
        delete[] array_;
        array_ = p;
      }
    }
    C& operator[](std::ptrdiff_t i) const 
    {
      //assert(i >= 0);
      //assert(array_ != NULL);
      return array_[i];
    }
    C* get() const 
    {
      return array_;
    }
    bool operator==(C* p) const { return array_ == p; }
    bool operator!=(C* p) const { return array_ != p; }
    void swap(scoped_array& p2) 
    {
      C* tmp = array_;
      array_ = p2.array_;
      p2.array_ = tmp;
    }
    C* release() 
    {
      C* retVal = array_;
      array_ = NULL;
      return retVal;
    }

  private:
    C* array_;
    template <class C2> bool operator==(scoped_array<C2> const& p2) const;
    template <class C2> bool operator!=(scoped_array<C2> const& p2) const;
    scoped_array(const scoped_array&);
    void operator=(const scoped_array&);
  };
}
#endif