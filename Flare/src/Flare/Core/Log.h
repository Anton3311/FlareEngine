#pragma once

#include "Flare/Core/Core.h"

#include <spdlog/spdlog.h>

namespace Flare
{
	class Log
	{
	public:
		static void Initialize();
		
		static Ref<spdlog::logger> GetCoreLogger() { return s_CoreLogger; }
		static Ref<spdlog::logger> GetClientLogger() { return s_ClientLogger; }
	private:
		static Ref<spdlog::logger> s_CoreLogger;
		static Ref<spdlog::logger> s_ClientLogger;
	};
}

#define FLARE_CORE_ERROR(...) Flare::Log::GetCoreLogger()->error(__VA_ARGS__)
#define FLARE_CORE_WARN(...) Flare::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define FLARE_CORE_INFO(...) Flare::Log::GetCoreLogger()->info(__VA_ARGS__)
#define FLARE_CORE_TRACE(...) Flare::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define FLARE_CORE_CRITICAL(...) Flare::Log::GetCoreLogger()->critical(__VA_ARGS__)

#define FLARE_ERROR(...) Flare::Log::GetClientLogger()->error(__VA_ARGS__)
#define FLARE_WARN(...) Flare::Log::GetClientLogger()->warn(__VA_ARGS__)
#define FLARE_INFO(...) Flare::Log::GetClientLogger()->info(__VA_ARGS__)
#define FLARE_TRACE(...) Flare::Log::GetClientLogger()->trace(__VA_ARGS__)
#define FLARE_CRITICAL(...) Flare::Log::GetClientLogger()->critical(__VA_ARGS__)
