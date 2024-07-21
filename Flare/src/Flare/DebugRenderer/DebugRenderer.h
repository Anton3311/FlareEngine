#pragma once

#include "FlareCore/Core.h"
#include "Flare/Math/Math.h"

namespace Flare
{
	struct SceneSubmition;

	class Viewport;
	class FLARE_API DebugRenderer
	{
	public:
		static void Initialize();
		static void Shutdown();

		static void BeginScene(SceneSubmition& sceneSubmition);
		static void EndScene();

		static void DrawLine(const glm::vec3& start, const glm::vec3& end);
		static void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color);
		static void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& startColor, const glm::vec4& endColor);

		static void DrawRay(const glm::vec3& origin, const glm::vec3& direction, const glm::vec4& color = glm::vec4(1.0f));

		static void DrawCircle(const glm::vec3& position,
			const glm::vec3& normal,
			const glm::vec3& tangent,
			float radius,
			const glm::vec4& color = glm::vec4(1.0f),
			uint32_t segments = 16);

		static void DrawWireSphere(const glm::vec3& origin,
			float radius,
			const glm::vec4& color = glm::vec4(1.0f),
			uint32_t segments = 16);

		static void DrawWireQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color = glm::vec4(1.0f));
		static void DrawWireQuad(const glm::vec3* corners, const glm::vec4& color = glm::vec4(1.0f));
		static void DrawFrustum(const glm::mat4& inverseViewProjection, const glm::vec4& color = glm::vec4(1.0f));
		static void DrawWireBox(const glm::vec3 corners[8], const glm::vec4& color = glm::vec4(1.0f));
		static void DrawAABB(const Math::AABB& aabb, const glm::vec4& color = glm::vec4(1.0f));

		static void ConfigurePasses(Viewport& viewport);
	};
}