#pragma once

#include "FlareECS/Entity/Entity.h"

#include "FlareEditor/SceneViewSettings.h"

#include "FlareEditor/UI/AssetEditor.h"
#include "FlareEditor/UI/ECS/EntitiesHierarchy.h"
#include "FlareEditor/UI/ECS/EntityProperties.h"
#include "FlareEditor/UI/SceneViewportWindow.h"

namespace Flare
{
	struct ECSContext;
	class Prefab;
	class Scene;
	class SceneRenderer;
	class World;
	class PrefabEditor : public AssetEditor
	{
	public:
		PrefabEditor(ECSContext& context);

		virtual void OnAttach() override;
		virtual void OnEvent(Event& event) override;
	protected:
		virtual void OnOpen(AssetHandle asset) override;
		virtual void OnClose() override;
		virtual void OnRenderImGui(bool& show) override;
	private:
		World& GetWorld();
	private:
		Ref<Scene> m_PreviewScene;

		Scope<SceneRenderer> m_SceneRenderer = nullptr;

		SceneViewSettings m_SceneViewSettings;
		
		SceneViewportWindow m_ViewportWindow;

		EntitiesHierarchy m_Entities;
		EntityProperties m_Properties;
		Ref<Prefab> m_Prefab;

		Entity m_SelectedEntity;
	};
}