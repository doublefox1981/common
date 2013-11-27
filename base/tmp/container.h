#ifndef _CROSSENGINE_CONTAINER_H_
#define _CROSSENGINE_CONTAINER_H_
#include <vector>
#include "list.h"
#include "template.h"
#include "portable.h"
#include "common.h"
#include "thread.h"

// -------------------------------
//          Hash Table
// -------------------------------
template<class Key>
struct HashFunc;

struct HashCollideList{HashCollideList* next;};

template<class Key>
struct NormalHashNode
{
    explicit NormalHashNode(const Key& k):key(k){}//lst和hlst在构造之后赋值
    list_head lst;
    HashCollideList hlst;
    Key key;
};
typedef std::vector<HashCollideList> NormalVector;

template<class Key,
    class Hash=HashFunc<Key>,
    class HashNode=NormalHashNode<Key>,
    class Vec=NormalVector,
    int ISize=8 // 必须是2的幂
>
class QueuedHashSet
{
public:
  class iterator
  {
  public:
    iterator(){mPos=NULL;}
    iterator(const list_head* pos){mPos=const_cast<list_head*>(pos);}
    iterator(const iterator& iter){mPos=iter.mPos;}
    bool operator!=(const iterator& it)const{return mPos!=it.mPos;}
    bool operator==(const iterator& it)const{return mPos==it.mPos;}
    iterator& operator++(){mPos=mPos->next;return *this;}
    iterator& operator--(){mPos=mPos->prev;return *this;}
    const Key& operator*() const{HashNode* node=list_entry(mPos,HashNode,lst);return node->key;}
    const Key* operator->() const{HashNode* node=list_entry(mPos,HashNode,lst);return &node->key;}
  public:
    list_head* mPos;
  };
public:
  QueuedHashSet();
  ~QueuedHashSet();
  QueuedHashSet(const QueuedHashSet& other);
  QueuedHashSet& operator=(const QueuedHashSet& other);
  void clear();// 遵守stl的约定,不释放内存,释放内存可以用一个空对象赋值(调用operator=)
  std::pair<iterator,bool> insert(const Key& k);
  iterator set(const Key& k);
  iterator erase(iterator it);
  iterator find(const Key& k) const;
  iterator begin() const{return mList.next;}
  iterator end() const{return &mList;}
  bool remove(const Key& k);
  int size() const{return mSize;}
  bool empty() const{return 0==mSize;}
  void reserve(int size);
private:
  void clearhash(int size);//参数必须是2的幂
  void rehash(int size);//参数必须是2的幂
  iterator addcollide(HashCollideList* h,const Key& k);
  HashCollideList* calchash(const Key& k) const;
  iterator findcollide(HashCollideList* h,const Key& k) const;
  list_head mList;
  int mSize;
  Vec mHash;
  int mMask;
};

template<class Key,class Hash,class HashNode,class Vec,int ISize>
QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::~QueuedHashSet()
{
  list_head* pos,*next;
  list_for_each_safe(pos,next,&mList)
  {
    HashNode* node=list_entry(pos,HashNode,lst);
    delete node;
  }
}

template<class Key,class Hash,class HashNode,class Vec,int ISize>
QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::QueuedHashSet()
{
  INIT_LIST_HEAD(&mList);
  mSize=0;
  HashCollideList tmp;
  tmp.next=NULL;
  mHash.resize(ISize,tmp);
  mMask=ISize-1;
}

template<class Key,class Hash,class HashNode,class Vec,int ISize>
QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::QueuedHashSet(const QueuedHashSet& other)
{
  INIT_LIST_HEAD(&mList);
  clearhash(other.mHash.size());
  list_head* pos;
  list_for_each(pos,&other.mList)
  {
    HashNode* othernode=list_entry(pos,HashNode,lst);
    HashCollideList* h=calchash(othernode->key);
    HashNode* node=new HashNode(othernode->key);
    list_add_tail(&node->lst,&mList);
    HashCollideList* current=&node->hlst;
    current->next=h->next;
    h->next=current;
  }
  mSize=other.mSize;
}

template<class Key,class Hash,class HashNode,class Vec,int ISize>
QueuedHashSet<Key,Hash,HashNode,Vec,ISize>& QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::operator=(const QueuedHashSet& other)
{
  if(this!=&other)
  {
    list_head* otherpos=other.mList.next;
    list_head* pos;
    list_for_each(pos,&mList)
    {
      if(otherpos==&other.mList)
        break;
      HashNode* node=list_entry(pos,HashNode,lst);
      HashNode* othernode=list_entry(otherpos,HashNode,lst);
      node->key=othernode->key;
      otherpos=otherpos->next;
    }
    if(pos!=&mList)
    {
      list_head* next;
      list_for_each_safe_from(pos,next,&mList)
      {
        HashNode* node=list_entry(pos,HashNode,lst);
        list_del(pos);
        delete node;
      }
    }
    if(otherpos!=&other.mList)
    {
      list_for_each_from(otherpos,&other.mList)
      {
        HashNode* othernode=list_entry(otherpos,HashNode,lst);
        HashNode* node=new HashNode(othernode->key);
        list_add_tail(&node->lst,&mList);
      }
    }
    mSize=other.mSize;
    rehash(other.mHash.size());
  }
  return *this;
}

template<class Key,class Hash,class HashNode,class Vec,int ISize>
inline void QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::clearhash(int size)
{
    //清空mHash
    if((int)mHash.size()>size&&(int)mHash.size()>=ISize*8)
        Vec().swap(mHash);// 如果要缩小一个比较大的hash,释放内存
    else
        mHash.clear();// 不释放内存
    HashCollideList tmp;
    tmp.next=NULL;
    mHash.resize(size,tmp);
    mMask=size-1;
}

