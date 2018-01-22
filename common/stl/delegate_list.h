#ifndef _delegate_list_h_
#define _delegate_list_h_
#pragma once

#if EGAL_USER_STL == 1
 #include "common/stl/tarrary.h"
#else
 #include "common/template.h"
#endif

#include "common/stl/delegate.h"

namespace egal
{
	template <typename T> class TDelegateList;

	template <typename R, typename... Args> class TDelegateList<R(Args...)>
	{
	public:
		explicit TDelegateList(IAllocator& allocator)
			: m_delegates(allocator)
		{
		}

		template <typename C, R(C::*Function)(Args...)> void bind(C* instance)
		{
			TDelegate<R(Args...)> cb;
			cb.template bind<C, Function>(instance);
			m_delegates.push_back(cb);
		}

		template <R(*Function)(Args...)> void bind()
		{
			TDelegate<R(Args...)> cb;
			cb.template bind<Function>();
			m_delegates.push_back(cb);
		}

		template <typename C, R(C::*Function)(Args...)> void unbind(C* instance)
		{
			TDelegate<R(Args...)> cb;
			cb.template bind<C, Function>(instance);
			for (int i = 0; i < m_delegates.size(); ++i)
			{
				if (m_delegates[i] == cb)
				{
					m_delegates.eraseFast(i);
					break;
				}
			}
		}

		void invoke(Args... args)
		{
			for (auto& i : m_delegates) i.invoke(args...);
		}

	private:
		TArrary<TDelegate<R(Args...)>> m_delegates;
	};
}
#endif