#include "PCH.h"

#include "SceneViewportWindow.h"

#include "Flare/DebugRenderer/DebugRenderer.h"

#include "Flare/Math/Math.h"

#include "Flare/Renderer/RendererComponents.h"

#include "Flare/Scene/Components.h"
#include "Flare/Scene/Hierarchy.h"
#include "Flare/Scene/Transform.h"
#include "Flare/Scene/Scene.h"
#include "Flare/Scene/Prefab.h"

#include "FlareEditor/UI/RenderGraphInspector.h"

#include "FlareEditor/Rendering/SceneViewGridPass.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"
#include "FlareEditor/EditorLayer.h"

#include "FlarePlatform/Events.h"

#include <ImGuizmo.h>

namespace Flare
{
	SceneViewportWindow::SceneViewportWindow(const Scope<SceneRenderer>& sceneRenderer,
		SceneViewSettings& sceneViewSettings,
		std::string_view name)
		: ViewportWindow(sceneRenderer, name), m_CameraController(m_EditorCamera), m_SceneViewSettings(sceneViewSettings)
	{
		World& world = Renderer::GetRenderWorld();
		Viewport& viewport = world.GetEntityComponent<Viewport>(m_ViewportEntity);

		viewport.Settings.DebugRenderingEnabled = true;
	}

	void SceneViewportWindow::OnAttach()
	{
	}

	void SceneViewportWindow::OnRenderViewport(const World& renderWorld)
	{
		FLARE_PROFILE_FUNCTION();

		Ref<Scene> scene = GetScene();
		if (scene == nullptr || !ShowWindow || !m_IsVisible)
			return;

		RenderView editorCameraView{};
		m_EditorCamera.FillRenderView(editorCameraView);

		m_SceneRenderer->RenderViewport(m_ViewportEntity, &editorCameraView, FLARE_BIND_EVENT_CALLBACK(BuildRenderGraph));
	}