template<class Key,class Hash,class HashNode,class Vec,int ISize>
void QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::clear()
{
    list_head* pos,*next;
    list_for_each_safe(pos,next,&mList)
    {
        HashNode* node=list_entry(pos,HashNode,lst);
        delete node;
    }
    INIT_LIST_HEAD(&mList);
    mSize=0;
    clearhash(mHash.size());
}

template<class Key,class Hash,class HashNode,class Vec,int ISize>
inline HashCollideList* QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::calchash(const Key& k) const
{
  assert(mMask==(int)mHash.size()-1);
  int i=Hash::Do(k)&mMask;
  return const_cast<HashCollideList*>(&mHash[i]);
}

template<class Key,class Hash,class HashNode,class Vec,int ISize>
typename QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::iterator QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::findcollide(HashCollideList* h,const Key& k) const
{
  for(HashCollideList* pos=h->next;pos;pos=pos->next)
  {
      HashNode* node=container_of(pos,HashNode,hlst);
      if(node->key==k)
          return &node->lst;
  }
  return &mList;
}

template<class Key,class Hash,class HashNode,class Vec,int ISize>
void QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::rehash(int size)
{
  assert(size>=ISize);
  //assert((size==mHash.size()*2)||size==mHash.size()/2);
  clearhash(size);
  list_head* pos;
  list_for_each(pos,&mList)
  {
    HashNode* node=list_entry(pos,HashNode,lst);
    HashCollideList* h=calchash(node->key);
    HashCollideList* current=&node->hlst;
    current->next=h->next;
    h->next=current;
  }
}

template<class Key,class Hash,class HashNode,class Vec,int ISize>
typename QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::iterator QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::addcollide(HashCollideList* h,const Key& k)
{
  HashNode* node=new HashNode(k);
  list_add_tail(&node->lst,&mList);
  assert(mSize>=0);
  ++mSize;
  HashCollideList* current=&node->hlst;
  current->next=h->next;
  h->next=current;
  if((int)mHash.size()<mSize)
    rehash(mHash.size()*2);
  assert(mMask==(int)mHash.size()-1);
  return &node->lst;
}

template<class Key,class Hash,class HashNode,class Vec,int ISize>
typename QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::iterator QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::erase(typename QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::iterator iter)
{
    assert(iter.mPos!=&mList);
    list_head* next=iter.mPos->next;
    HashNode* node=list_entry(iter.mPos,HashNode,lst);
    HashCollideList* h=calchash(node->key);
    for(HashCollideList** prev=&h->next;*prev;prev=&(*prev)->next)
    {
        HashCollideList* cur=*prev;
        HashNode* current=container_of(cur,HashNode,hlst);
        if(current==node)
        {
            *prev=(*prev)->next;
            break;
        }
    }
    list_del(&node->lst);
    --mSize;
    assert(mSize>=0);
    delete node;
    if((int)mHash.size()>ISize&&(int)mHash.size()>=mSize*4)
        rehash(mHash.size()/2);
    return next;
}

template<class Key,class Hash,class HashNode,class Vec,int ISize>
inline typename QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::iterator QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::find(const Key& k) const
{
  HashCollideList* h=calchash(k);
  return findcollide(h,k);
}

template<class Key,class Hash,class HashNode,class Vec,int ISize>
std::pair<typename QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::iterator,bool> QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::insert(const Key& k)
{
  HashCollideList* h=calchash(k);
  iterator iter=findcollide(h,k);
  if(end()==iter)
  {
    iter=addcollide(h,k);
    return std::pair<typename QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::iterator,bool>(iter,true);
  }
  // 无法插入
  return std::pair<typename QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::iterator,bool>(iter,false);
}

template<class Key,class Hash,class HashNode,class Vec,int ISize>
typename QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::iterator QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::set(const Key& k)
{
  HashCollideList* h=calchash(k);
  iterator iter=findcollide(h,k);
  if(end()==iter)
  {
    iter=addcollide(h,k);
    return iter;
  }
  HashNode* node=list_entry(iter.mPos,HashNode,lst);
  node->key=k;
  return iter;
}

template<class Key,class Hash,class HashNode,class Vec,int ISize>
bool QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::remove(const Key& k)
{
    HashNode* node=NULL;
    HashCollideList* h=calchash(k);
    for(HashCollideList** prev=&h->next;*prev;prev=&(*prev)->next)
    {
        HashCollideList* cur=*prev;
        HashNode* current=container_of(cur,HashNode,hlst);
        if(current->key==k)
        {
            node=current;
            *prev=(*prev)->next;
            break;
        }
    }
    if(node)
    {
        list_del(&node->lst);
        --mSize;
        assert(mSize>=0);
        delete node;
        if((int)mHash.size()>ISize&&(int)mHash.size()>=mSize*4)
            rehash(mHash.size()/2);
    }
    return node!=NULL;
}

template<class Key,class Hash,class HashNode,class Vec,int ISize>
void QueuedHashSet<Key,Hash,HashNode,Vec,ISize>::reserve(int size)
{
    if(size>(int)mHash.size())
    {
        uint32 log2=Cross::CalcLog2Base(size);//找到size对应的log2下限,如果size是2的幂,下限就等于上限
        if(((size-1)&size)!=0)//如果size不是2的幂
            ++log2;//找到size对应的log2上限
        rehash(1<<log2);
    }
}

template<class K,class V>
struct HashPair
{
  HashPair(const K& k,const V& v):first(k),second(v){}
  K first;
  mutable V second;
  bool operator==(const HashPair& p)const
  {
    return first==p.first;
  }
};

