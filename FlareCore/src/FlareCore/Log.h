#pragma once

#include "FlareCore/Core.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/bundled/format.h>
#include <glm/glm.hpp>

#include <locale>

namespace Flare
{
    class FLARECORE_API Log
    {
    public:
        static void Initialize();

        static std::shared_ptr<spdlog::logger> GetCoreLogger();
        static std::shared_ptr<spdlog::logger> GetClientLogger();
    };
}

template<typename T>
struct fmt::formatter<glm::vec<4, T>> : fmt::formatter<T>
{
    using VectorT = glm::vec<4, T>;
    auto format(VectorT vector, format_context& ctx) const
    {
        return fmt::format_to(ctx.out(), "({}, {}, {}, {})", vector.x, vector.y, vector.z, vector.w);
    }
};

template<typename T>
struct fmt::formatter<glm::vec<3, T>> : fmt::formatter<T>
{
    using VectorT = glm::vec<3, T>;
    auto format(VectorT vector, format_context& ctx) const
    {
        return fmt::format_to(ctx.out(), "({}, {}, {})", vector.x, vector.y, vector.z);
    }
};

template<typename T>
struct fmt::formatter<glm::vec<2, T>> : fmt::formatter<T>
{
    using VectorT = glm::vec<2, T>;
    auto format(VectorT vector, format_context& ctx) const
    {
        return fmt::format_to(ctx.out(), "({}, {})", vector.x, vector.y);
    }
};

template<typename T>
struct fmt::formatter<glm::vec<1, T>> : fmt::formatter<T>
{
    using VectorT = glm::vec<1, T>;
    auto format(VectorT vector, format_context& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", vector.x);
    }
};

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

