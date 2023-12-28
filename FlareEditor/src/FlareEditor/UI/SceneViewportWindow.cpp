#include "SceneViewportWindow.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RenderCommand.h"
#include "Flare/Renderer/DebugRenderer.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Scene/Components.h"
#include "Flare/Scene/Scene.h"
#include "Flare/Scene/Prefab.h"

#include "Flare/Math/Math.h"

#include "Flare/Input/InputManager.h"

#include "FlareEditor/AssetManager/EditorAssetManager.h"
#include "FlareEditor/ImGui/ImGuiLayer.h"
#include "FlareEditor/EditorLayer.h"
#include "FlareEditor/UI/EditorGUI.h"

#include "FlarePlatform//Events.h"

#include <ImGuizmo.h>

namespace Flare
{
	struct GridPropertyIndices
	{
		uint32_t Offset = UINT32_MAX;
		uint32_t Scale = UINT32_MAX;
		uint32_t Thickness = UINT32_MAX;
		uint32_t CellScale = UINT32_MAX;
		uint32_t Color = UINT32_MAX;
		uint32_t FallOffThreshold = UINT32_MAX;
	};

	static GridPropertyIndices s_GridPropertyIndices;

	SceneViewportWindow::SceneViewportWindow(EditorCamera& camera)
		: ViewportWindow("Scene Viewport", true),
		m_Camera(camera),
		m_Overlay(ViewportOverlay::Default),
		m_IsToolbarHovered(false) {}

	void SceneViewportWindow::OnAttach()
	{
		AssetHandle gridShaderHandle = ShaderLibrary::FindShader("SceneViewGrid").value_or(NULL_ASSET_HANDLE);
		if (AssetManager::IsAssetHandleValid(gridShaderHandle))
		{
			m_GridMaterial = CreateRef<Material>(AssetManager::GetAsset<Shader>(gridShaderHandle));

			Ref<Shader> gridShader = m_GridMaterial->GetShader();
			s_GridPropertyIndices.Color = gridShader->GetPropertyIndex("u_Data.Color").value_or(UINT32_MAX);
			s_GridPropertyIndices.Offset = gridShader->GetPropertyIndex("u_Data.Offset").value_or(UINT32_MAX);
			s_GridPropertyIndices.Scale = gridShader->GetPropertyIndex("u_Data.GridScale").value_or(UINT32_MAX);
			s_GridPropertyIndices.Thickness = gridShader->GetPropertyIndex("u_Data.Thickness").value_or(UINT32_MAX);
			s_GridPropertyIndices.CellScale = gridShader->GetPropertyIndex("u_Data.CellScale").value_or(UINT32_MAX);
			s_GridPropertyIndices.FallOffThreshold = gridShader->GetPropertyIndex("u_Data.FallOffThreshold").value_or(UINT32_MAX);
		}
		else
			FLARE_CORE_ERROR("Failed to load scene view grid shader");

		AssetHandle selectionOutlineShader = ShaderLibrary::FindShader("SelectionOutline").value_or(NULL_ASSET_HANDLE);
		if (AssetManager::IsAssetHandleValid(selectionOutlineShader))
		{
			Ref<Shader> shader = AssetManager::GetAsset<Shader>(selectionOutlineShader);
			m_SelectionOutlineMaterial = CreateRef<Material>(shader);

			ImVec4 primaryColor = ImGuiTheme::Primary;
			glm::vec4 selectionColor = glm::vec4(primaryColor.x, primaryColor.y, primaryColor.z, 1.0f);

			std::optional<uint32_t> colorProperty = shader->GetPropertyIndex("u_Outline.Color");

			if (colorProperty)
				m_SelectionOutlineMaterial->WritePropertyValue(*colorProperty, selectionColor);
		}
		else
			FLARE_CORE_ERROR("Failed to load selection outline shader");
	}