//!重要!针对const char*的Key不进行拷贝，必须在QueuedHashMap生存期间保持生存并不被修改
template<class V>
struct HashPair<const char*,V>
{
  HashPair(const char* k,const V& v):first(k),second(v){}
  const char* first;
  mutable V second;
  bool operator==(const HashPair& p)const
  {
    return strcmp(first,p.first)==0;
  }
};

template<class K,class V,
    class Hash=HashFunc<HashPair<K,V> >,
    class HashNode=NormalHashNode<HashPair<K,V> >,
    class Vec=NormalVector,
    int ISize=8 // 必须是2的幂
>
class QueuedHashMap
{
  typedef HashPair<K,V> Pair;
  typedef QueuedHashSet<Pair,Hash,HashNode,Vec,ISize> HashType;
public:
  typedef typename HashType::iterator iterator;
public:
  iterator erase(iterator it){return mImpl.erase(it);}
  iterator find(const K& k) const
  {
      // 要求Pair.operator==和HashFunc<Pair>都只能访问Pair.first
      const Pair* p=container_of(&k,Pair,first);
      return mImpl.find(*p);
  }
  bool remove(const K& k)
  {
      // 要求Pair.operator==和HashFunc<Pair>都只能访问Pair.first
      const Pair* p=container_of(&k,Pair,first);
      return mImpl.remove(*p);
  }
  std::pair<iterator,bool> insert(const K& k,const V& v){return mImpl.insert(Pair(k,v));}
  iterator set(const K& k,const V& v){return mImpl.set(Pair(k,v));}
  iterator begin() const{return mImpl.begin();}
  iterator end() const{return mImpl.end();}
  void clear(){mImpl.clear();}
  int size() const{return mImpl.size();}
  bool empty() const{return mImpl.empty();}
private:
  HashType mImpl;
};
template<class K,class V>
struct HashFunc<HashPair<K,V> >
{
  static int Do(const HashPair<K,V>& k)
  {
    return HashFunc<K>::Do(k.first);
  }
};

#define SimpleHashFunc(Type) \
template<> struct HashFunc<Type> \
{ \
  static int Do(Type k) \
  { \
    return (k*2654435761U)&0x7fffffff; \
  } \
}

SimpleHashFunc(int);
SimpleHashFunc(short);
SimpleHashFunc(char);
SimpleHashFunc(long);
SimpleHashFunc(unsigned int);
SimpleHashFunc(unsigned short);
SimpleHashFunc(unsigned char);
SimpleHashFunc(unsigned long);

template<> struct HashFunc<uint64>
{
  static int Do(uint64 k)
  {
    return int(((k&0xffffffff)+(k>>32))*2654435761U)&0x7fffffff;
  }
};

template<> struct HashFunc<int64>
{
  static int Do(int64 k)
  {
    return int(((k&0xffffffff)+(k>>32))*2654435761U)&0x7fffffff;
  }
};

template<class T> struct HashFunc<T*>
{
  static int Do(T* ptr)
  {
    uint64 k=(uint64)ptr;
    return int(((k&0xffffffff)+(k>>32))*2654435761U)&0x7fffffff;
  }
};

template<> struct HashFunc<const char*>
{
  static int Do(const char* str)
  {
    // BKDRHash
    unsigned  int  seed  =   131;//  31 131 1313 13131 131313 etc...
    unsigned  int  hash  =   0 ;
    while( * str )
    {
      hash  =  hash  *  seed  +  ( * str ++ );
    } 
    return  (hash  &   0x7FFFFFFF );
  }
};

template<class T,class Traits,class Alloc> struct HashFunc<std::basic_string<T,Traits,Alloc> >
{
    static int Do(const std::basic_string<T,Traits,Alloc>& str)
    {
        return HashFunc<const char*>::Do(str.c_str());
    }
};

// ----------------------------------------------------
// radix tree，适用于稀疏数组，提供比hashmap更快的存取性能
// 但空间消耗较大，且其他操作不如hashmap,仅在及其必要时使用
// 从tcmalloc中拷贝，有改动
// ----------------------------------------------------

// Three-level radix tree
template <int BITS>
class TCMalloc_PageMap3 {
 private:
  // How many bits should we consume at each interior level
  static const int INTERIOR_BITS = (BITS + 2) / 3; // Round-up
  static const int INTERIOR_LENGTH = 1 << INTERIOR_BITS;

  // How many bits should we consume at leaf level
  static const int LEAF_BITS = BITS - 2*INTERIOR_BITS;
  static const int LEAF_LENGTH = 1 << LEAF_BITS;

  // Interior node
  struct Node {
    list_head lst;
    Node* ptrs[INTERIOR_LENGTH];
  };

  // Leaf node
  struct Leaf {
    list_head lst;
    void* values[LEAF_LENGTH];
  };

  Node* root_;                          // Root of radix tree
  void* (*allocator_)(size_t);          // Memory allocator
  void (*deallocator_)(void*);          // Memory deallocator
  list_head alloc_;

