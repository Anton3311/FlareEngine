#pragma once

#include "Flare/Core/Log.h"
#include "Flare/Core/Core.h"

#include <filesystem>

#ifdef FLARE_DEBUG
	#define FLARE_ASSERT_IMPL(type, condition, msg, ...) { if (!(condition)) { FLARE##type##ERROR(msg, __VA_ARGS__); FLARE_DEBUGBREAK; } }
	#define FLARE_ASSERT_IMPL_WITH_MSG(type, condition, ...) FLARE_EXPEND_MACRO(FLARE_ASSERT_IMPL(type, condition, "Assertion failed: {0}", __VA_ARGS__))
	#define FLARE_ASSERT_IMPL_WITHOUT_MSG(type, condition, ...) FLARE_EXPEND_MACRO(FLARE_ASSERT_IMPL(type, condition, "Assertion '{0}' failed at {1}:{2}", FALRE_STRINGIFY_MACRO(condition), std::filesystem::path(__FILE__).filename().string(), __LINE__))

	#define FLARE_GET_ASSERT_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define FLARE_GET_ASSERT_MACRO(...) FLARE_EXPEND_MACRO(FLARE_GET_ASSERT_MACRO_NAME(__VA_ARGS__, FLARE_ASSERT_IMPL_WITH_MSG, FLARE_ASSERT_IMPL_WITHOUT_MSG))

	#define FLARE_CORE_ASSERT(...) FLARE_EXPEND_MACRO(FLARE_GET_ASSERT_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__));
	#define FLARE_ASSERT(...) FLARE_EXPEND_MACRO(FLARE_GET_ASSERT_MACRO(__VA_ARGS__)(_, __VA_ARGS__))
#else
	#define FLARE_CORE_ASSERT(...)
	#define FLARE_ASSERT(...)
#endif