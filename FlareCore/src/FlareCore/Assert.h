#pragma once

#include "FlareCore/Core.h"

#if defined(FLARE_DEBUG) || defined(FLARE_RELEASE)
	#define FLARE_ENABLE_ASSERTIONS
#endif

#if defined(FLARE_DEBUG) || defined(FLARE_RELEASE)
	#define FLARE_ENABLE_VERIFY
#endif

namespace Flare
{
	FLARECORE_API void LogCoreAssert(size_t line, const char* file, const char* message);
	FLARECORE_API void LogAssert(size_t line, const char* file, const char* message);

	FLARECORE_API void LogCoreVerify(size_t line, const char* file, const char* message);
	FLARECORE_API void LogVerify(size_t line, const char* file, const char* message);
}

#if defined(FLARE_ENABLE_ASSERTIONS) || defined(FLARE_ENABLE_VERIFY)
	#define FLARE_ASSERT_IMPL(logger, condition, msg) { if (!(condition)) { logger(__LINE__, __FILE__, msg); FLARE_DEBUGBREAK; } }
	#define FLARE_ASSERT_IMPL_WITH_MSG(logger, condition, msg) FLARE_EXPEND_MACRO(FLARE_ASSERT_IMPL(logger, condition, msg))
	#define FLARE_ASSERT_IMPL_WITHOUT_MSG(logger, condition, ...) FLARE_EXPEND_MACRO(FLARE_ASSERT_IMPL(logger, condition, FALRE_STRINGIFY_MACRO(condition)))

	#define FLARE_GET_ASSERT_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define FLARE_GET_ASSERT_MACRO(...) FLARE_EXPEND_MACRO(FLARE_GET_ASSERT_MACRO_NAME(__VA_ARGS__, FLARE_ASSERT_IMPL_WITH_MSG, FLARE_ASSERT_IMPL_WITHOUT_MSG))
#endif

#ifdef FLARE_ENABLE_ASSERTIONS
	#define FLARE_CORE_ASSERT(...) FLARE_EXPEND_MACRO(FLARE_GET_ASSERT_MACRO(__VA_ARGS__)(Flare::LogCoreAssert, __VA_ARGS__));
	#define FLARE_ASSERT(...) FLARE_EXPEND_MACRO(FLARE_GET_ASSERT_MACRO(__VA_ARGS__)(Flare::LogAssert, __VA_ARGS__))
#else
	#define FLARE_CORE_ASSERT(...)
	#define FLARE_ASSERT(...)
#endif

#ifdef FLARE_ENABLE_VERIFY
	#define FLARE_CORE_VERIFY(...) FLARE_EXPEND_MACRO(FLARE_GET_ASSERT_MACRO(__VA_ARGS__)(Flare::LogCoreVerify, __VA_ARGS__));
	#define FLARE_CORE_VERIFY_UNREACHABLE() FLARE_CORE_VERIFY(false, "Reached an unreachable statement");

	#define FLARE_VERIFY(...) FLARE_EXPEND_MACRO(FLARE_GET_ASSERT_MACRO(__VA_ARGS__)(Flare::LogVerify, __VA_ARGS__));
	#define FLARE_VERIFY_UNREACHABLE() FLARE_VERIFY(false, "Reached an unreachable statement");
#else
	#define FLARE_CORE_VERIFY(...)
	#define FLARE_CORE_VERIFY_UNREACHABLE()

	#define FLARE_VERIFY(...)
	#define FLARE_VERIFY_UNREACHABLE()
#endif
