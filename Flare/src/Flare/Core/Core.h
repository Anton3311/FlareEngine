#pragma once

#include <Flare/Core/Platform.h>

#include <memory>

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

	template<typename T, typename ...Args>
	constexpr Scope<T> CreateScope(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}
}