  Node* NewNode() {
    Node* result = reinterpret_cast<Node*>((*allocator_)(sizeof(Node)));
    if (result != NULL) {
      memset(result, 0, sizeof(*result));
      list_add(&result->lst,&alloc_);
    }
    return result;
  }
  //none copyable
  TCMalloc_PageMap3(const TCMalloc_PageMap3&);
  TCMalloc_PageMap3& operator=(const TCMalloc_PageMap3&);
 public:
  void Swap(TCMalloc_PageMap3<BITS>& other)
  {
      if(this!=&other)
      {
          Node* tmpRoot=root_;
          void* (*tmpAllocator)(size_t);
          tmpAllocator=allocator_;
          void (*tmpDeallocator)(void*);
          tmpDeallocator=deallocator_;
          LIST_HEAD(tmpAlloc);
          list_splice_init(&alloc_,&tmpAlloc);

          root_=other.root_;
          allocator_=other.allocator_;
          deallocator_=other.deallocator_;
          list_splice_init(&other.alloc_,&alloc_);

          other.root_=tmpRoot;
          other.allocator_=tmpAllocator;
          other.deallocator_=tmpDeallocator;
          list_splice_init(&tmpAlloc,&other.alloc_);
      }
  }

  //typedef uintptr_t Number;
  //BITS>=32而不是BITS>32，是因为 int32>>32 == int32 != 0
  typedef typename Cross::if_<BITS>=32,uint64,uintptr_t>::type Number;
  ~TCMalloc_PageMap3()
  {
      list_head* iter;
      list_head* next;
      list_for_each_safe(iter,next,&alloc_)
      {
          // iter就在Node和Leaf的开头
          (*deallocator_)(iter);
      }
      INIT_LIST_HEAD(&alloc_);
  }

  explicit TCMalloc_PageMap3(void* (*allocator)(size_t),void (*deallocator)(void*)) {
    allocator_ = allocator;
    deallocator_ = deallocator;
    INIT_LIST_HEAD(&alloc_);
    root_ = NewNode();
  }

  void* get(Number k) const {
    const Number i1 = k >> (LEAF_BITS + INTERIOR_BITS);
    const Number i2 = (k >> LEAF_BITS) & (INTERIOR_LENGTH-1);
    const Number i3 = k & (LEAF_LENGTH-1);
    if ((k >> BITS) > 0 ||
        root_->ptrs[i1] == NULL || root_->ptrs[i1]->ptrs[i2] == NULL) {
      return NULL;
    }
    return reinterpret_cast<Leaf*>(root_->ptrs[i1]->ptrs[i2])->values[i3];
  }

  void set(Number k, void* v) {
    assert(k >> BITS == 0);
    const Number i1 = k >> (LEAF_BITS + INTERIOR_BITS);
    const Number i2 = (k >> LEAF_BITS) & (INTERIOR_LENGTH-1);
    const Number i3 = k & (LEAF_LENGTH-1);
    if (root_->ptrs[i1] == NULL || root_->ptrs[i1]->ptrs[i2] == NULL) {
        Ensure(k,1);
    }
    reinterpret_cast<Leaf*>(root_->ptrs[i1]->ptrs[i2])->values[i3] = v;
  }

  bool Ensure(Number start, size_t n) {
    for (Number key = start; key <= start + n - 1; ) {
      const Number i1 = key >> (LEAF_BITS + INTERIOR_BITS);
      const Number i2 = (key >> LEAF_BITS) & (INTERIOR_LENGTH-1);

      // Check for overflow
      if (i1 >= INTERIOR_LENGTH || i2 >= INTERIOR_LENGTH)
        return false;

      // Make 2nd level node if necessary
      if (root_->ptrs[i1] == NULL) {
        Node* n = NewNode();
        if (n == NULL) return false;
        root_->ptrs[i1] = n;
      }

      // Make leaf node if necessary
      if (root_->ptrs[i1]->ptrs[i2] == NULL) {
        Leaf* leaf = reinterpret_cast<Leaf*>((*allocator_)(sizeof(Leaf)));
        if (leaf == NULL) return false;
        memset(leaf, 0, sizeof(*leaf));
        list_add(&leaf->lst,&alloc_);
        root_->ptrs[i1]->ptrs[i2] = reinterpret_cast<Node*>(leaf);
      }

      // Advance key past whatever is covered by this leaf node
      key = ((key >> LEAF_BITS) + 1) << LEAF_BITS;
    }
    return true;
  }

  void* Next(Number& k) const {
    while (k < (Number(1) << BITS)) {
      const Number i1 = k >> (LEAF_BITS + INTERIOR_BITS);
      const Number i2 = (k >> LEAF_BITS) & (INTERIOR_LENGTH-1);
      if (root_->ptrs[i1] == NULL) {
        // Advance to next top-level entry
        k = (i1 + 1) << (LEAF_BITS + INTERIOR_BITS);
      } else {
        Leaf* leaf = reinterpret_cast<Leaf*>(root_->ptrs[i1]->ptrs[i2]);
        if (leaf != NULL) {
          for (Number i3 = (k & (LEAF_LENGTH-1)); i3 < LEAF_LENGTH; i3++) {
            if (leaf->values[i3] != NULL) {
              k = (k & ~(LEAF_LENGTH-1)) | i3;
              assert(k == ((i1 << (LEAF_BITS + INTERIOR_BITS)) | (i2 << LEAF_BITS) | i3)); 
              return leaf->values[i3];
            }
          }
        }
        // Advance to next interior entry
        k = ((k >> LEAF_BITS) + 1) << LEAF_BITS;
      }
    }
    return NULL;
  }
};

// 支持32位整数的radix tree
template<class T>
class RadixTreeMap
{
    typedef TCMalloc_PageMap3<32> ImplType;
public:
    RadixTreeMap()
        :_map(malloc,free)
    {
    }
    void Set(uint32 key,T* v)
    {
        _map.set(key,v);
    }
    T* Get(uint32 key) const
    {
        return (T*)_map.get(key);
    }
    T* Next(uint32& key) const
    {
        ImplType::Number k=key;
        T* v=(T*)_map.Next(k);
        if(v)
            key=(uint32)k;
        return v;
    }
    //当内存膨胀有很多空内存块时，CopyTo和Swap一起可以达到“瘦身”效果
    void CopyTo(RadixTreeMap<T>& other)
    {
        if(this!=&other)
        {
            ImplType::Number k=0;
            void* v=_map.Next(k);
            while(v)
            {
                other._map.set(k,v);
                ++k;
                v=_map.Next(k);
            }
        }
    }
    void Swap(RadixTreeMap<T>& other)
    {
        _map.Swap(other._map);
    }
private:
    ImplType _map;
};

