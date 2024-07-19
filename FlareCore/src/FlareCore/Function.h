#pragma once

#include <utility>

namespace Flare
{
	template<typename RetT, typename... ArgsT>
	struct CallableBase
	{
		virtual RetT operator()(ArgsT&& ...args) = 0;
		virtual ~CallableBase() = default;
	};

	template <typename FuncT, typename RetT, typename... ArgsT>
	struct Callable : CallableBase<RetT, ArgsT...>
	{
		Callable(FuncT&& functor)
			: m_Functor(std::move(functor)) {}

		virtual RetT operator()(ArgsT&& ...args) override
		{
			return m_Functor(std::forward<ArgsT>(args)...);
		}
	private:
		FuncT m_Functor;
	};

	struct DefaultFunctionAllocator
	{
		template<typename T, typename... ArgsT>
		static T* Allocate(ArgsT&& ...args)
		{
			return new T(std::forward<ArgsT>(args)...);
		}

		template<typename T>
		static void Release(T* callable)
		{
			delete callable;
		}
	};

	template<typename AllocatorT, typename RetT, typename... ArgsT>
	struct FunctionImplementation
	{
		using ReturnType = RetT;
		using CallableType = CallableBase<RetT, ArgsT...>;

		FunctionImplementation()
			: m_Callable(nullptr) {}

		FunctionImplementation(FunctionImplementation&& other) noexcept
			: m_Callable(other.m_Callable)
		{
			other.m_Callable = nullptr;
		}

		~FunctionImplementation()
		{
			if (m_Callable != nullptr)
			{
				AllocatorT::Release<CallableType>(m_Callable);
				m_Callable = nullptr;
			}
		}

		template<typename FuncT>
		void SetCallable(FuncT&& function)
		{
			if (m_Callable != nullptr)
				AllocatorT::Release<CallableType>(m_Callable);

			m_Callable = AllocatorT::Allocate<Callable<FuncT, RetT, ArgsT...>>(std::forward<FuncT>(function));
		}

		inline CallableBase<RetT, ArgsT...>& GetCallable()
		{
			return *m_Callable;
		}
	private:
		CallableType* m_Callable;
	};

	template<typename T>
	constexpr bool AlwaysFalse = false;

	template<typename AllocatorT, typename T>
	struct GetFunctionImplementation
	{
		using Type = void;

		static_assert(AlwaysFalse<T>);
	};

	template<typename AllocatorT, typename RetT, typename... ArgsT>
	struct GetFunctionImplementation<AllocatorT, RetT(ArgsT...)>
	{
		using Type = FunctionImplementation<AllocatorT, RetT, ArgsT...>;
	};

	template<typename FunctionT, typename AllocatorT = DefaultFunctionAllocator>
	class Function
	{
	public:
		using Implementation = typename GetFunctionImplementation<AllocatorT, FunctionT>::Type;
		using ReturnType = typename Implementation::ReturnType;

		Function() {}

		Function(Function<FunctionT>&& other) noexcept
			: m_Impl(std::move(other.m_Impl)) {}

		Function(const Function<FunctionT>&) = delete;

		template <typename F>
		Function(F&& function)
		{
			m_Impl.SetCallable<F>(std::move(function));
		}

		Function<FunctionT> operator=(const Function<FunctionT>&) = delete;
		Function<FunctionT> operator=(Function<FunctionT>&& other)
		{
			m_Impl = std::move(other.m_Impl);
			return *this;
		}

		template<typename F>
		Function<FunctionT> operator=(F&& callable)
		{
			m_Impl.SetCallable<F>(std::move(callable));
			return *this;
		}

		template<typename... Args>
		ReturnType operator()(Args&& ...args)
		{
			return m_Impl.GetCallable()(std::forward<Args>(args)...);
		}
	private:
		Implementation m_Impl;
	};
}