#ifndef _delegate_h_
#define _delegate_h_

#include "common/type.h"
#include "common/stl/vector.h"


namespace egal
{
	template <typename R>
	class TDelegate;

	template <typename T>
	class TDelegate
	{
	private:
		typedef void* InstancePtr;
		typedef T (*InternalFunction)(InstancePtr);
		struct Stub
		{
			InstancePtr first;
			InternalFunction second;
		};

		template <T (*Function)()> static T FunctionStub(InstancePtr)
		{
			return (Function)();
		}

		template <class C, T (C::*Function)()>
		static T ClassMethodStub(InstancePtr instance)
		{
			return (static_cast<C*>(instance)->*Function)();
		}

		template <class C, T (C::*Function)() const>
		static T ClassMethodStub(InstancePtr instance)
		{
			return (static_cast<C*>(instance)->*Function)();
		}

	public:
		TDelegate()
		{
			m_stub.first = nullptr;
			m_stub.second = nullptr;
		}

		template <T (*Function)()>
		void bind()
		{
			m_stub.first = nullptr;
			m_stub.second = &FunctionStub<Function>;
		}

		template <class C, T (C::*Function)()>
		void bind(C* instance)
		{
			m_stub.first = instance;
			m_stub.second = &ClassMethodStub<C, Function>;
		}

		template <class C, T (C::*Function)() const>
		void bind(C* instance)
		{
			m_stub.first = instance;
			m_stub.second = &ClassMethodStub<C, Function>;
		}

		T invoke() const
		{
			ASSERT(m_stub.second != nullptr);
			return m_stub.second(m_stub.first);
		}

		bool operator==(const TDelegate<T>& rhs)
		{
			return m_stub.first == rhs.m_stub.first && m_stub.second == rhs.m_stub.second;
		}

	private:
		Stub m_stub;
	};

	template <typename T, typename... Args>
	class TDelegate<T (Args...)>
	{
	private:
		typedef void* InstancePtr;
		typedef T(*InternalFunction)(InstancePtr, Args...);
		struct Stub
		{
			InstancePtr first;
			InternalFunction second;
		};

		template <T (*Function)(Args...)>
		static T FunctionStub(InstancePtr, Args... args)
		{
			return (Function)(args...);
		}

		template <class C, T (C::*Function)(Args...)>
		static T ClassMethodStub(InstancePtr instance, Args... args)
		{
			return (static_cast<C*>(instance)->*Function)(args...);
		}

		template <class C, T (C::*Function)(Args...) const>
		static T ClassMethodStub(InstancePtr instance, Args... args)
		{
			return (static_cast<C*>(instance)->*Function)(args...);
		}

	public:
		TDelegate()
		{
			m_stub.first = nullptr;
			m_stub.second = nullptr;
		}

		bool isValid()
		{
			return m_stub.second != nullptr;
		}

		template <T (*Function)(Args...)>
		void bind()
		{
			m_stub.first = nullptr;
			m_stub.second = &FunctionStub<Function>;
		}

		template <class C, T (C::*Function)(Args...)>
		void bind(C* instance)
		{
			m_stub.first = instance;
			m_stub.second = &ClassMethodStub<C, Function>;
		}

		template <class C, T (C::*Function)(Args...) const>
		void bind(C* instance)
		{
			m_stub.first = instance;
			m_stub.second = &ClassMethodStub<C, Function>;
		}

		T invoke(Args... args) const
		{
			ASSERT(m_stub.second != nullptr);
			return m_stub.second(m_stub.first, args...);
		}

		bool operator==(const TDelegate<T (Args...)>& rhs)
		{
			return m_stub.first == rhs.m_stub.first && m_stub.second == rhs.m_stub.second;
		}

	private:
		Stub m_stub;
	};

	//---------------------------------------------------------------------------------
	template <typename R>
	class TDelegateList;

	template <typename T, typename... Args>
	class TDelegateList<T (Args...)>
	{
	public:
		explicit TDelegateList(IAllocator& allocator)
			: m_delegates(allocator)
		{
		}

		template <typename C, T (C::*Function)(Args...)>
		void bind(C* instance)
		{
			TDelegate<T (Args...)> cb;
			cb.template bind<C, Function>(instance);
			m_delegates.push(cb);
		}

		template <T (*Function)(Args...)>
		void bind()
		{
			TDelegate<T (Args...)> cb;
			cb.template bind<Function>();
			m_delegates.push(cb);
		}

		template <typename C, T (C::*Function)(Args...)>
		void unbind(C* instance)
		{
			TDelegate<T (Args...)> cb;
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
			for (auto& i : m_delegates)
				i.invoke(args...);
		}

	private:
		TVector<TDelegate<T (Args...)>> m_delegates;
	};

}
#endif