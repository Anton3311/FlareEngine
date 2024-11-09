#include "PCH.h"

#include "Scene.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/PostProcessing/ToneMapping.h"
#include "Flare/Renderer/PostProcessing/Vignette.h"
#include "Flare/Renderer/PostProcessing/SSAO.h"
#include "Flare/Renderer/PostProcessing/Atmosphere.h"

#include "Flare/Math/Math.h"
#include "Flare/Scene/Components.h"
#include "Flare/Scene/Hierarchy.h"

namespace Flare
{
	Ref<Scene> s_Active = nullptr;

	FLARE_SERIALIZABLE_IMPL(Scene);
	FLARE_IMPL_ASSET(Scene);

	Scene::Scene(ECSContext& context)
		: Asset(AssetType::Scene), m_World(context), m_SystemsManager(m_World, context.SystemsRegistry)
	{
		FLARE_PROFILE_FUNCTION();
		m_World.MakeCurrent();
		Initialize();
	}

	Scene::~Scene()
	{
	}

	void Scene::Initialize()
	{
		FLARE_PROFILE_FUNCTION();
		m_SystemsManager.CreateGroup("Debug Rendering");

		m_RenderingGroup = m_SystemsManager.CreateGroup("Rendering");
		m_ScriptingUpdateGroup = m_SystemsManager.CreateGroup("Scripting Update");
		m_LateUpdateGroup = m_SystemsManager.CreateGroup("Late Update");
		m_OnRuntimeStartGroup = m_SystemsManager.CreateGroup("On Runtime Start");
		m_OnRuntimeEndGroup = m_SystemsManager.CreateGroup("On Runtime End");

		m_SceneHierarchyUpdate = m_SystemsManager.CreateGroup("SceneHierarchyUpdate");

		m_SystemsManager.SetDefaultSystemsGroup(m_ScriptingUpdateGroup);

		m_OnFrameStart = m_SystemsManager.CreateGroup("On Frame End");
		m_OnFrameEnd = m_SystemsManager.CreateGroup("On Frame End");

		m_EnvironmentQuery = m_World.NewQuery().All().With<Environment>().Build();

		m_PostProcessingManager.AddEffect(Ref<SSAO>::New());
		m_PostProcessingManager.AddEffect(Ref<Atmosphere>::New());
		m_PostProcessingManager.AddEffect(Ref<Vignette>::New());
		m_PostProcessingManager.AddEffect(Ref<ToneMapping>::New());
	}

	void Scene::InitializeRuntime()
	{
		FLARE_PROFILE_FUNCTION();
		m_SystemsManager.RegisterSystems();
		m_SystemsManager.RebuildExecutionGraphs();
	}

	void Scene::OnRuntimeStart()
	{
		FLARE_PROFILE_FUNCTION();
		m_SystemsManager.ExecuteGroup(m_OnRuntimeStartGroup);
	}

	void Scene::OnRuntimeEnd()
	{
		FLARE_PROFILE_FUNCTION();
		m_SystemsManager.ExecuteGroup(m_OnRuntimeEndGroup);
	}

	void Scene::OnUpdateRuntime()
	{
		FLARE_PROFILE_FUNCTION();
		m_SystemsManager.ExecuteGroup(m_OnFrameStart);
		m_SystemsManager.ExecuteGroup(m_ScriptingUpdateGroup);
		m_SystemsManager.ExecuteGroup(m_LateUpdateGroup);
		m_SystemsManager.ExecuteGroup(m_OnFrameEnd);
	}

	void Scene::OnUpdate()
	{
		FLARE_PROFILE_FUNCTION();
		m_SystemsManager.ExecuteGroup(m_SceneHierarchyUpdate);

		m_World.Entities.ClearQueuedForDeletion();
		m_World.Entities.ClearCreatedEntitiesQueryResult();

		UpdateEnvironmentSettings();
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
	}

	Ref<Scene> Scene::GetActive()
	{
		return s_Active;
	}

	void Scene::SetActive(const Ref<Scene>& scene)
	{
		s_Active = scene;
	}

	void Scene::UpdateEnvironmentSettings()
	{
		FLARE_PROFILE_FUNCTION();

		m_EnvironmentQuery.ForEachChunk([](QueryChunk chunk, ComponentView<const Environment> environments)
			{
				for (size_t entityIndex = 0; entityIndex < chunk.GetEntityCount(); entityIndex++)
				{
					Renderer::SetShadowSettings(environments[entityIndex].ShadowSettings);
					return;
				}
			});
	}
}