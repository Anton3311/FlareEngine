#include "SceneViewportWindow.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/ShaderLibrary.h"

#include "Flare/DebugRenderer/DebugRenderer.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

#include "Flare/Scene/Components.h"
#include "Flare/Scene/Scene.h"
#include "Flare/Scene/Prefab.h"

#include "Flare/Math/Math.h"

#include "Flare/Input/InputManager.h"

#include "FlareEditor/Rendering/SceneViewGridPass.h"

#include "FlareEditor/AssetManager/EditorAssetManager.h"
#include "FlareEditor/ImGui/ImGuiLayer.h"
#include "FlareEditor/EditorLayer.h"
#include "FlareEditor/UI/EditorGUI.h"

#include "FlarePlatform/Events.h"

#include <ImGuizmo.h>

namespace Flare
{
	SceneViewportWindow::SceneViewportWindow(EditorCamera& camera, const Scope<SceneRenderer>& sceneRenderer, std::string_view name)
		: ViewportWindow(sceneRenderer, name),
		m_Camera(camera),
		m_Overlay(ViewportOverlay::Default),
		m_IsToolbarHovered(false),
		m_CameraController(m_Camera),
		m_Guizmo(GuizmoMode::None)
	{
		m_Viewport.SetDebugRenderingEnabled(true);
	}

	void SceneViewportWindow::OnAttach()
	{
	}

	void SceneViewportWindow::OnRenderViewport()
	{
		FLARE_PROFILE_FUNCTION();

		Ref<Scene> scene = GetScene();
		if (scene == nullptr || !ShowWindow || !m_IsVisible)
			return;

		PrepareViewport();

		if (m_Viewport.GetSize() == glm::ivec2(0))
			return;

		std::optional<SystemGroupId> debugRenderingGroup = scene->GetECSWorld().GetSystemsManager().FindGroup("Debug Rendering");

		RenderView editorCameraView{};
		editorCameraView.SetViewAndProjection(m_Camera.GetProjectionMatrix(), m_Camera.GetViewMatrix());
		editorCameraView.Position = m_Camera.GetPosition();
		editorCameraView.ViewDirection = m_Camera.GetViewDirection();
		editorCameraView.Near = m_Camera.GetSettings().Near;
		editorCameraView.Far = m_Camera.GetSettings().Far;
		editorCameraView.FOV = m_Camera.GetSettings().FOV;

		m_SceneRenderer->RenderViewport(m_Viewport);

		return;

		Renderer::BeginScene(m_Viewport);
		OnClear();

		scene->OnRender(m_Viewport);

		std::optional<Entity> selectedEntity = EditorLayer::GetInstance().Selection.TryGetEntity();
		if (debugRenderingGroup.has_value())
		{
			FLARE_PROFILE_SCOPE("DebugRendering");
			DebugRenderer::Begin();

			scene->GetECSWorld().GetSystemsManager().ExecuteGroup(debugRenderingGroup.value());

			// Draw bouding box for decal projectors
			if (selectedEntity)
			{
				const Decal* decal = scene->GetECSWorld().TryGetEntityComponent<const Decal>(*selectedEntity);
				const TransformComponent* transform = scene->GetECSWorld().TryGetEntityComponent<const TransformComponent>(*selectedEntity);
				if (decal && transform)
				{
					glm::vec3 cubeCorners[] =
					{
						glm::vec3(-0.5f, -0.5f, -0.5f),
						glm::vec3(+0.5f, -0.5f, -0.5f),
						glm::vec3(-0.5f, +0.5f, -0.5f),
						glm::vec3(+0.5f, +0.5f, -0.5f),

						glm::vec3(-0.5f, -0.5f, +0.5f),
						glm::vec3(+0.5f, -0.5f, +0.5f),
						glm::vec3(-0.5f, +0.5f, +0.5f),
						glm::vec3(+0.5f, +0.5f, +0.5f),
					};

					glm::mat4 transformationMatrix = transform->GetTransformationMatrix();
					for (size_t i = 0; i < 8; i++)
						cubeCorners[i] = transformationMatrix * glm::vec4(cubeCorners[i], 1.0f);

					DebugRenderer::DrawWireBox(cubeCorners);
				}
			}

			DebugRenderer::End();
		}

		Renderer::EndScene();
	}