// 队列,优化过性能,从zeromq中拷贝，改了内存分配(用malloc对非平凡的class T不正确)
//  yqueue is an efficient queue implementation. The main goal is
//  to minimise number of allocations/deallocations needed. Thus yqueue
//  allocates/deallocates elements in batches of N.
//
//  yqueue allows one thread to use push/back function and another one 
//  to use pop/front functions. However, user must ensure that there's no
//  pop on the empty queue and that both threads don't access the same
//  element in unsynchronised manner.
//
//  T is the type of the object in the queue.
//  N is granularity of the queue (how many pushes have to be done till
//  actual memory allocation is required).
template <typename T, int N> class FifoQueue
{
public:
    //  Create the queue.
    inline FifoQueue ()
    {
        begin_chunk = new chunk_t;
        assert (begin_chunk);
        begin_pos = 0;
        back_chunk = NULL;
        back_pos = 0;
        end_chunk = begin_chunk;
        end_pos = 0;
    }
    //  Destroy the queue.
    ~FifoQueue ()
    {
        while (true) {
            if (begin_chunk == end_chunk) {
                delete begin_chunk;
                break;
            } 
            chunk_t *o = begin_chunk;
            begin_chunk = begin_chunk->next;
            delete o;
        }
        chunk_t *sc = (chunk_t*)spare_chunk.Exchange (NULL);
        if (sc)
            delete sc;
    }
    //  Returns reference to the front element of the queue.
    //  If the queue is empty, behaviour is undefined.
    inline T &front ()
    {
        return begin_chunk->values [begin_pos];
    }
    //  Returns reference to the back element of the queue.
    //  If the queue is empty, behaviour is undefined.
    inline T &back ()
    {
        return back_chunk->values [back_pos];
    }
    //  Adds an element to the back end of the queue.
    inline void push ()
    {
        back_chunk = end_chunk;
        back_pos = end_pos;
        if (++end_pos != N)
            return;
        chunk_t *sc = (chunk_t*)spare_chunk.Exchange (NULL);
        if (sc) {
            end_chunk->next = sc;
            sc->prev = end_chunk;
        } else {
            end_chunk->next = new chunk_t;
            assert (end_chunk->next);
            end_chunk->next->prev = end_chunk;
        }
        end_chunk = end_chunk->next;
        end_pos = 0;
    }
    //  Removes element from the back end of the queue. In other words
    //  it rollbacks last push to the queue. Take care: Caller is
    //  responsible for destroying the object being unpushed.
    //  The caller must also guarantee that the queue isn't empty when
    //  unpush is called. It cannot be done automatically as the read
    //  side of the queue can be managed by different, completely
    //  unsynchronised thread.
    inline void unpush ()
    {
        //  First, move 'back' one position backwards.
        if (back_pos)
            --back_pos;
        else {
            back_pos = N - 1;
            back_chunk = back_chunk->prev;
        }
        //  Now, move 'end' position backwards. Note that obsolete end chunk
        //  is not used as a spare chunk. The analysis shows that doing so
        //  would require delete and atomic operation per chunk deallocated
        //  instead of a simple delete.
        if (end_pos)
            --end_pos;
        else {
            end_pos = N - 1;
            end_chunk = end_chunk->prev;
            delete end_chunk->next;
            end_chunk->next = NULL;
        }
    }
    //  Removes an element from the front end of the queue.
    inline void pop ()
    {
        if (++ begin_pos == N) {
            chunk_t *o = begin_chunk;
            begin_chunk = begin_chunk->next;
            begin_chunk->prev = NULL;
            begin_pos = 0;
            //  'o' has been more recently used than spare_chunk,
            //  so for cache reasons we'll get rid of the spare and
            //  use 'o' as the spare.
            chunk_t *cs = (chunk_t*)spare_chunk.Exchange (o);
            if (cs)
                delete cs;
        }
    }
private:
    //  Individual memory chunk to hold N elements.
    struct chunk_t
    {
        T values [N];
        chunk_t *prev;
        chunk_t *next;
    };
    //  Back position may point to invalid memory if the queue is empty,
    //  while begin & end positions are always valid. Begin position is
    //  accessed exclusively be queue reader (front/pop), while back and
    //  end positions are accessed exclusively by queue writer (back/push).
    chunk_t *begin_chunk;
    int begin_pos;
    chunk_t *back_chunk;
    int back_pos;
    chunk_t *end_chunk;
    int end_pos;
    //  People are likely to produce and consume at similar rates.  In
    //  this scenario holding onto the most recently freed chunk saves
    //  us from having to call new/delete.
    Cross::AtomicPointer spare_chunk;
    //  Disable copying of yqueue.
    FifoQueue (const FifoQueue&);
    FifoQueue &operator = (const FifoQueue&);
};

