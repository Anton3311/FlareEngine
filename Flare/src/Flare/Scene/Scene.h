#pragma once

#include "FlareCore/Core.h"
#include "Flare/AssetManager/Asset.h"

#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/Viewport.h"
#include "Flare/Renderer/Font.h"
#include "Flare/Renderer/PostProcessing/PostProcessingManager.h"

#include "Flare/Scene/SceneRenderer.h"

#include "FlareECS.h"

namespace Flare
{
	class SceneSerializer;
	class FLARE_API Scene : public Asset
	{
	public:
		FLARE_ASSET;
		FLARE_SERIALIZABLE;

		Scene(ECSContext& context);
		~Scene();

		void Initialize();
		void InitializeRuntime();

		void InitializePostProcessing();
		void UninitializePostProcessing();
	public:
		void OnRuntimeStart();
		void OnRuntimeEnd();

		void OnBeforeRender(Viewport& viewport);
		void OnRender(const Viewport& viewport);

		void OnUpdateRuntime();
		void OnUpdateEditor();
		void OnViewportResize(uint32_t width, uint32_t height);

		World& GetECSWorld();
		inline PostProcessingManager& GetPostProcessingManager() { return m_PostProcessingManager; }
		inline const PostProcessingManager& GetPostProcessingManager() const { return m_PostProcessingManager; }

		static Ref<Scene> GetActive();
		static void SetActive(const Ref<Scene>& scene);
	private:
		World m_World;

		SystemGroupId m_2DRenderingGroup;
		SystemGroupId m_ScriptingUpdateGroup;
		SystemGroupId m_LateUpdateGroup;
		SystemGroupId m_OnRuntimeStartGroup;
		SystemGroupId m_OnRuntimeEndGroup;

		SystemGroupId m_OnFrameStart;
		SystemGroupId m_OnFrameEnd;

		Query m_CameraDataUpdateQuery;
		Query m_DirectionalLightQuery;
		Query m_EnvironmentQuery;
		Query m_PointLightsQuery;
		Query m_SpotLightsQuery;

		PostProcessingManager m_PostProcessingManager;
	private:
		friend SceneSerializer;
	};
}