	void SceneViewportWindow::OnViewportChanged()
	{
		FLARE_PROFILE_FUNCTION();
		ViewportWindow::OnViewportChanged();
		m_Camera.OnViewportChanged(m_Viewport.GetSize(), m_Viewport.GetPosition());
	}

	void SceneViewportWindow::OnRenderImGui()
	{
		FLARE_PROFILE_FUNCTION();
		if (!ShowWindow)
			return;

		BeginImGui();

		if (m_IsVisible)
		{
			if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
				ImGui::SetWindowFocus();

			RenderWindowContents();
		}

		m_CameraController.Update(m_RelativeMousePosition);

		if (m_IsFocused)
		{
			EditorLayer& editorLayer = EditorLayer::GetInstance();
			GuizmoMode guizmoMode = m_Guizmo;

			if (ImGui::IsKeyPressed(ImGuiKey_Escape))
				guizmoMode = GuizmoMode::None;
			if (ImGui::IsKeyPressed(ImGuiKey_G))
				guizmoMode = GuizmoMode::Translate;
			if (ImGui::IsKeyPressed(ImGuiKey_R))
				guizmoMode = GuizmoMode::Rotate;
			if (ImGui::IsKeyPressed(ImGuiKey_S))
				guizmoMode = GuizmoMode::Scale;

			if (ImGui::IsKeyPressed(ImGuiKey_F))
			{
				const auto& editorSelection = EditorLayer::GetInstance().Selection;
				if (editorSelection.GetType() == EditorSelectionType::Entity)
				{
					const World& world = GetScene()->GetECSWorld();
					const TransformComponent* transform = world.TryGetEntityComponent<TransformComponent>(editorSelection.GetEntity());

					if (transform)
						m_Camera.SetRotationOrigin(transform->Position);
				}
			}
				
			m_Guizmo = guizmoMode;
		}

		EndImGui();
	}

	void SceneViewportWindow::OnEvent(Event& event)
	{
	}

	void SceneViewportWindow::OnAddRenderPasses()
	{
		if (EditorLayer::GetInstance().GetSceneViewSettings().ShowGrid)
		{
			RenderGraphPassSpecifications gridPass{};
			gridPass.AddOutput(m_Viewport.ColorTextureId, 0);
			gridPass.AddOutput(m_Viewport.DepthTextureId, 1);
			gridPass.SetDebugName("SceneViewGridPass");
			gridPass.SetType(RenderGraphPassType::Graphics);

			m_Viewport.Graph.AddPass(gridPass, CreateRef<SceneViewGridPass>());
		}
	}

