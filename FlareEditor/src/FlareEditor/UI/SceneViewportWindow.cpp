#include "SceneViewportWindow.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RenderCommand.h"
#include "Flare/Renderer/DebugRenderer.h"
#include "Flare/Scene/Components.h"
#include "Flare/Scene/Scene.h"
#include "Flare/Scene/Prefab.h"

#include "Flare/Math/Math.h"

#include "Flare/Input/InputManager.h"

#include "FlareEditor/AssetManager/EditorAssetManager.h"
#include "FlareEditor/ImGui/ImGuiLayer.h"
#include "FlareEditor/EditorLayer.h"

#include "FlarePlatform//Events.h"

#include <imgui.h>
#include <imgui_internal.h>
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
		: ViewportWindow("Scene Viewport", true), m_Camera(camera)
	{
		m_SelectionOutlineShader = Shader::Create("assets/Shaders/SelectionOutline.glsl");
		m_GridMaterial = CreateRef<Material>(Shader::Create("assets/Shaders/Grid.glsl"));

		m_GridMaterial->Features.Culling = CullingMode::None;

		Ref<Shader> gridShader = m_GridMaterial->GetShader();
		s_GridPropertyIndices.Color = gridShader->GetPropertyIndex("u_Data.Color").value_or(UINT32_MAX);
		s_GridPropertyIndices.Offset = gridShader->GetPropertyIndex("u_Data.Offset").value_or(UINT32_MAX);
		s_GridPropertyIndices.Scale = gridShader->GetPropertyIndex("u_Data.GridScale").value_or(UINT32_MAX);
		s_GridPropertyIndices.Thickness = gridShader->GetPropertyIndex("u_Data.Thickness").value_or(UINT32_MAX);
		s_GridPropertyIndices.CellScale = gridShader->GetPropertyIndex("u_Data.CellScale").value_or(UINT32_MAX);
		s_GridPropertyIndices.FallOffThreshold = gridShader->GetPropertyIndex("u_Data.FallOffThreshold").value_or(UINT32_MAX);
	}

	void SceneViewportWindow::OnRenderViewport()
	{
		if (Scene::GetActive() == nullptr || !ShowWindow || !m_IsVisible)
			return;

		if (m_Viewport.FrameData.IsEditorCamera)
		{
			m_Viewport.FrameData.Camera.View = m_Camera.GetViewMatrix();
			m_Viewport.FrameData.Camera.Projection = m_Camera.GetProjectionMatrix();
			m_Viewport.FrameData.Camera.InverseProjection = glm::inverse(m_Viewport.FrameData.Camera.Projection);
			m_Viewport.FrameData.Camera.CalculateViewProjection();
			m_Viewport.FrameData.Camera.Position = m_Camera.GetPosition();
		}

		PrepareViewport();

		Ref<Scene> scene = Scene::GetActive();
		if (m_Viewport.GetSize() != glm::ivec2(0))
		{
			std::optional<SystemGroupId> debugRenderingGroup = scene->GetECSWorld().GetSystemsManager().FindGroup("Debug Rendering");

			m_Viewport.RenderTarget = m_ScreenBuffer;
			scene->OnBeforeRender(m_Viewport);

			Renderer::BeginScene(m_Viewport);
			m_ScreenBuffer->Bind();
			OnClear();

			RenderGrid();

			scene->OnRender(m_Viewport);
			
			m_ScreenBuffer->Unbind();
			m_Viewport.RenderTarget = m_FinalImageBuffer;
			m_Viewport.RenderTarget->Blit(m_ScreenBuffer, 0, 0);
			m_Viewport.RenderTarget->Bind();

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
			if (selectedEntity != Entity())
			{
				m_ScreenBuffer->BindAttachmentTexture(1);

				ImVec4 primaryColor = ImGuiTheme::Primary;
				glm::vec4 selectionColor = glm::vec4(primaryColor.x, primaryColor.y, primaryColor.z, 1.0f);

				m_SelectionOutlineShader->Bind();
				m_SelectionOutlineShader->SetInt("u_Outline.SelectedId", (int32_t)selectedEntity.GetIndex());
				m_SelectionOutlineShader->SetInt("u_IdsTexture", 0);
				m_SelectionOutlineShader->SetFloat4("u_Outline.Color", selectionColor);
				m_SelectionOutlineShader->SetFloat2("u_Outline.Thickness", glm::vec2(4.0f) / (glm::vec2)m_Viewport.GetSize() / 2.0f);

				RenderCommand::DrawIndexed(Renderer::GetFullscreenQuad());
			}

			m_Viewport.RenderTarget->Unbind();

			Renderer::EndScene();
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
			RenderWindowContents();

		EndImGui();
	}

	void SceneViewportWindow::OnEvent(Event& event)
	{
		if (!m_IsFocused)
			return;

		m_Camera.ProcessEvents(event);

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
	}

	void SceneViewportWindow::CreateFrameBuffer()
	{
		FrameBufferSpecifications specifications(m_Viewport.GetSize().x, m_Viewport.GetSize().y, {
			{ FrameBufferTextureFormat::RGB8, TextureWrap::Clamp, TextureFiltering::Closest },
			});

		FrameBufferSpecifications screenBufferSpecs(m_Viewport.GetSize().x, m_Viewport.GetSize().y, {
			{ FrameBufferTextureFormat::RGB8, TextureWrap::Clamp, TextureFiltering::Closest },
			{ FrameBufferTextureFormat::RedInteger, TextureWrap::Clamp, TextureFiltering::Closest },
			{ FrameBufferTextureFormat::Depth, TextureWrap::Clamp, TextureFiltering::Closest },
			});

		m_FinalImageBuffer = FrameBuffer::Create(specifications);
		m_Viewport.RenderTarget = m_FinalImageBuffer;
		m_ScreenBuffer = FrameBuffer::Create(screenBufferSpecs);
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
			EndImGui();
			return;
		}

		World& world = Scene::GetActive()->GetECSWorld();
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_PAYLOAD_NAME))
			{
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
						Entity entity = GetEntityUnderCursor();
						if (!world.IsEntityAlive(entity))
							break;

						std::optional<SpriteComponent*> sprite = world.TryGetEntityComponent<SpriteComponent>(entity);
						if (sprite.has_value())
							sprite.value()->Texture = handle;

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

		if (!ImGuizmo::IsUsingAny())
		{
			if (io.MouseClicked[ImGuiMouseButton_Left] && m_Viewport.RenderTarget != nullptr && m_IsHovered && m_RelativeMousePosition.x >= 0 && m_RelativeMousePosition.y >= 0)
				EditorLayer::GetInstance().Selection.SetEntity(GetEntityUnderCursor());
		}
	}

	void SceneViewportWindow::RenderGrid()
	{
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