//  单线程写单线程读的队列，有批量读写的优化,从zeromq中拷贝，无功能性改动
//    原注释假设reader会睡眠，而我们不假设reader线程会睡眠,也不需要修改代码，分析如下
//      考虑flush里面的c.SetWithoutAtomic操作与check_read里面的c.CompareAndSwap操作同时发生时
//        此时队列里是没有数据的,可以一直执行c.CompareAndSwap将r设为无效值(front或null)。除非writer的c.SetWithoutAtomic成功,reader的c.CompareAndSwap才会把r设为有效值
//      如果flush里面没有c.CompareAndSwap (w, f) != w这一句就直接c.SetWithoutAtomic
//        此时就无法判断队列里面到底有没有数据
//  Lock-free queue implementation.
//  Only a single thread can read from the pipe at any specific moment.
//  Only a single thread can write to the pipe at any specific moment.
//  T is the type of the object in the queue.
//  N is granularity of the pipe, i.e. how many items are needed to
//  perform next memory allocation.
template <typename T, int N> class SingleReaderWriterQueue
{
public:
    //  Initialises the pipe.
    inline SingleReaderWriterQueue ()
    {
        //  Insert terminator element into the queue.
        queue.push ();
        //  Let all the pointers to point to the terminator.
        //  (unless pipe is dead, in which case c is set to NULL).
        r = w = f = &queue.back ();
        c.SetWithoutAtomic (&queue.back ());
    }
    //  Write an item to the pipe.  Don't flush it yet. If incomplete is
    //  set to true the item is assumed to be continued by items
    //  subsequently written to the pipe. Incomplete items are never
    //  flushed down the stream.
    inline void write (const T &value_, bool incomplete_)
    {
        //  Place the value to the queue, add new terminator element.
        queue.back () = value_;
        queue.push ();
        //  Move the "flush up to here" poiter.
        if (!incomplete_)
            f = &queue.back ();
    }
    //  Pop an incomplete item from the pipe. Returns true is such
    //  item exists, false otherwise.
    inline bool unwrite (T *value_)
    {
        if (f == &queue.back ())
            return false;
        queue.unpush ();
        *value_ = queue.back ();
        return true;
    }
    //  Flush all the completed items into the pipe. Returns false if
    //  the reader thread is sleeping. In that case, caller is obliged to
    //  wake the reader up before using the pipe again.
    inline bool flush ()
    {
        //  If there are no un-flushed items, do nothing.
        if (w == f)
            return true;
        //  Try to set 'c' to 'f'.
        if (c.CompareAndSwap (w, f) != w) {
            //  Compare-and-swap was unseccessful because 'c' is NULL.
            //  This means that the reader is asleep. Therefore we don't
            //  care about thread-safeness and update c in non-atomic
            //  manner. We'll return false to let the caller know
            //  that reader is sleeping.
            c.SetWithoutAtomic (f);
            w = f;
            return false;
        }
        //  Reader is alive. Nothing special to do now. Just move
        //  the 'first un-flushed item' pointer to 'f'.
        w = f;
        return true;
    }
    //  Check whether item is available for reading.
    inline bool check_read ()
    {
        //  Was the value prefetched already? If so, return.
        if (&queue.front () != r && r)
                return true;
        //  There's no prefetched value, so let us prefetch more values.
        //  Prefetching is to simply retrieve the
        //  pointer from c in atomic fashion. If there are no
        //  items to prefetch, set c to NULL (using compare-and-swap).
        r = (T*)c.CompareAndSwap (&queue.front (), NULL);
        //  If there are no elements prefetched, exit.
        //  During pipe's lifetime r should never be NULL, however,
        //  it can happen during pipe shutdown when items
        //  are being deallocated.
        if (&queue.front () == r || !r)
            return false;
        //  There was at least one value prefetched.
        return true;
    }
    //  Reads an item from the pipe. Returns false if there is no value.
    //  available.
    inline bool read (T *value_)
    {
        //  Try to prefetch a value.
        if (!check_read ())
            return false;
        //  There was at least one value prefetched.
        //  Return it to the caller.
        *value_ = queue.front ();
        queue.pop ();
        return true;
    }
    //  Applies the function fn to the first elemenent in the pipe
    //  and returns the value returned by the fn.
    //  The pipe mustn't be empty or the function crashes.
    inline bool probe (bool (*fn)(T &))
    {
        bool rc = check_read ();
        assert (rc);
        return (*fn) (queue.front ());
    }
protected:
    //  Allocation-efficient queue to store pipe items.
    //  Front of the queue points to the first prefetched item, back of
    //  the pipe points to last un-flushed item. Front is used only by
    //  reader thread, while back is used only by writer thread.
    FifoQueue <T, N> queue;
    //  Points to the first un-flushed item. This variable is used
    //  exclusively by writer thread.
    T *w;
    //  Points to the first un-prefetched item. This variable is used
    //  exclusively by reader thread.
    T *r;
    //  Points to the first item to be flushed in the future.
    T *f;
    //  The single point of contention between writer and reader thread.
    //  Points past the last flushed item. If it is NULL,
    //  reader is asleep. This pointer should be always accessed using
    //  atomic operations.
    Cross::AtomicPointer c;
    //  Disable copying of ypipe object.
    SingleReaderWriterQueue (const SingleReaderWriterQueue&);
    SingleReaderWriterQueue &operator = (const SingleReaderWriterQueue&);
};



#if 0//SingleProducerConsumer的内部节点可能会增长到很多不好控制,无锁队列改用上面的zeromq实现
// 无锁队列,从raknet中拷贝
/// \brief A single producer consumer implementation without critical sections.
template <class T>
class SingleProducerConsumer
{
  static const int MINIMUM_LIST_SIZE=8;
public:
  /// Constructor
  SingleProducerConsumer();

  /// Destructor
  ~SingleProducerConsumer();

  /// WriteLock must be immediately followed by WriteUnlock.  These two functions must be called in the same thread.
  /// \return A pointer to a block of data you can write to.
  T* WriteLock(void);

