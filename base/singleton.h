#ifndef _SINGLE_TON_H 
#define _SINGLE_TON_H

namespace base
{
template<typename T>
class SingleTon
{
public:
	static T* instance() 
	{ 
		if(!pInstance)
			pInstance=new T;
		return pInstance; 
	}
protected: 
	static T* pInstance;
	SingleTon(){}
	virtual ~SingleTon(){}
};

template<typename T>
T* SingleTon<T>::pInstance = nullptr;
}
#endif
