#include "PCH.h"

#include "AABBVisualizer.h"

#include "Flare/Scene/Components.h"
#include "Flare/Scene/Transform.h"
#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/RendererPrimitives.h"

#include "Flare/DebugRenderer/DebugRenderer.h"

#include "FlareECS/World.h"
#include "FlareECS/System/SystemsManager.h"

#include "FlareEditor/EditorLayer.h"

namespace Flare
{
	FLARE_IMPL_SYSTEM(AABBVisualizer);

	void AABBVisualizer::OnConfig(World& world, SystemConfig& config)
	{
		FLARE_PROFILE_FUNCTION();
		m_MeshRendererQuery = world.NewQuery()
			.All()
			.With<TransformComponent>()
			.With<MeshRenderer>()
			.Build();
		m_DecalsQuery = world.NewQuery()
			.All()
			.With<TransformComponent>()
			.With<Decal>()
			.Build();

		std::optional<uint32_t> groupId = config.SystemsManager.FindGroup("Debug Rendering");
		FLARE_CORE_ASSERT(groupId.has_value());
		config.Group = *groupId;
	}

	void AABBVisualizer::OnUpdate(World& world, SystemExecutionContext& context)
	{
		FLARE_PROFILE_FUNCTION();
		if (!EditorLayer::GetInstance().GetSceneViewSettings().ShowAABBs)
			return;

		m_MeshRendererQuery.ForEachChunk([](QueryChunk chunk,
			ComponentView<const MeshRenderer> meshes,
			ComponentView<const TransformComponent> transforms)
			{
				for (size_t entityIndex = 0; entityIndex < chunk.GetEntityCount(); entityIndex++)
				{
					glm::mat4 transform = transforms[entityIndex].GetTransformationMatrix();

					if (meshes[entityIndex].Mesh == nullptr)
						continue;

					Math::AABB meshBounds = meshes[entityIndex].Mesh->GetBounds();
					DebugRenderer::DrawAABB(meshBounds.Transformed(transform));
				}
			});

		Math::AABB cubeAABB = RendererPrimitives::GetCube()->GetBounds();
		m_DecalsQuery.ForEachChunk([&cubeAABB](QueryChunk chunk,
			ComponentView<const TransformComponent> transforms,
			ComponentView<const Decal> decals)
			{
				for (size_t entityIndex = 0; entityIndex < chunk.GetEntityCount(); entityIndex++)
				{
					DebugRenderer::DrawAABB(cubeAABB.Transformed(transforms[entityIndex].GetTransformationMatrix()));
				}
			});
	}
}
