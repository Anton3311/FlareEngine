#include "AABBVisualizer.h"

#include "Flare/Scene/Components.h"
#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/DebugRenderer.h"

#include "FlareECS/World.h"

#include "FlareEditor/EditorLayer.h"

namespace Flare
{
	FLARE_IMPL_SYSTEM(AABBVisualizer);

	void AABBVisualizer::OnConfig(World& world, SystemConfig& config)
	{
		m_Query = world.NewQuery()
			.With<TransformComponent>()
			.With<MeshComponent>()
			.Create();

		config.Group = world.GetSystemsManager().FindGroup("Debug Rendering");
	}

	void AABBVisualizer::OnUpdate(World& world, SystemExecutionContext& context)
	{
		if (!EditorLayer::GetInstance().GetSceneViewSettings().ShowAABBs)
			return;

		for (EntityView view : m_Query)
		{
			auto transforms = view.View<TransformComponent>();
			auto meshes = view.View<MeshComponent>();

			for (EntityViewElement entity : view)
			{
				glm::mat4 transform = transforms[entity].GetTransformationMatrix();

				AssetHandle meshHandle = meshes[entity].Mesh;
				
				Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(meshHandle);
				if (mesh != nullptr)
				{
					const Math::AABB& bounds = mesh->GetSubMesh().Bounds;

					glm::vec3 corners[8];
					bounds.GetCorners(corners);

					Math::AABB newBounds;
					for (size_t i = 0; i < 8; i++)
					{
						glm::vec3 transformed = transform * glm::vec4(corners[i], 1.0f);
						if (i == 0)
						{
							newBounds.Min = transformed;
							newBounds.Max = transformed;
						}
						else
						{
							newBounds.Min = glm::min(newBounds.Min, transformed);
							newBounds.Max = glm::max(newBounds.Max, transformed);
						}
					}

					DebugRenderer::DrawAABB(newBounds);
				}
			}
		}
	}
}
