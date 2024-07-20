#pragma once

#include "Flare/Scene/Prefab.h"
#include "Flare/Scene/Scene.h"

#include "FlareECS/World.h"

#include "FlareEditor/UI/AssetEditor.h"
#include "FlareEditor/UI/ECS/EntitiesHierarchy.h"
#include "FlareEditor/UI/ECS/EntityProperties.h"

#include "FlareEditor/UI/SceneViewportWindow.h"

namespace Flare
{
	class SceneRenderer;
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
		inline World& GetWorld() { return m_PreviewScene->GetECSWorld(); }
	private:
		Ref<Scene> m_PreviewScene;

		Scope<SceneRenderer> m_SceneRenderer = nullptr;
		
		EditorCamera m_EditorCamera;
		SceneViewportWindow m_ViewportWindow;

		EntitiesHierarchy m_Entities;
		EntityProperties m_Properties;
		Ref<Prefab> m_Prefab;

		Entity m_SelectedEntity;
	};
}