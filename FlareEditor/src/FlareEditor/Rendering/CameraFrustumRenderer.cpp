#include "PCH.h"

#include "CameraFrustumRenderer.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Scene/Components.h"
#include "Flare/Scene/Transform.h"

#include "Flare/Renderer/Viewport.h"

#include "Flare/DebugRenderer/DebugRenderer.h"

#include "FlareECS/World.h"

#include "FlareEditor/EditorLayer.h"

namespace Flare
{
	FLARE_IMPL_SYSTEM(CameraFrustumRenderer);

	void CameraFrustumRenderer::OnConfig(World& world, SystemConfig& config)
	{
		FLARE_PROFILE_FUNCTION();
		std::optional<uint32_t> groupId = world.GetSystemsManager().FindGroup("Debug Rendering");
		FLARE_CORE_ASSERT(groupId.has_value());
		config.Group = *groupId;

		m_Query = world.NewQuery().All().With<CameraComponent, TransformComponent>().Build();
	}

	void CameraFrustumRenderer::OnUpdate(World& world, SystemExecutionContext& context)
	{
		FLARE_PROFILE_FUNCTION();
		if (!EditorLayer::GetInstance().GetSceneViewSettings().ShowCameraFrustum)
			return;

		m_Query.ForEachChunk([](QueryChunk chunk, ComponentView<const TransformComponent> transforms, ComponentView<const CameraComponent> cameras)
			{
				for (QueryChunkEntity entity : chunk)
				{
					const TransformComponent& transform = transforms[entity];
					const CameraComponent& camera = cameras[entity];

					glm::mat4 transformationMatrix = transform.GetTransformationMatrix();
					glm::mat4 projectionMatrix = camera.GetProjection();

					glm::mat4 viewProjection = projectionMatrix * glm::inverse(transformationMatrix);
					DebugRenderer::DrawFrustum(glm::inverse(viewProjection), glm::vec4(1.0f));
				}
			});
	}
}