	void SceneViewportWindow::RenderWindowContents()
	{
		FLARE_PROFILE_FUNCTION();
		if (GetScene() == nullptr)
			return;

		switch (m_Overlay)
		{
		case ViewportOverlay::Default:
			RenderViewportBuffer(m_Viewport.Graph.GetTexture(m_Viewport.ColorTextureId));
			break;
		case ViewportOverlay::Normal:
			RenderViewportBuffer(m_Viewport.Graph.GetTexture(m_Viewport.NormalsTextureId));
			break;
		case ViewportOverlay::Depth:
			RenderViewportBuffer(m_Viewport.Graph.GetTexture(m_Viewport.DepthTextureId));
			break;
		}

		World& world = GetScene()->GetECSWorld();
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_PAYLOAD_NAME))
			{
				HandleAssetDragAndDrop(*(AssetHandle*)payload->Data);
				ImGui::EndDragDropTarget();
			}
		}

		// Render scene toolbar
		RenderToolBar();

		m_IsToolbarHovered = ImGui::IsAnyItemHovered();

		ImGuiIO& io = ImGui::GetIO();

		const EditorSelection& selection = EditorLayer::GetInstance().Selection;

		bool showGuizmo = m_Guizmo != GuizmoMode::None;
		bool hasSelection = selection.GetType() == EditorSelectionType::Entity && world.IsEntityAlive(selection.GetEntity());
		if (showGuizmo && hasSelection)
		{
			Entity selectedEntity = selection.GetEntity();
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();

			ImVec2 windowPosition = ImGui::GetWindowPos();
			ImGuizmo::SetRect(windowPosition.x + m_ViewportOffset.x,
				windowPosition.y + m_ViewportOffset.y,
				(float)m_Viewport.GetSize().x,
				(float)m_Viewport.GetSize().y);

			TransformComponent* transform = world.TryGetEntityComponent<TransformComponent>(selectedEntity);
			if (transform)
			{
				glm::mat4 transformationMatrix = transform->GetTransformationMatrix();

				// TODO: move snap values to editor settings
				float snapValue = 0.5f;

				ImGuizmo::OPERATION operation = (ImGuizmo::OPERATION)-1;
				switch (m_Guizmo)
				{
				case GuizmoMode::Translate:
					operation = ImGuizmo::TRANSLATE;
					break;
				case GuizmoMode::Rotate:
					snapValue = 5.0f;
					operation = ImGuizmo::ROTATE;
					break;
				case GuizmoMode::Scale:
					operation = ImGuizmo::SCALE;
					break;
				default:
					FLARE_CORE_ASSERT("Unhandled Gizmo type");
				}

				ImGuizmo::MODE mode = ImGuizmo::WORLD;

				bool snappingEnabled = InputManager::IsKeyHeld(KeyCode::LeftControl) || InputManager::IsKeyHeld(KeyCode::RightControl);

				if (ImGuizmo::Manipulate(
					glm::value_ptr(m_Camera.GetViewMatrix()),
					glm::value_ptr(m_Camera.GetProjectionMatrix()),
					operation, mode,
					glm::value_ptr(transformationMatrix),
					nullptr, snappingEnabled ? &snapValue : nullptr))
				{
					Math::DecomposeTransform(transformationMatrix,
						transform->Position,
						transform->Rotation,
						transform->Scale);

					transform->Rotation = glm::degrees(transform->Rotation);
				}
			}
		}
	}

	static bool GuizmoButton(const char* text, bool active)
	{
		bool result = false;
		ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;
		ImGuiStyle& style = ImGui::GetStyle();

		if (ImGui::InvisibleButton(text, ImVec2(30, style.FramePadding.y * 2 + ImGui::GetFontSize())))
			result = true;

		ImRect buttonRect = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };
		ImU32 buttonColor = 0;

		ImVec2 textSize = ImGui::CalcTextSize(text, text + 1);
		ImVec2 textPosition = buttonRect.Min + buttonRect.GetSize() / 2.0f - (textSize / 2.0f);

		if (ImGui::IsItemHovered())
			buttonColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_ButtonHovered]);
		if (active)
			buttonColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_ButtonActive]);

		if (buttonColor != 0)
			drawList->AddRectFilled(buttonRect.Min, buttonRect.Max, buttonColor, style.FrameRounding);

		drawList->AddText(textPosition, ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]), text, text + 1);

		return result;
	}

	void SceneViewportWindow::RenderToolBar()
	{
		FLARE_PROFILE_FUNCTION();
		ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;
		ImVec2 initialCursorPosition = ImGui::GetCursorPos();

		const ImGuiStyle& style = ImGui::GetStyle();
		ImRect viewportImageRect = { ImGui::GetItemRectMin() + style.FramePadding * ImVec2(3, 1), ImGui::GetItemRectMax() };

		ImGui::SetCursorPos(ImVec2((float)m_ViewportOffset.x, (float)m_ViewportOffset.y) + style.FramePadding * ImVec2(3, 1));

		ImGui::PushItemWidth(100);
		ImGui::PushID("Overlay");

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.FramePadding); // Window Padding
		const char* overlayName = nullptr;
		switch (m_Overlay)
		{
		case ViewportOverlay::Default:
			overlayName = "Default";
			break;
		case ViewportOverlay::Normal:
			overlayName = "Normal";
			break;
		case ViewportOverlay::Depth:
			overlayName = "Depth";
			break;
		}

		if (ImGui::BeginCombo("", overlayName))
		{
			if (ImGui::MenuItem("Default"))
				m_Overlay = ViewportOverlay::Default;
			if (ImGui::MenuItem("Normal"))
				m_Overlay = ViewportOverlay::Normal;
			if (ImGui::MenuItem("Depth"))
				m_Overlay = ViewportOverlay::Depth;

			ImGui::EndCombo();
		}

		ImRect comboBoxRect = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };
		drawList->AddRect(
			comboBoxRect.Min,
			comboBoxRect.Max,
			ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Border]), style.FrameRounding, 0, 1.5f);

		ImGui::PopID();


		// Guizmos
		float width = 30.0f * 3.0f;
		float offset = style.ItemSpacing.x + ImGui::GetItemRectSize().x;
		float buttonHeight = style.FramePadding.y * 2 + ImGui::GetFontSize();

		EditorLayer& editorLayer = EditorLayer::GetInstance();
		drawList->AddRectFilled(
			viewportImageRect.Min + ImVec2(offset, 0.0f), 
			viewportImageRect.Min + ImVec2(offset + width, buttonHeight),
			ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_FrameBg]), style.FrameRounding);

		drawList->AddRect(
			viewportImageRect.Min + ImVec2(offset, 0.0f),
			viewportImageRect.Min + ImVec2(offset + width, buttonHeight),
			ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Border]), style.FrameRounding, 0, 1.5f);

		ImGui::SameLine();

		{
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
			if (GuizmoButton("T", m_Guizmo == GuizmoMode::Translate))
				m_Guizmo = GuizmoMode::Translate;

			ImGui::SameLine();
			if (GuizmoButton("R", m_Guizmo == GuizmoMode::Rotate))
				m_Guizmo = GuizmoMode::Rotate;

			ImGui::SameLine();
			if (GuizmoButton("S", m_Guizmo == GuizmoMode::Scale))
				m_Guizmo = GuizmoMode::Scale;

			ImGui::PopStyleVar(); // Item spacing
		}

		// Scene View Settings

		ImGui::SameLine();
		
		{
			ImGui::PushID("SceneViewSettings");
			SceneViewSettings& settings = EditorLayer::GetInstance().GetSceneViewSettings();
			if (ImGui::BeginCombo("", "Settings"))
			{
				{
					bool value = m_Viewport.IsShadowMappingEnabled();
					if (ImGui::MenuItem("Shadows", nullptr, &value))
						m_Viewport.SetShadowMappingEnabled(value);
				}

				{
					bool value = m_Viewport.IsPostProcessingEnabled();
					if (ImGui::MenuItem("Post Processing", nullptr, &value))
						m_Viewport.SetPostProcessingEnabled(value);
				}

				{
					bool value = m_Viewport.IsDebugRenderingEnabled();
					if (ImGui::MenuItem("Debug Rendering", nullptr, &value))
						m_Viewport.SetDebugRenderingEnabled(value);
				}

				ImGui::Separator();

				ImGui::MenuItem("Show AABBs", nullptr, &settings.ShowAABBs);
				ImGui::MenuItem("Show Lights", nullptr, &settings.ShowLights);
				ImGui::MenuItem("Show Camera Frustums", nullptr, &settings.ShowCameraFrustum);
				ImGui::MenuItem("Show Grid", nullptr, &settings.ShowGrid);

				ImGui::EndCombo();
			}
			ImGui::PopID();
		}

		ImGui::PopItemWidth();

		ImGui::PopStyleVar(); // Window Padding

		ImGui::SetCursorPos(initialCursorPosition);
	}

	void SceneViewportWindow::HandleAssetDragAndDrop(AssetHandle handle)
	{
		World& world = GetScene()->GetECSWorld();
		const AssetMetadata* metadata = AssetManager::GetAssetMetadata(handle);
		if (metadata != nullptr)
		{
			switch (metadata->Type)
			{
			case AssetType::Scene:
				EditorLayer::GetInstance().OpenScene(handle);
				break;
			case AssetType::Prefab:
			{
				Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(handle);
				prefab->CreateInstance(GetScene()->GetECSWorld());
				break;
			}
			}
		}
	}
}
