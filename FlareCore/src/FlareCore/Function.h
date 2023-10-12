#pragma once

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

	template<typename RetT, typename... ArgsT>
	struct FunctionImplementation
	{
		using ReturnType = RetT;

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
				delete m_Callable;
		}

		template<typename FuncT>
		void SetCallable(FuncT&& function)
		{
			if (m_Callable != nullptr)
				delete m_Callable;

			m_Callable = new Callable<FuncT, RetT, ArgsT...>(std::forward<FuncT>(function));
		}

		inline CallableBase<RetT, ArgsT...>& GetCallable()
		{
			return *m_Callable;
		}
	private:
		CallableBase<RetT, ArgsT...>* m_Callable;
	};

	template<typename T>
	constexpr bool AlwaysFalse = false;

	template<typename T>
	struct GetFunctionImplementation
	{
		using Type = void;

		static_assert(AlwaysFalse<T>);
	};

	template<typename RetT, typename... ArgsT>
	struct GetFunctionImplementation<RetT(ArgsT...)>
	{
		using Type = FunctionImplementation<RetT, ArgsT...>;
	};

	template<typename FunctionT>
	class Function
	{
	public:
		using Implementation = typename GetFunctionImplementation<FunctionT>::Type;
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