	void SceneViewportWindow::OnRenderViewport()
	{
		FLARE_PROFILE_FUNCTION();

		if (Scene::GetActive() == nullptr || !ShowWindow || !m_IsVisible)
			return;

		if (m_Viewport.FrameData.IsEditorCamera)
		{
			m_Viewport.FrameData.Camera.View = m_Camera.GetViewMatrix();
			m_Viewport.FrameData.Camera.Projection = m_Camera.GetProjectionMatrix();
			m_Viewport.FrameData.Camera.InverseProjection = glm::inverse(m_Viewport.FrameData.Camera.Projection);
			m_Viewport.FrameData.Camera.CalculateViewProjection();
			m_Viewport.FrameData.Camera.Position = m_Camera.GetPosition();

			m_Viewport.FrameData.Camera.ViewDirection = m_Camera.GetViewDirection();

			m_Viewport.FrameData.Camera.Near = m_Camera.GetSettings().Near;
			m_Viewport.FrameData.Camera.Far = m_Camera.GetSettings().Far;

			m_Viewport.FrameData.Camera.FOV = m_Camera.GetSettings().FOV;
		}

		PrepareViewport();

		Ref<Scene> scene = Scene::GetActive();
		if (m_Viewport.GetSize() != glm::ivec2(0))
		{
			std::optional<SystemGroupId> debugRenderingGroup = scene->GetECSWorld().GetSystemsManager().FindGroup("Debug Rendering");

			scene->OnBeforeRender(m_Viewport);

			Renderer::BeginScene(m_Viewport);
			m_ScreenBuffer->Bind();
			OnClear();

			scene->OnRender(m_Viewport);

			RenderGrid();

			if (debugRenderingGroup.has_value())
			{
				DebugRenderer::Begin();
				scene->GetECSWorld().GetSystemsManager().ExecuteGroup(debugRenderingGroup.value());

				for (uint32_t i = 0; i < 3; i++)
				{
					glm::vec3 direction = glm::vec3(0.0f);
					direction[i] = 1.0f;
					DebugRenderer::DrawRay(glm::vec3(0.0f), direction * 0.5f, glm::vec4(direction * 0.66f, 1.0f));
				}

				DebugRenderer::End();
			}

			Entity selectedEntity = EditorLayer::GetInstance().Selection.TryGetEntity().value_or(Entity());
			if (selectedEntity != Entity() && m_SelectionOutlineMaterial)
			{
				m_ScreenBuffer->BindAttachmentTexture(1);

				std::optional<uint32_t> idPropertyIndex = m_SelectionOutlineMaterial->GetShader()->GetPropertyIndex("u_Outline.SelectedId");

				if (idPropertyIndex)
				{
					Ref<Shader> shader = m_SelectionOutlineMaterial->GetShader();

					m_SelectionOutlineMaterial->WritePropertyValue<int32_t>(
						*idPropertyIndex,
						(int32_t)selectedEntity.GetIndex());

					std::optional<uint32_t> thicknessProperty = shader->GetPropertyIndex("u_Outline.Thickness");
					if (thicknessProperty)
						m_SelectionOutlineMaterial->WritePropertyValue(*thicknessProperty, glm::vec2(4.0f) / (glm::vec2)m_Viewport.GetSize() / 2.0f);

					Renderer::DrawFullscreenQuad(m_SelectionOutlineMaterial);
				}
			}

			Renderer::EndScene();

			m_Viewport.RenderTarget->Unbind();
		}
	}

	void SceneViewportWindow::OnViewportChanged()
	{
		m_ScreenBuffer->Resize(m_Viewport.GetSize().x, m_Viewport.GetSize().y);
		m_Camera.OnViewportChanged(m_Viewport.GetSize(), m_Viewport.GetPosition());
	}

	void SceneViewportWindow::OnRenderImGui()
	{
		if (!ShowWindow)
			return;

		BeginImGui();

		if (m_IsVisible)
		{
			if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
				ImGui::SetWindowFocus();

			RenderWindowContents();
		}

		EndImGui();
	}