  /// Call if you don't want to write to a block of data from WriteLock() after all.
  /// Cancelling locks cancels all locks back up to the data passed.  So if you lock twice and cancel using the first lock, the second lock is ignored
  /// \param[in] cancelToLocation Which WriteLock() to cancel.
  void CancelWriteLock(T* cancelToLocation);

  /// Call when you are done writing to a block of memory returned by WriteLock()
  void WriteUnlock(void);

  /// ReadLock must be immediately followed by ReadUnlock. These two functions must be called in the same thread.
  /// \retval 0 No data is availble to read
  /// \retval Non-zero The data previously written to, in another thread, by WriteLock followed by WriteUnlock.
  T* ReadLock(void);

  // Cancelling locks cancels all locks back up to the data passed.  So if you lock twice and cancel using the first lock, the second lock is ignored
  /// param[in] Which ReadLock() to cancel.
  void CancelReadLock(T* cancelToLocation);

  /// Signals that we are done reading the the data from the least recent call of ReadLock.
  /// At this point that pointer is no longer valid, and should no longer be read.
  void ReadUnlock(void);

  /// Clear is not thread-safe and none of the lock or unlock functions should be called while it is running.
  void Clear(void);

  /// This function will estimate how many elements are waiting to be read.  It's threadsafe enough that the value returned is stable, but not threadsafe enough to give accurate results.
  /// \return An ESTIMATE of how many data elements are waiting to be read
  int Size(void) const;

  /// Make sure that the pointer we done reading for the call to ReadUnlock is the right pointer.
  /// param[in] A previous pointer returned by ReadLock()
  bool CheckReadUnlockOrder(const T* data) const;

  /// Returns if ReadUnlock was called before ReadLock
  /// \return If the read is locked
  bool ReadIsLocked(void) const;

private:
  struct DataPlusPtr
  {
    DataPlusPtr () {readyToRead=false;}
    T object;

    // Ready to read is so we can use an equality boolean comparison, in case the writePointer var is trashed while context switching.
    volatile bool readyToRead;
    volatile DataPlusPtr *next;
  };
  volatile DataPlusPtr *readAheadPointer;
  volatile DataPlusPtr *writeAheadPointer;
  volatile DataPlusPtr *readPointer;
  volatile DataPlusPtr *writePointer;
  unsigned readCount, writeCount;
};

template <class T>
SingleProducerConsumer<T>::SingleProducerConsumer()
{
  // Preallocate
  readPointer = new DataPlusPtr;
  writePointer=readPointer;
  readPointer->next = new DataPlusPtr;
  int listSize;
#ifdef _DEBUG
  assert(MINIMUM_LIST_SIZE>=3);
#endif
  for (listSize=2; listSize < MINIMUM_LIST_SIZE; listSize++)
  {
    readPointer=readPointer->next;
    readPointer->next = new DataPlusPtr;
  }
  readPointer->next->next=writePointer; // last to next = start
  readPointer=writePointer;
  readAheadPointer=readPointer;
  writeAheadPointer=writePointer;
  readCount=writeCount=0;
}

template <class T>
SingleProducerConsumer<T>::~SingleProducerConsumer()
{
  volatile DataPlusPtr *next;
  readPointer=writeAheadPointer->next;
  while (readPointer!=writeAheadPointer)
  {
    next=readPointer->next;
    delete readPointer;
    readPointer=next;
  }
  delete readPointer;
}

template <class T>
T* SingleProducerConsumer<T>::WriteLock( void )
{
  if (writeAheadPointer->next==readPointer ||
    writeAheadPointer->next->readyToRead==true)
  {
    volatile DataPlusPtr *originalNext=writeAheadPointer->next;
    writeAheadPointer->next=new DataPlusPtr;
    assert(writeAheadPointer->next);
    writeAheadPointer->next->next=originalNext;
  }

  volatile DataPlusPtr *last;
  last=writeAheadPointer;
  writeAheadPointer=writeAheadPointer->next;

  return (T*) last;
}

template <class T>
void SingleProducerConsumer<T>::CancelWriteLock( T* cancelToLocation )
{
  writeAheadPointer=(DataPlusPtr *)cancelToLocation;
}

template <class T>
void SingleProducerConsumer<T>::WriteUnlock( void )
{
  //	DataPlusPtr *dataContainer = (DataPlusPtr *)structure;

#ifdef _DEBUG
  assert(writePointer->next!=readPointer);
  assert(writePointer!=writeAheadPointer);
#endif

  writeCount++;
  // User is done with the data, allow send by updating the write pointer
  writePointer->readyToRead=true;
  writePointer=writePointer->next;
}

template <class T>
T* SingleProducerConsumer<T>::ReadLock( void )
{
  if (readAheadPointer==writePointer ||
    readAheadPointer->readyToRead==false)
  {
    return 0;
  }

  volatile DataPlusPtr *last;
  last=readAheadPointer;
  readAheadPointer=readAheadPointer->next;
  return (T*)last;
}

template <class T>
void SingleProducerConsumer<T>::CancelReadLock( T* cancelToLocation )
{
#ifdef _DEBUG
  assert(readPointer!=writePointer);
#endif
  readAheadPointer=(DataPlusPtr *)cancelToLocation;
}

template <class T>
void SingleProducerConsumer<T>::ReadUnlock( void )
{
#ifdef _DEBUG
  assert(readAheadPointer!=readPointer); // If hits, then called ReadUnlock before ReadLock
  assert(readPointer!=writePointer); // If hits, then called ReadUnlock when Read returns 0
#endif
  readCount++;

  // Allow writes to this memory block
  readPointer->readyToRead=false;
  readPointer=readPointer->next;
}

