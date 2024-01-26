#include "LightVisualizer.h"

#include "FlareECS/World.h"

#include "Flare/Scene/Components.h"
#include "Flare/Renderer/DebugRenderer.h"
#include "Flare/Renderer/ShaderLibrary.h"

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
		config.Group = world.GetSystemsManager().FindGroup("Debug Rendering");
		m_DirectionalLightQuery = world.NewQuery().With<TransformComponent, DirectionalLight>().Create();
		m_PointLightsQuery = world.NewQuery().With<TransformComponent, PointLight>().Create();
		m_SpotlightsQuery = world.NewQuery().With<TransformComponent, SpotLight>().Create();
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

		if (!m_HasProjectOpenHandler)
		{
			Project::OnProjectOpen.Bind(FLARE_BIND_EVENT_CALLBACK(ReloadShaders));
			ReloadShaders();

			m_HasProjectOpenHandler = true;
		}

		if (!m_DebugIconsMaterial)
			return;

		Renderer2D::SetMaterial(m_DebugIconsMaterial);

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
				position = cameraView * glm::vec4(position, 1.0f);

				Renderer2D::DrawQuad(position,
					glm::vec2(1.0f),
					glm::vec4(lights[entity].Color, 1.0f),
					iconsTexture,
					glm::vec2(pointLightIconUVs.Min.x, pointLightIconUVs.Max.y),
					glm::vec2(pointLightIconUVs.Max.x, pointLightIconUVs.Min.y)
				);
			}
		}

		for (EntityView view : m_SpotlightsQuery)
		{
			auto transforms = view.View<TransformComponent>();
			auto lights = view.View<SpotLight>();

			for (EntityViewElement entity : view)
			{
				glm::vec3 position = transforms[entity].Position;
				position = cameraView * glm::vec4(position, 1.0f);

				Renderer2D::DrawQuad(position,
					glm::vec2(1.0f),
					glm::vec4(lights[entity].Color, 1.0f),
					iconsTexture,
					glm::vec2(spotlightIconUVs.Min.x, spotlightIconUVs.Max.y),
					glm::vec2(spotlightIconUVs.Max.x, spotlightIconUVs.Min.y)
				);
			}
		}

		Renderer2D::SetMaterial(nullptr);
	}

	void LightVisualizer::ReloadShaders()
	{
		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("DebugIcon");

		if (!shaderHandle || !AssetManager::IsAssetHandleValid(shaderHandle.value()))
			FLARE_CORE_ERROR("LightVisualizer: Failed to find DebugIcon shader");
		else
		{
			Ref<Shader> shader = AssetManager::GetAsset<Shader>(*shaderHandle);
			m_DebugIconsMaterial = CreateRef<Material>(shader);
		}
	}
}