	void SceneViewportWindow::OnEvent(Event& event)
	{
		if (!m_IsFocused)
			return;

		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<KeyReleasedEvent>([this](KeyReleasedEvent& e) -> bool
		{
			GuizmoMode guizmoMode = GuizmoMode::None;
			switch (e.GetKeyCode())
			{
			case KeyCode::G:
				guizmoMode = GuizmoMode::Translate;
				break;
			case KeyCode::R:
				guizmoMode = GuizmoMode::Rotate;
				break;
			case KeyCode::S:
				guizmoMode = GuizmoMode::Scale;
				break;
			default:
				return false;
			}

			EditorLayer::GetInstance().Guizmo = guizmoMode;
			return false;
		});

		if (m_IsHovered)
		{
			m_Camera.ProcessEvents(event);
		}
	}

	void SceneViewportWindow::CreateFrameBuffer()
	{
		FrameBufferSpecifications screenBufferSpecs(m_Viewport.GetSize().x, m_Viewport.GetSize().y, {
			{ FrameBufferTextureFormat::RGB8, TextureWrap::Clamp, TextureFiltering::Closest },
			{ FrameBufferTextureFormat::RedInteger, TextureWrap::Clamp, TextureFiltering::Closest },
			{ FrameBufferTextureFormat::Depth, TextureWrap::Clamp, TextureFiltering::Closest },
		});

		m_ScreenBuffer = FrameBuffer::Create(screenBufferSpecs);
		m_Viewport.RenderTarget = m_ScreenBuffer;
	}

	void SceneViewportWindow::OnClear()
	{
		RenderCommand::Clear();
		m_ScreenBuffer->ClearAttachment(1, INT32_MAX);
	}