	void SceneViewportWindow::OnViewportChanged()
	{
		FLARE_PROFILE_FUNCTION();
		ViewportWindow::OnViewportChanged();

		const Viewport& viewport = Renderer::GetRenderWorld().GetEntityComponent<const Viewport>(m_ViewportEntity);

		if (viewport.IsValid())
			m_EditorCamera.OnViewportChanged(viewport.Size, viewport.Position);
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
						m_EditorCamera.SetRotationOrigin(transform->Position);
				}
			}

			m_Guizmo = guizmoMode;
		}

		EndImGui();
	}

	void SceneViewportWindow::OnEvent(Event& event)
	{
	}

	void SceneViewportWindow::OnAddRenderPasses(RenderGraph& renderGraph)
	{
		FLARE_PROFILE_FUNCTION();
		if (m_SceneViewSettings.ShowGrid)
		{
			const World& renderWorld = Renderer::GetRenderWorld();

			RenderGraphPassSpecifications gridPass{};
			gridPass.AddOutput(renderWorld.GetEntityComponent<const ViewportColorOutput>(m_ViewportEntity).Id);
			gridPass.AddOutput(renderWorld.GetEntityComponent<const ViewportDepthOutput>(m_ViewportEntity).Id);
			gridPass.SetDebugName("SceneViewGridPass");
			gridPass.SetType(RenderGraphPassType::Graphics);

			renderGraph.AddPass(gridPass, Ref<SceneViewGridPass>::New());
		}
	}

	void SceneViewportWindow::RenderWindowContents()
	{
		FLARE_PROFILE_FUNCTION();
		if (GetScene() == nullptr)
			return;

		const World& renderWorld = Renderer::GetRenderWorld();
		const Viewport& viewport = renderWorld.GetEntityComponent<const Viewport>(m_ViewportEntity);
		const ViewportRenderGraph& viewportRenderGraph = renderWorld.GetEntityComponent<const ViewportRenderGraph>(m_ViewportEntity);

		RenderGraphTextureId viewportColorTexture = renderWorld.GetEntityComponent<const ViewportColorOutput>(m_ViewportEntity).Id;
		RenderGraphTextureId viewportDepthTexture = renderWorld.GetEntityComponent<const ViewportDepthOutput>(m_ViewportEntity).Id;

		if (!viewportRenderGraph.IsReadyForRendering())
			return;

		switch (m_Overlay)
		{
		case ViewportOverlay::Default:
			RenderViewportBuffer(viewportRenderGraph.Graph->GetTexture(viewportColorTexture));
			break;
		case ViewportOverlay::Depth:
			RenderViewportBuffer(viewportRenderGraph.Graph->GetTexture(viewportDepthTexture));
			break;
		}

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

		HandleGuizmo();

		if (m_RenderGraphInspector)
		{
			m_RenderGraphInspector->OnRenderImGui();
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
		case ViewportOverlay::Depth:
			overlayName = "Depth";
			break;
		}

		if (ImGui::BeginCombo("", overlayName))
		{
			if (ImGui::MenuItem("Default"))
				m_Overlay = ViewportOverlay::Default;
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
			if (ImGui::BeginCombo("", "Settings"))
			{
				World& renderWorld = Renderer::GetRenderWorld();
				Viewport& viewport = renderWorld.GetEntityComponent<Viewport>(m_ViewportEntity);
				ViewportRenderGraph& viewportRenderGraph = renderWorld.GetEntityComponent<ViewportRenderGraph>(m_ViewportEntity);

				ImGui::MenuItem("Shadows", nullptr, &viewport.Settings.ShadowMappingEnabled);
				ImGui::MenuItem("Post Processing", nullptr, &viewport.Settings.PostProcessingEnabled);
				ImGui::MenuItem("Debug Rendering", nullptr, &viewport.Settings.DebugRenderingEnabled);

				ImGui::Separator();

				ImGui::MenuItem("Show AABBs", nullptr, &m_SceneViewSettings.ShowAABBs);
				ImGui::MenuItem("Show Lights", nullptr, &m_SceneViewSettings.ShowLights);
				ImGui::MenuItem("Show Camera Frustums", nullptr, &m_SceneViewSettings.ShowCameraFrustum);

				if (ImGui::MenuItem("Show Grid", nullptr, &m_SceneViewSettings.ShowGrid))
				{
					viewportRenderGraph.Graph->SetNeedsRebuilding();
				}

				if (ImGui::MenuItem("Inspect Render Graph"))
				{
					if (!m_RenderGraphInspector)
					{
						m_RenderGraphInspector = CreateScope<RenderGraphInspector>(*viewportRenderGraph.Graph);
					}

					m_RenderGraphInspector->SetVisible(true);
				}

				ImGui::EndCombo();
			}
			ImGui::PopID();
		}

		// Transformation Space

		ImGui::SameLine();

		{
			ImGui::PushID("TransformationSpace");

			const char* spaceName = "";
			switch (m_TransformationSpace)
			{
			case TransformationSpace::Local:
				spaceName = "Local";
				break;
			case TransformationSpace::World:
				spaceName = "World";
				break;
			default:
				FLARE_CORE_ASSERT(false);
			}

			if (ImGui::BeginCombo("", spaceName))
			{
				if (ImGui::MenuItem("Local"))
					m_TransformationSpace = TransformationSpace::Local;
				if (ImGui::MenuItem("World"))
					m_TransformationSpace = TransformationSpace::World;

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
		FLARE_PROFILE_FUNCTION();
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
				Entity instance = prefab->CreateInstance(GetScene()->GetECSWorld());

				EditorLayer::GetInstance().Selection.SetEntity(instance);

				break;
			}
			case AssetType::Mesh:
			{
				Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(handle);
				if (mesh)
				{
					Entity entity = world.CreateEntity(
						TransformComponent(),
						LocalTransform(),
						MeshRenderer(mesh));

					EditorLayer::GetInstance().Selection.SetEntity(entity);
				}

				break;
			}
			}
		}
	}

	void SceneViewportWindow::HandleGuizmo()
	{
		FLARE_PROFILE_FUNCTION();

		const Viewport& viewport = Renderer::GetRenderWorld().GetEntityComponent<const Viewport>(m_ViewportEntity);

		World& world = GetScene()->GetECSWorld();
		const EditorSelection& selection = EditorLayer::GetInstance().Selection;

		bool showGuizmo = m_Guizmo != GuizmoMode::None;
		bool hasSelection = selection.GetType() == EditorSelectionType::Entity && world.IsEntityAlive(selection.GetEntity());

		if (!showGuizmo || !hasSelection)
			return;

		Entity selectedEntity = selection.GetEntity();
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();

		ImVec2 windowPosition = ImGui::GetWindowPos();
		ImGuizmo::SetRect(windowPosition.x + m_ViewportOffset.x,
			windowPosition.y + m_ViewportOffset.y,
			(float)viewport.Size.x,
			(float)viewport.Size.y);

		Math::AffineTransform* globalTransform = world.TryGetEntityComponent<TransformComponent>(selectedEntity);
		Math::AffineTransform* localTransform = world.TryGetEntityComponent<LocalTransform>(selectedEntity);

		const Parent* parent = world.TryGetEntityComponent<const Parent>(selectedEntity);

		if (globalTransform && localTransform)
		{
			if (parent)
			{
				const TransformComponent* parentTransform = world.TryGetEntityComponent<const TransformComponent>(parent->GetParentEntity());
				if (parentTransform)
				{
					glm::mat4 parentTransformationMatrix = parentTransform->GetTransformationMatrix();
					if (HandleTransformation(*localTransform, globalTransform, &parentTransformationMatrix))
					{
						*globalTransform = *localTransform;
						globalTransform->ApplyTransform(*parentTransform);

						TransformPropagationSystem::PropagateTransformToChildren(world, selectedEntity);
					}
				}
			}
			else
			{
				if (HandleTransformation(*localTransform, globalTransform, nullptr))
				{
					*globalTransform = *localTransform;
					TransformPropagationSystem::PropagateTransformToChildren(world, selectedEntity);
				}
			}
		}
		else if (globalTransform && !localTransform)
		{
			HandleTransformation(*globalTransform, nullptr, nullptr);
		}
		else if (!globalTransform && localTransform)
		{
			HandleTransformation(*localTransform, nullptr, nullptr);
		}
	}

	bool SceneViewportWindow::HandleTransformation(Math::AffineTransform& localTransform,
		const Math::AffineTransform* globalTransform,
		const glm::mat4* parentTransform) const
	{
		FLARE_PROFILE_FUNCTION();

		glm::mat4 transformationMatrix;
		glm::mat4 worldToLocalSpace;

		if (globalTransform)
			transformationMatrix = globalTransform->GetTransformationMatrix();
		else
			transformationMatrix = localTransform.GetTransformationMatrix();

		if (parentTransform)
			worldToLocalSpace = glm::inverse(*parentTransform);
		else
			worldToLocalSpace = glm::mat4(1.0f);


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
			FLARE_CORE_ASSERT(false);
		}

		ImGuizmo::MODE mode = ImGuizmo::WORLD;
		switch (m_TransformationSpace)
		{
		case TransformationSpace::Local:
			mode = ImGuizmo::LOCAL;
			break;
		case TransformationSpace::World:
			mode = ImGuizmo::WORLD;
			break;
		default:
			FLARE_CORE_ASSERT(false);
		}

		bool snappingEnabled = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
		if (ImGuizmo::Manipulate(
			glm::value_ptr(m_EditorCamera.GetViewMatrix()),
			glm::value_ptr(m_EditorCamera.GetProjectionMatrix()),
			operation, mode,
			glm::value_ptr(transformationMatrix),
			nullptr, snappingEnabled ? &snapValue : nullptr))
		{
			Math::DecomposeTransform(worldToLocalSpace * transformationMatrix,
				localTransform.Position,
				localTransform.Rotation,
				localTransform.Scale);

			localTransform.Rotation = glm::degrees(localTransform.Rotation);
			return true;
		}

		return false;
	}
}
