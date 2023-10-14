#include "LightVisualizer.h"

#include "FlareECS/World.h"

#include "Flare/Scene/Components.h"
#include "Flare/Renderer/DebugRenderer.h"

namespace Flare
{
	FLARE_IMPL_SYSTEM(LightVisualizer);

	void LightVisualizer::OnConfig(SystemConfig& config)
	{
		config.Group = World::GetCurrent().GetSystemsManager().FindGroup("Debug Rendering");

		m_Query = World::GetCurrent().CreateQuery<With<TransformComponent>, With<DirectionalLight>>();
	}

	void LightVisualizer::OnUpdate(SystemExecutionContext& context)
	{
		for (EntityView view : m_Query)
		{
			auto transforms = view.View<TransformComponent>();
			auto lights = view.View<DirectionalLight>();

			for (EntityViewElement entity : view)
				DebugRenderer::DrawRay(transforms[entity].Position, transforms[entity].TransformDirection(glm::vec3(0.0f, 0.0f, -1.0f)));
		}
	}
}
