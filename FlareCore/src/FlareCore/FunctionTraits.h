#pragma once

namespace Flare
{
	template<typename... Args>
	struct ArgumentsList {};

	template<typename T>
	struct FirstArgument
	{
		using Type = void;
	};

	template<typename FirstArg, typename... Args>
	struct FirstArgument<ArgumentsList<FirstArg, Args...>>
	{
		using Type = FirstArg;
	};

	template<typename F>
	struct FunctionTraitsImpl
	{
		static constexpr size_t ArgumentsCount = 0;
		using ReturnType = void;
		using Arguments = ArgumentsList<>;
	};

	template<typename ReturnT, typename... Args>
	struct FunctionTraitsImpl<ReturnT(Args...)>
	{
		static constexpr size_t ArgumentsCount = sizeof...(Args);
		using ReturnType = ReturnT;
		using Arguments = ArgumentsList<Args...>;
	};

	template<typename ReturnT, typename... Args>
	struct FunctionTraitsImpl<ReturnT(Args...) const> : FunctionTraitsImpl<ReturnT(Args...)> {};

	template<typename ReturnT, typename... Args>
	struct FunctionTraitsImpl<ReturnT(*)(Args...)> : FunctionTraitsImpl<ReturnT(Args...)> {};

	template<typename ClassT, typename ReturnT, typename... Args>
	struct FunctionTraitsImpl<ReturnT(ClassT::*)(Args...)> : FunctionTraitsImpl<ReturnT(Args...)> {};

	template<typename ClassT, typename ReturnT, typename... Args>
	struct FunctionTraitsImpl<ReturnT(ClassT::*)(Args...) const> : FunctionTraitsImpl<ReturnT(Args...)> {};

	template<typename F, typename V = void>
	struct FunctionTraits : FunctionTraitsImpl<F> {};

	template<typename F>
	struct FunctionTraits<F, decltype((void)&F::operator())> : FunctionTraitsImpl<decltype(&F::operator())> {};
}