	void SceneViewportWindow::RenderWindowContents()
	{
		if (Scene::GetActive() == nullptr)
		{
			return;
		}

		switch (m_Overlay)
		{
		case ViewportOverlay::Default:
			RenderViewportBuffer(m_Viewport.RenderTarget, 0);
			break;
		case ViewportOverlay::Depth:
			RenderViewportBuffer(m_ScreenBuffer, 2);
			break;
		}

		World& world = Scene::GetActive()->GetECSWorld();
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_PAYLOAD_NAME))
			{
				Entity entity = GetEntityUnderCursor();
				AssetHandle handle = *(AssetHandle*)payload->Data;
				const AssetMetadata* metadata = AssetManager::GetAssetMetadata(handle);
				if (metadata != nullptr)
				{
					switch (metadata->Type)
					{
					case AssetType::Scene:
						EditorLayer::GetInstance().OpenScene(handle);
						break;
					case AssetType::Texture:
					{
						if (!world.IsEntityAlive(entity))
							break;

						std::optional<SpriteComponent*> sprite = world.TryGetEntityComponent<SpriteComponent>(entity);
						if (sprite.has_value())
							sprite.value()->Texture = handle;

						break;
					}
					case AssetType::Material:
					{
						std::optional<MeshComponent*> meshComponent = world.TryGetEntityComponent<MeshComponent>(entity);
						if (meshComponent)
							meshComponent.value()->Material = handle;
						else
						{
							std::optional<MaterialComponent*> materialComponent = world.TryGetEntityComponent<MaterialComponent>(entity);
							if (materialComponent)
								materialComponent.value()->Material = handle;
						}

						break;
					}
					case AssetType::Prefab:
					{
						Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(handle);
						prefab->CreateInstance(Scene::GetActive()->GetECSWorld());
						break;
					}
					}
				}

				ImGui::EndDragDropTarget();
			}
		}


		// Render scene toolbar
		RenderToolBar();

		m_IsToolbarHovered = ImGui::IsAnyItemHovered();

		ImGuiIO& io = ImGui::GetIO();

		const EditorSelection& selection = EditorLayer::GetInstance().Selection;

		bool showGuizmo = EditorLayer::GetInstance().Guizmo != GuizmoMode::None;
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

			std::optional<TransformComponent*> transform = world.TryGetEntityComponent<TransformComponent>(selectedEntity);
			if (transform.has_value())
			{
				glm::mat4 transformationMatrix = transform.value()->GetTransformationMatrix();

				// TODO: move snap values to editor settings
				float snapValue = 0.5f;

				ImGuizmo::OPERATION operation = (ImGuizmo::OPERATION)-1;
				switch (EditorLayer::GetInstance().Guizmo)
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
						transform.value()->Position,
						transform.value()->Rotation,
						transform.value()->Scale);

					transform.value()->Rotation = glm::degrees(transform.value()->Rotation);
				}
			}
		}

		if (!ImGuizmo::IsUsingAny() && !m_IsToolbarHovered)
		{
			if (io.MouseClicked[ImGuiMouseButton_Left] && m_Viewport.RenderTarget != nullptr && m_IsHovered && m_RelativeMousePosition.x >= 0 && m_RelativeMousePosition.y >= 0)
				EditorLayer::GetInstance().Selection.SetEntity(GetEntityUnderCursor());
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
			if (GuizmoButton("T", editorLayer.Guizmo == GuizmoMode::Translate))
				editorLayer.Guizmo = GuizmoMode::Translate;

			ImGui::SameLine();
			if (GuizmoButton("R", editorLayer.Guizmo == GuizmoMode::Rotate))
				editorLayer.Guizmo = GuizmoMode::Rotate;

			ImGui::SameLine();
			if (GuizmoButton("S", editorLayer.Guizmo == GuizmoMode::Scale))
				editorLayer.Guizmo = GuizmoMode::Scale;

			ImGui::PopStyleVar(); // Item spacing
		}

		// Scene View Settings

		ImGui::SameLine();
		
		{
			ImGui::PushID("SceneViewSettings");
			SceneViewSettings& settings = EditorLayer::GetInstance().GetSceneViewSettings();
			if (ImGui::BeginCombo("", "Settings"))
			{
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

	void SceneViewportWindow::RenderGrid()
	{
		if (!m_GridMaterial || !EditorLayer::GetInstance().GetSceneViewSettings().ShowGrid)
			return;

		float scale = m_Camera.GetZoom() * 3.0f;
		float cellScale = 5.0f + glm::floor(m_Camera.GetZoom() / 20.0f) * 5.0f;
		glm::vec3 gridColor = glm::vec3(0.5f);

		glm::vec3 cameraPosition = m_Camera.GetRotationOrigin();

		m_GridMaterial->WritePropertyValue(s_GridPropertyIndices.Offset, glm::vec3(cameraPosition.x, 0.0f, cameraPosition.z));
		m_GridMaterial->WritePropertyValue(s_GridPropertyIndices.Scale, scale);
		m_GridMaterial->WritePropertyValue(s_GridPropertyIndices.Thickness, 0.01f);
		m_GridMaterial->WritePropertyValue(s_GridPropertyIndices.CellScale, 1.0f / cellScale);
		m_GridMaterial->WritePropertyValue(s_GridPropertyIndices.Color, gridColor);
		m_GridMaterial->WritePropertyValue(s_GridPropertyIndices.FallOffThreshold, 0.8f);
		Renderer::DrawFullscreenQuad(m_GridMaterial);
	}

	Entity SceneViewportWindow::GetEntityUnderCursor() const
	{
		m_ScreenBuffer->Bind();

		int32_t entityIndex;
		m_ScreenBuffer->ReadPixel(1, m_RelativeMousePosition.x, m_RelativeMousePosition.y, &entityIndex);

		std::optional<Entity> entity = Scene::GetActive()->GetECSWorld().Entities.FindEntityByIndex(entityIndex);

		m_ScreenBuffer->Unbind();

		return entity.value_or(Entity());
	}
}
