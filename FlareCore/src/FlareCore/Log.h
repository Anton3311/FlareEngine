#pragma once

#include "FlareCore/Core.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/bundled/format.h>
#include <glm/glm.hpp>

namespace Flare
{
    class FLARECORE_API Log
    {
    public:
        static void Initialize();

        static Ref<spdlog::logger> GetCoreLogger();
        static Ref<spdlog::logger> GetClientLogger();
    };
}

#define VECTOR_FORMATTER(vectorType, formatString, ...)                        \
    template<>                                                                 \
    struct fmt::formatter<vectorType> : fmt::formatter<vectorType::value_type> \
    {                                                                          \
        auto format(vectorType vector, format_context& ctx)                    \
        {                                                                      \
            return format_to(ctx.out(), formatString, __VA_ARGS__);            \
        }                                                                      \
    };

VECTOR_FORMATTER(glm::vec2, "Vector2({}; {})", vector.x, vector.y);
VECTOR_FORMATTER(glm::ivec2, "Vector2Int({}; {})", vector.x, vector.y);
VECTOR_FORMATTER(glm::uvec2, "Vector2UInt({}; {})", vector.x, vector.y);

VECTOR_FORMATTER(glm::vec3, "Vector3({}; {}; {})", vector.x, vector.y, vector.z);
VECTOR_FORMATTER(glm::ivec3, "Vector3Int({}; {}; {})", vector.x, vector.y, vector.z);
VECTOR_FORMATTER(glm::uvec3, "Vector3UInt({}; {}; {})", vector.x, vector.y, vector.z);

VECTOR_FORMATTER(glm::vec4, "Vector4({}; {}; {}; {})", vector.x, vector.y, vector.z, vector.w);
VECTOR_FORMATTER(glm::ivec4, "Vector4Int({}; {}; {}; {})", vector.x, vector.y, vector.z, vector.w);
VECTOR_FORMATTER(glm::uvec4, "Vector4UInt({}; {}; {}; {})", vector.x, vector.y, vector.z, vector.w);

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
