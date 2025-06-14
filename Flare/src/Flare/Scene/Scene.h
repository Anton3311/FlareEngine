#pragma once

#include "FlareCore/Core.h"
#include "Flare/AssetManager/Asset.h"

#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/Font.h"
#include "Flare/Renderer/PostProcessing/PostProcessingManager.h"

#include "Flare/Scene/SceneRenderer.h"

#include "FlareECS/World.h"
#include "FlareECS/System/SystemsManager.h"

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

		void InitializeRuntime();
	public:
		void OnRuntimeStart();
		void OnRuntimeEnd();

		void OnUpdateRuntime();
		void OnUpdate();

		void OnFrameStart();

		inline World& GetECSWorld() { return m_World; }
		inline const World& GetECSWorld() const { return m_World; }

		inline SystemsManager& GetECSSystemsManager() { return m_SystemsManager; }
		inline const SystemsManager& GetECSSystemsManager() const { return m_SystemsManager; }

		inline PostProcessingManager& GetPostProcessingManager() { return m_PostProcessingManager; }
		inline const PostProcessingManager& GetPostProcessingManager() const { return m_PostProcessingManager; }

		inline Query GetRootEntitiesQuery() const { return m_RootEntitiesQuery; }

		static Ref<Scene> GetActive();
		static void SetActive(const Ref<Scene>& scene);
	private:
		void Initialize();
		void UpdateEnvironmentSettings();
	private:
		World m_World;
		SystemsManager m_SystemsManager;

		SystemGroupId m_RenderingGroup;
		SystemGroupId m_ScriptingUpdateGroup;
		SystemGroupId m_LateUpdateGroup;
		SystemGroupId m_OnRuntimeStartGroup;
		SystemGroupId m_OnRuntimeEndGroup;

		SystemGroupId m_SceneHierarchyUpdate;

		SystemGroupId m_OnFrameStart;
		SystemGroupId m_OnFrameEnd;

		Query m_EnvironmentQuery;
		Query m_RootEntitiesQuery;

		PostProcessingManager m_PostProcessingManager;
	private:
		friend SceneSerializer;
	};
}