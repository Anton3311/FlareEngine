#pragma once

#include "Flare.h"
#include "Flare/Core/Layer.h"
#include "Flare/Core/CommandLineArguments.h"

#include "FlareECS/ECSContext.h"

#include "FlarePlatform/FileWatcher.h"

#include "FlareEditor/UI/AssetEditor.h"

#include "FlareEditor/UI/SceneWindow.h"
#include "FlareEditor/UI/PropertiesWindow.h"
#include "FlareEditor/UI/AssetManagerWindow.h"

#include "FlareEditor/UI/EditorTitleBar.h"
#include "FlareEditor/UI/PrefabEditor.h"
#include "FlareEditor/UI/SpriteEditor.h"
#include "FlareEditor/UI/QuickSearch/QuickSearch.h"

#include "FlareEditor/ViewportWindow.h"
#include "FlareEditor/EditorCamera.h"

#include "FlareEditor/EditorSelection.h"
#include "FlareEditor/SceneViewSettings.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"

#include <vector>
#include <set>

namespace Flare
{
	enum class EditorMode
	{
		Edit,
		Play,
	};

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		~EditorLayer();
	public:
		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(float deltaTime) override;

		virtual void OnEvent(Event& event) override;
		virtual void OnImGUIRender() override;

		void SaveActiveScene();
		void SaveActiveSceneAs();

		void OpenScene(AssetHandle handle);
		void CreateNewScene();

		void EnterPlayMode();
		void ExitPlayMode();

		void ReloadScriptingModules();

		inline bool IsPlaymodePaused() const { return m_PlaymodePaused; }
		inline void SetPlaymodePaused(bool value) { m_PlaymodePaused = value; }

		inline EditorMode GetMode() const { return m_Mode; }

		inline EditorCamera& GetCamera() { return m_Camera; }
		inline const EditorCamera& GetCamera() const { return m_Camera; }

		inline ECSContext& GetECSContext() { return m_ECSContext; }
		inline const ECSContext& GetECSContext() const { return m_ECSContext; }

		inline const std::vector<Ref<ViewportWindow>>& GetViewportWindows() const { return m_ViewportWindows; }

		inline SceneViewSettings& GetSceneViewSettings() { return m_SceneViewSettings; }
		inline const SceneViewSettings& GetSceneViewSettings() const { return m_SceneViewSettings; }

		static EditorLayer& GetInstance();
	private:
		void UpdateWindowTitle();
		void OnOpenProject();

		void ResetViewportRenderGraphs();
	private:
		bool m_UpdateCursorModeNextFrame = false;
		bool m_ExitPlayModeRequested = false;

		std::set<AssetHandle> m_AssetReloadQueue;

		SceneViewSettings m_SceneViewSettings;

		Ref<ImGuiLayer> m_ImGuiLayer = nullptr;

		EditorTitleBar m_TitleBar;
		Ref<ViewportWindow> m_GameWindow;

		std::vector<Ref<AssetEditor>> m_AssetEditorWindows;
		Ref<PrefabEditor> m_PrefabEditor;
		Ref<SpriteEditor> m_SpriteEditor;

		SceneWindow m_SceneWindow;
		PropertiesWindow m_PropertiesWindow;
		AssetManagerWindow m_AssetManagerWindow;
		QuickSearch m_QuickSearch;

		std::vector<Ref<ViewportWindow>> m_ViewportWindows;

		EditorCamera m_Camera;
		AssetHandle m_EditedSceneHandle;
		bool m_PlaymodePaused;
		EditorMode m_Mode;

		ECSContext m_ECSContext;

		Scope<FileWatcher> m_ProjectFilesWacher;
	public:
		EditorSelection Selection;
	private:
		static EditorLayer* s_Instance;
	};
}