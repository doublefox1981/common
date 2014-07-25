#ifndef _SCOPE_GUARD_H
#define _SCOPE_GUARD_H

#include <functional>
namespace base
{
	class ScopeGuard
	{
	public:
		explicit ScopeGuard(std::function<void ()> onExitScope):on_exit_scope_(onExitScope),dismissed_(false)
		{}
		~ScopeGuard()
		{
			if(!dismissed_)
			{
				on_exit_scope_();
			}
		}
		void dismiss()
		{
			dismissed_=true;
		}
	private:
		std::function<void ()> on_exit_scope_;
		bool dismissed_;
	private:
		ScopeGuard(const ScopeGuard&);
		ScopeGuard& operator=(const ScopeGuard&);
	};
};

#endif