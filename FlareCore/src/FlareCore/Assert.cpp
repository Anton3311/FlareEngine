#include "Assert.h"

#include "FlareCore/Log.h"

namespace Flare
{
	void LogCoreAssert(size_t line, const char* file, const char* message)
	{
		FLARE_CORE_ERROR("Assertion '{}' failed at {}:{}", message, file, line);
	}

	void LogAssert(size_t line, const char* file, const char* message)
	{
		FLARE_ERROR("Assertion '{}' failed at {}:{}", message, file, line);
	}

	void LogCoreVerify(size_t line, const char* file, const char* message)
	{
		FLARE_CORE_CRITICAL("Verification failed at {}:{}: {}", file, line, message);
	}

	void LogVerify(size_t line, const char* file, const char* message)
	{
		FLARE_CRITICAL("Verification failed at {}:{}: {}", file, line, message);
	}
}
