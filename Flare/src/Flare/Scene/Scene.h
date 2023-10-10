#pragma once

#include "FlareCore/Core.h"
#include "Flare/AssetManager/Asset.h"

#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/Viewport.h"
#include "Flare/Renderer/Font.h"

#include "Flare/Scene/SceneRenderer.h"

#include "FlareECS.h"

namespace Flare
{
	class SceneSerializer;
	class FLARE_API Scene : public Asset
	{
	public:
		Scene(ECSContext& context);
		~Scene();

		void Initialize();
		void InitializeRuntime();
	public:
		void OnRuntimeStart();
		void OnRuntimeEnd();

		void OnBeforeRender(Viewport& viewport);
		void OnRender(const Viewport& viewport);

		void OnUpdateRuntime();
		void OnViewportResize(uint32_t width, uint32_t height);

		World& GetECSWorld();

		static Ref<Scene> GetActive();
		static void SetActive(const Ref<Scene>& scene);
	private:
		World m_World;

		SystemGroupId m_2DRenderingGroup;
		SystemGroupId m_ScriptingUpdateGroup;
		SystemGroupId m_OnRuntimeStartGroup;
		SystemGroupId m_OnRuntimeEndGroup;

		Query m_CameraDataUpdateQuery;
	private:
		friend SceneSerializer;
	};
}