#ifndef _delegate_h_
#define _delegate_h_
#pragma once

namespace egal
{
	template <typename T> class TDelegate;
	template <typename R> class TDelegate
	{
	private:
		typedef void* InstancePtr;
		typedef R(*InternalFunction)(InstancePtr);
		struct Stub
		{
			InstancePtr first;
			InternalFunction second;
		};

		template <R(*Function)()> static __forceinline R FunctionStub(InstancePtr) { return (Function)(); }

		template <class C, R(C::*Function)()> static __forceinline R ClassMethodStub(InstancePtr instance)
		{
			return (static_cast<C*>(instance)->*Function)();
		}

		template <class C, R(C::*Function)() const> static __forceinline R ClassMethodStub(InstancePtr instance)
		{
			return (static_cast<C*>(instance)->*Function)();
		}

	public:
		TDelegate()
		{
			m_stub.first = nullptr;
			m_stub.second = nullptr;
		}

		template <R(*Function)()> void bind()
		{
			m_stub.first = nullptr;
			m_stub.second = &FunctionStub<Function>;
		}

		template <class C, R(C::*Function)()> void bind(C* instance)
		{
			m_stub.first = instance;
			m_stub.second = &ClassMethodStub<C, Function>;
		}

		template <class C, R(C::*Function)() const> void bind(C* instance)
		{
			m_stub.first = instance;
			m_stub.second = &ClassMethodStub<C, Function>;
		}

		R invoke() const
		{
			ASSERT(m_stub.second != nullptr);
			return m_stub.second(m_stub.first);
		}

		bool operator==(const TDelegate<R>& rhs)
		{
			return m_stub.first == rhs.m_stub.first && m_stub.second == rhs.m_stub.second;
		}

	private:
		Stub m_stub;
	};


	template <typename R, typename... Args> class TDelegate<R(Args...)>
	{
	private:
		typedef void* InstancePtr;
		typedef R(*InternalFunction)(InstancePtr, Args...);
		struct Stub
		{
			InstancePtr first;
			InternalFunction second;
		};

		template <R(*Function)(Args...)> static __forceinline R FunctionStub(InstancePtr, Args... args)
		{
			return (Function)(args...);
		}

		template <class C, R(C::*Function)(Args...)>
		static __forceinline R ClassMethodStub(InstancePtr instance, Args... args)
		{
			return (static_cast<C*>(instance)->*Function)(args...);
		}

		template <class C, R(C::*Function)(Args...) const>
		static __forceinline R ClassMethodStub(InstancePtr instance, Args... args)
		{
			return (static_cast<C*>(instance)->*Function)(args...);
		}

	public:
		TDelegate()
		{
			m_stub.first = nullptr;
			m_stub.second = nullptr;
		}

		bool isValid() { return m_stub.second != nullptr; }

		template <R(*Function)(Args...)> void bind()
		{
			m_stub.first = nullptr;
			m_stub.second = &FunctionStub<Function>;
		}

		template <class C, R(C::*Function)(Args...)> void bind(C* instance)
		{
			m_stub.first = instance;
			m_stub.second = &ClassMethodStub<C, Function>;
		}

		template <class C, R(C::*Function)(Args...) const> void bind(C* instance)
		{
			m_stub.first = instance;
			m_stub.second = &ClassMethodStub<C, Function>;
		}

		R invoke(Args... args) const
		{
			ASSERT(m_stub.second != nullptr);
			return m_stub.second(m_stub.first, args...);
		}

		bool operator==(const TDelegate<R(Args...)>& rhs)
		{
			return m_stub.first == rhs.m_stub.first && m_stub.second == rhs.m_stub.second;
		}

	private:
		Stub m_stub;
	};
}
#endif