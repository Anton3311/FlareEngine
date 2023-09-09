#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/RenderData.h"

namespace Flare
{
	class FLARE_API DebugRenderer
	{
	public:
		static void Initialize(uint32_t maxLinesCount = 40000);
		static void Shutdown();

		static void Begin(const RenderData& renderData);
		static void End();

		static void FlushLines();

		static void DrawLine(const glm::vec3& start, const glm::vec3& end);
		static void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color);
		static void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& startColor, const glm::vec4& endColor);
	};
}