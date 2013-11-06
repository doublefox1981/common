#ifndef _SCOPE_GUARD_H
#define _SCOPE_GUARD_H

#include <functional>
namespace base
{
	class ScopeGuard
	{
	public:
		explicit ScopeGuard(std::function<void ()> onExitScope):_onExitScope(onExitScope),_dismissed(false)
		{}
		~ScopeGuard()
		{
			if(!_dismissed)
			{
				_onExitScope();
			}
		}
		void Dismiss()
		{
			_dismissed=true;
		}
	private:
		std::function<void ()> _onExitScope;
		bool _dismissed;
	private:
		ScopeGuard(const ScopeGuard&);
		ScopeGuard& operator=(const ScopeGuard&);
	};
};

#endif