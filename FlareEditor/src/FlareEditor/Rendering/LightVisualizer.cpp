#include "LightVisualizer.h"

#include "FlareECS/World.h"

#include "Flare/Scene/Components.h"
#include "Flare/Scene/Transform.h"

#include "Flare/Renderer/ShaderLibrary.h"

#include "Flare/DebugRenderer/DebugRenderer.h"

#include "Flare/Project/Project.h"

#include "FlarePlatform/Event.h"

#include "FlareEditor/EditorLayer.h"
#include "FlareEditor/UI/EditorGUI.h"
#include "FlareEditor/UI/EditorIcons.h"

namespace Flare
{
	FLARE_IMPL_SYSTEM(LightVisualizer);

	void LightVisualizer::OnConfig(World& world, SystemConfig& config)
	{
		std::optional<uint32_t> groupId = world.GetSystemsManager().FindGroup("Debug Rendering");
		FLARE_CORE_ASSERT(groupId.has_value());
		config.Group = *groupId;

		m_DirectionalLightQuery = world.NewQuery().All().With<TransformComponent, DirectionalLight>().Build();
		m_PointLightsQuery = world.NewQuery().All().With<TransformComponent, PointLight>().Build();
		m_SpotlightsQuery = world.NewQuery().All().With<TransformComponent, SpotLight>().Build();
	}

	void LightVisualizer::OnUpdate(World& world, SystemExecutionContext& context)
	{
		if (!EditorLayer::GetInstance().GetSceneViewSettings().ShowLights)
			return;

		for (EntityView view : m_DirectionalLightQuery)
		{
			auto transforms = view.View<TransformComponent>();
			auto lights = view.View<DirectionalLight>();

			for (EntityViewElement entity : view)
				DebugRenderer::DrawRay(transforms[entity].Position, transforms[entity].TransformDirection(glm::vec3(0.0f, 0.0f, -1.0f)));
		}

		if (!m_HasProjectOpenHandler && RendererAPI::GetAPI() != RendererAPI::API::Vulkan)
		{
			Project::OnProjectOpen.Bind(FLARE_BIND_EVENT_CALLBACK(ReloadShaders));
			ReloadShaders();

			m_HasProjectOpenHandler = true;
		}

		if (RendererAPI::GetAPI() != RendererAPI::API::Vulkan)
		{
			if (!m_DebugIconsMaterial)
				return;
			Renderer2D::SetMaterial(m_DebugIconsMaterial);
		}

		ImRect pointLightIconUVs = EditorGUI::GetIcons().GetIconUVs(EditorIcons::PointLightIcon);
		ImRect spotlightIconUVs = EditorGUI::GetIcons().GetIconUVs(EditorIcons::SpotlightIcon);
		const Ref<Texture>& iconsTexture = EditorGUI::GetIcons().GetTexture();

		const glm::mat4& cameraView = Renderer::GetCurrentViewport().FrameData.Camera.View;
		for (EntityView view : m_PointLightsQuery)
		{
			auto transforms = view.View<TransformComponent>();
			auto lights = view.View<PointLight>();

			for (EntityViewElement entity : view)
			{
				glm::vec3 position = transforms[entity].Position;
				glm::vec3 iconPosition = cameraView * glm::vec4(position, 1.0f);

				const float intensityLimit = 0.1f;
				float radius = glm::sqrt(lights[entity].Intensity / intensityLimit);

				DebugRenderer::DrawWireSphere(position, radius, glm::vec4(lights[entity].Color, 1.0f));

				if (RendererAPI::GetAPI() != RendererAPI::API::Vulkan)
				{
					Renderer2D::DrawQuad(iconPosition,
						glm::vec2(1.0f),
						glm::vec4(lights[entity].Color, 1.0f),
						iconsTexture,
						glm::vec2(pointLightIconUVs.Min.x, pointLightIconUVs.Max.y),
						glm::vec2(pointLightIconUVs.Max.x, pointLightIconUVs.Min.y)
					);
				}
			}
		}

		for (EntityView view : m_SpotlightsQuery)
		{
			auto transforms = view.View<TransformComponent>();
			auto lights = view.View<SpotLight>();

			for (EntityViewElement entity : view)
			{
				glm::vec3 iconPosition = transforms[entity].Position;
				iconPosition = cameraView * glm::vec4(iconPosition, 1.0f);

				if (RendererAPI::GetAPI() != RendererAPI::API::Vulkan)
				{
					Renderer2D::DrawQuad(iconPosition,
						glm::vec2(1.0f),
						glm::vec4(lights[entity].Color, 1.0f),
						iconsTexture,
						glm::vec2(spotlightIconUVs.Min.x, spotlightIconUVs.Max.y),
						glm::vec2(spotlightIconUVs.Max.x, spotlightIconUVs.Min.y)
					);
				}

				glm::vec3 lightDirection = transforms[entity].TransformDirection(glm::vec3(0.0f, 0.0f, 1.0f));
				glm::vec3 tangent = transforms[entity].TransformDirection(glm::vec3(1.0f, 0.0f, 0.0f));
				glm::vec3 bitangent = glm::cross(lightDirection, tangent);

				const float intensityLimit = 0.1f;
				float radius = lights[entity].Intensity / intensityLimit;
				radius = glm::sqrt(radius);

				float outerCircleRadius = radius * glm::tan(glm::radians(lights[entity].OuterAngle / 2.0f));
				float innerCircleRadius = radius * glm::tan(glm::radians(lights[entity].InnerAngle / 2.0f));

				DebugRenderer::DrawCircle(transforms[entity].Position + lightDirection * radius,
					lightDirection,
					tangent,
					outerCircleRadius,
					glm::vec4(lights[entity].Color, 1.0f));
				DebugRenderer::DrawCircle(transforms[entity].Position + lightDirection * radius,
					lightDirection,
					tangent,
					innerCircleRadius,
					glm::vec4(lights[entity].Color, 1.0f));

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
					DebugRenderer::DrawLine(transforms[entity].Position,
						transforms[entity].Position + lightDirection * radius + offset * outerCircleRadius,
						glm::vec4(lights[entity].Color, 1.0f));
				}
			}
		}

		if (RendererAPI::GetAPI() != RendererAPI::API::Vulkan)
		{
			Renderer2D::SetMaterial(nullptr);
		}
	}

	void LightVisualizer::ReloadShaders()
	{
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
