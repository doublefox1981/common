#ifndef _SINGLE_TON_H 
#define _SINGLE_TON_H

namespace base
{
template<typename T>
class ezSingleTon
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
	ezSingleTon(){}
	virtual ~ezSingleTon(){}
};

template<typename T>
T* ezSingleTon<T>::pInstance = NULL;
}
#endif
