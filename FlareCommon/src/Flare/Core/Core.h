#pragma once

#include <memory>
#include <xhash>

#ifdef _WIN32
	#ifdef _WIN64
		#define FLARE_PLATFORM_WINDOWS
	#else
		#error "x86 platform is not supported"
	#endif
#endif

#ifdef FLARE_DEBUG
	#ifdef FLARE_PLATFORM_WINDOWS
		#define FLARE_DEBUGBREAK __debugbreak()
	#else
		#define FLARE_DEBUGBREAK
	#endif
#else
	#define FLARE_DEBUGBREAK
#endif

#define FLARE_EXPEND_MACRO(a) a
#define FALRE_STRINGIFY_MACRO(a) #a

#define HAS_BIT(value, bit) ((value & bit) != 0)

namespace Flare
{
	template<typename T>
	using Ref = std::shared_ptr<T>;

	template<typename T>
	using Scope = std::unique_ptr<T>;

	template<typename T, typename ...Args>
	constexpr Ref<T> CreateRef(Args&&... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template<typename T, typename F>
	constexpr Ref<T> As(const Ref<F>& ref)
	{
		return std::static_pointer_cast<T>(ref);
	}

	template<typename T, typename ...Args>
	constexpr Scope<T> CreateScope(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	void CombineHashes(size_t& seed, const T& value)
	{
		std::hash<T> hasher;
		seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}
}