template <class T>
void SingleProducerConsumer<T>::Clear( void )
{
  // Shrink the list down to MINIMUM_LIST_SIZE elements
  volatile DataPlusPtr *next;
  writePointer=readPointer->next;

  int listSize=1;
  next=readPointer->next;
  while (next!=readPointer)
  {
    listSize++;
    next=next->next;
  }

  while (listSize-- > MINIMUM_LIST_SIZE)
  {
    next=writePointer->next;
#ifdef _DEBUG
    assert(writePointer!=readPointer);
#endif
    delete writePointer;
    writePointer=next;
  }

  readPointer->next=writePointer;
  writePointer=readPointer;
  readAheadPointer=readPointer;
  writeAheadPointer=writePointer;
  readCount=writeCount=0;
}

template <class T>
int SingleProducerConsumer<T>::Size( void ) const
{
  return writeCount-readCount;
}
template <class T>
bool SingleProducerConsumer<T>::CheckReadUnlockOrder(const T* data) const
{
  return const_cast<const T *>(&readPointer->object) == data;
}
template <class T>
bool SingleProducerConsumer<T>::ReadIsLocked(void) const
{
  return readAheadPointer!=readPointer;
}

template<class T>
class SingleReaderWriterQueue
{
public:
  void Push(const T& d)
  {
    T* p=mImpl.WriteLock();
    *p=d;
    mImpl.WriteUnlock();
  }
  bool Pop(T& d)
  {
    T* p=mImpl.ReadLock();
    if(NULL==p)
    {
      // 这里不能Unlock
      return false;
    }
    d=*p;
    mImpl.ReadUnlock();
    return true;
  }
  bool PeekTop(T& d)
  {
    T* p=mImpl.ReadLock();
    if(NULL==p)
      return false;
    d=*p;
    mImpl.CancelReadLock(p);
    return true;
  }
  int Size() const{return mImpl.Size();}
private:
  SingleProducerConsumer<T> mImpl;
};
#endif//#if 0

// ------------------------------------------
// 基于Vector的Binary Heap实现,默认是最小堆
// ------------------------------------------

template<class T,class Cmp=std::less<T> >
class BinaryHeap
{
  typedef typename std::vector<T>::type Vec;
public:
  typedef typename Vec::iterator iterator;
  iterator begin() const{return mArray.begin();}
  iterator end() const{return mArray.end();}
  BinaryHeap(){}
  explicit BinaryHeap(const Vec& vec);
  explicit BinaryHeap(Cmp Less):mLess(Less){}
  explicit BinaryHeap(const Vec& vec,Cmp Less);
  void reserve(int n){mArray.reserve(n);}
  bool empty() const{return mArray.empty();}
  size_t size() const{return mArray.size();}
  void clear(){mArray.clear();}
  void insert(const T& x);
  const T& top() const;
  void pop();
  void pop(T* elem);
private:
  void build();
  void percolate_down(int index);
  int up_index(int index);
  int down_index(int index);
  Vec mArray;
  Cmp mLess;
};

template<class T,class Cmp>
inline BinaryHeap<T,Cmp>::BinaryHeap(const Vec& vec)
    :mArray(vec)
{
    build();
}
template<class T,class Cmp>
inline BinaryHeap<T,Cmp>::BinaryHeap(const Vec& vec,Cmp Less)
:mArray(vec)
,mLess(Less)
{
  build();
}

template<class T,class Cmp>
inline int BinaryHeap<T,Cmp>::up_index(int index)
{
  return (index+1)/2-1;
}

template<class T,class Cmp>
inline int BinaryHeap<T,Cmp>::down_index(int index)
{
  return (index+1)*2-1;
}

template<class T,class Cmp>
void BinaryHeap<T,Cmp>::insert(const T& x)
{
  // percolate up
  mArray.resize(mArray.size()+1);
  int index=mArray.size()-1;
  int upindex=up_index(index);
  for(;index>0;index=upindex,upindex=up_index(index))
  {
    assert(upindex>=0);
    if(mLess(x,mArray[upindex]))
      mArray[index]=mArray[upindex];
    else
      break;
  }
  mArray[index]=x;
}

template<class T,class Cmp>
inline const T& BinaryHeap<T,Cmp>::top() const
{
  assert(!mArray.empty());
  return mArray[0];
}

template<class T,class Cmp>
inline void BinaryHeap<T,Cmp>::pop()
{
  assert(!mArray.empty());
  mArray[0]=mArray.back();
  mArray.pop_back();
  percolate_down(0);
}

template<class T,class Cmp>
inline void BinaryHeap<T,Cmp>::pop(T* elem)
{
  assert(!mArray.empty());
  *elem=mArray[0];
  mArray[0]=mArray.back();
  mArray.pop_back();
  percolate_down(0);
}

template<class T,class Cmp>
void BinaryHeap<T,Cmp>::build()
{
  for(int i=up_index(mArray.size()-1);i>=0;--i)
  {
    percolate_down(i);
  }
}

template<class T,class Cmp>
void BinaryHeap<T,Cmp>::percolate_down(int index)
{
  assert(index>=0);
  if(index<(int)mArray.size())
  {
    int child=down_index(index);
    T tmp=mArray[index];
    for(;child<(int)mArray.size();index=child,child=down_index(index))
    {
      if(child!=mArray.size()-1&&mLess(mArray[child+1],mArray[child]))
        ++child;
      if(mLess(mArray[child],tmp))
        mArray[index]=mArray[child];
      else
        break;
    }
    assert(index<(int)mArray.size());
    mArray[index]=tmp;
  }
}

#endif//_CROSSENGINE_CONTAINER_H_
