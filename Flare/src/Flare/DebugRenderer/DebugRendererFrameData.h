#pragma once

#include "FlareCore/Collections/Span.h"

#include "Flare/Math/Math.h"

#include <vector>
#include <glm/glm.hpp>

namespace Flare
{
	struct DebugRendererSettings
	{
		static constexpr uint32_t VerticesPerLine = 2;
		static constexpr uint32_t VerticesPerRay = 7;
		static constexpr uint32_t IndicesPerRay = 9;

		uint32_t MaxLines = 0;
		uint32_t MaxRays = 0;

		float RayThickness = 0.0f;
	};

	struct DebugRayData
	{
		glm::vec3 Origin = glm::vec3(0.0f);
		glm::vec3 Direction = glm::vec3(0.0f);
		glm::vec4 Color = glm::vec4(1.0f);
	};

	struct DebugRendererFrameData
	{
		struct Vertex
		{
			glm::vec3 Position = glm::vec3(0.0f);
			glm::vec4 Color = glm::vec4(1.0f);
		};

		Span<Vertex> LineVertices;
		std::vector<DebugRayData> Rays;
	};
}