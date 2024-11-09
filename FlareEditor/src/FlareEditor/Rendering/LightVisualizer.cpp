#include "PCH.h"

#include "LightVisualizer.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/World.h"
#include "FlareECS/System/SystemsManager.h"

#include "Flare/Scene/Components.h"
#include "Flare/Scene/Transform.h"
#include "Flare/Project/Project.h"

#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/DebugRenderer/DebugRenderer.h"

#include "FlarePlatform/Event.h"

#include "FlareEditor/EditorLayer.h"
#include "FlareEditor/UI/EditorGUI.h"
#include "FlareEditor/UI/EditorIcons.h"

namespace Flare
{
	FLARE_IMPL_SYSTEM(LightVisualizer);

	void LightVisualizer::OnConfig(World& world, SystemConfig& config)
	{
		FLARE_PROFILE_FUNCTION();
		std::optional<uint32_t> groupId = config.SystemsManager.FindGroup("Debug Rendering");
		FLARE_CORE_ASSERT(groupId.has_value());
		config.Group = *groupId;

		m_DirectionalLightQuery = world.NewQuery().All().With<TransformComponent, DirectionalLight>().Build();
		m_PointLightsQuery = world.NewQuery().All().With<TransformComponent, PointLight>().Build();
		m_SpotlightsQuery = world.NewQuery().All().With<TransformComponent, SpotLight>().Build();
	}

	void LightVisualizer::OnUpdate(World& world, SystemExecutionContext& context)
	{
		FLARE_PROFILE_FUNCTION();
		if (!EditorLayer::GetInstance().GetSceneViewSettings().ShowLights)
			return;

		m_DirectionalLightQuery.ForEachChunk([](QueryChunk chunk, ComponentView<const TransformComponent> transforms, ComponentView<const DirectionalLight> lights)
			{
				for (size_t entityIndex = 0; entityIndex < chunk.GetEntityCount(); entityIndex++)
					DebugRenderer::DrawRay(transforms[entityIndex].Position, transforms[entityIndex].TransformDirection(glm::vec3(0.0f, 0.0f, -1.0f)));
			});

		if (!m_HasProjectOpenHandler && RendererAPI::GetAPI() != RendererAPI::API::Vulkan)
		{
			Project::OnProjectOpen.Bind(FLARE_BIND_EVENT_CALLBACK(ReloadShaders));
			ReloadShaders();

			m_HasProjectOpenHandler = true;
		}

		m_PointLightsQuery.ForEachChunk([](QueryChunk chunk, ComponentView<const TransformComponent> transforms, ComponentView<const PointLight> lights)
			{
				for (size_t entityIndex = 0; entityIndex < chunk.GetEntityCount(); entityIndex++)
				{
					glm::vec3 position = transforms[entityIndex].Position;

					const float intensityLimit = 0.1f;
					float radius = glm::sqrt(lights[entityIndex].Intensity / intensityLimit);

					DebugRenderer::DrawWireSphere(position, radius, glm::vec4(lights[entityIndex].Color, 1.0f));
				}
			});

		m_SpotlightsQuery.ForEachChunk([](QueryChunk chunk, ComponentView<const TransformComponent> transforms, ComponentView<const SpotLight> lights)
			{
				for (size_t entityIndex = 0; entityIndex < chunk.GetEntityCount(); entityIndex++)
				{
					glm::vec3 iconPosition = transforms[entityIndex].Position;

					glm::vec3 lightDirection = transforms[entityIndex].TransformDirection(glm::vec3(0.0f, 0.0f, 1.0f));
					glm::vec3 tangent = transforms[entityIndex].TransformDirection(glm::vec3(1.0f, 0.0f, 0.0f));
					glm::vec3 bitangent = glm::cross(lightDirection, tangent);

					const float intensityLimit = 0.1f;
					float radius = lights[entityIndex].Intensity / intensityLimit;
					radius = glm::sqrt(radius);

					float outerCircleRadius = radius * glm::tan(glm::radians(lights[entityIndex].OuterAngle));
					float innerCircleRadius = radius * glm::tan(glm::radians(lights[entityIndex].InnerAngle));

					DebugRenderer::DrawCircle(transforms[entityIndex].Position + lightDirection * radius,
						lightDirection,
						tangent,
						outerCircleRadius,
						glm::vec4(lights[entityIndex].Color, 1.0f));
					DebugRenderer::DrawCircle(transforms[entityIndex].Position + lightDirection * radius,
						lightDirection,
						tangent,
						innerCircleRadius,
						glm::vec4(lights[entityIndex].Color, 1.0f));

					glm::vec2 offsetSigns[] =
					{
						glm::vec2(1, 0),
						glm::vec2(0, 1),
						glm::vec2(-1, 0),
						glm::vec2(0, -1)
					};

					for (size_t i = 0; i < 4; i++)
					{
						glm::vec3 offset = offsetSigns[i].x * tangent + offsetSigns[i].y * bitangent;
						DebugRenderer::DrawLine(transforms[entityIndex].Position,
							transforms[entityIndex].Position + lightDirection * radius + offset * outerCircleRadius,
							glm::vec4(lights[entityIndex].Color, 1.0f));
					}
				}
			});
	}

	void LightVisualizer::ReloadShaders()
	{
		FLARE_PROFILE_FUNCTION();
		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("DebugIcon");

		if (!shaderHandle || !AssetManager::IsAssetHandleValid(shaderHandle.value()))
			FLARE_CORE_ERROR("LightVisualizer: Failed to find DebugIcon shader");
		else
		{
			Ref<Shader> shader = AssetManager::GetAsset<Shader>(*shaderHandle);
			m_DebugIconsMaterial = Material::Create(shader);
		}
	}
}
