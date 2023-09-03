#include "Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace Flare
{
	Ref<spdlog::logger> s_CoreLogger;
	Ref<spdlog::logger> s_ClientLogger;

	void Log::Initialize()
	{
		spdlog::set_pattern("%^[%T] %n:%$ %v");

		s_CoreLogger = spdlog::stdout_color_mt("FLARE");
		s_ClientLogger = spdlog::stdout_color_mt("FLARE_CLIENT");

		s_CoreLogger->set_level(spdlog::level::level_enum::trace);
		s_ClientLogger->set_level(spdlog::level::level_enum::trace);
	}

	Ref<spdlog::logger> Log::GetCoreLogger()
	{
		return s_CoreLogger;
	}

	Ref<spdlog::logger> Log::GetClientLogger()
	{
		return s_ClientLogger;
	}
}