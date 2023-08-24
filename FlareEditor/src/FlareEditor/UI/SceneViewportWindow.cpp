#include "SceneViewportWindow.h"

#include "Flare/Renderer/RenderCommand.h"
#include "Flare/Scene/Components.h"
#include "Flare/Scene/Scene.h"
#include "Flare/Math/Math.h"

#include "Flare/Input/InputManager.h"

#include "FlareEditor/AssetManager/EditorAssetManager.h"
#include "FlareEditor/ImGui/ImGuiLayer.h"
#include "FlareEditor/EditorLayer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>

namespace Flare
{
	SceneViewportWindow::SceneViewportWindow(EditorCamera& camera)
		: ViewportWindow("Scene Viewport", true), m_Camera(camera)
	{
		m_SelectionOutlineShader = Shader::Create("assets/Shaders/SelectionOutline.glsl");
		
		float vertices[] = {
			-1, -1,
			-1,  1,
			 1,  1,
			 1, -1,
		};

		uint32_t indices[] = {
			0, 1, 2,
			2, 0, 3,
		};

		Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(sizeof(vertices), (const void*)vertices);
		vertexBuffer->SetLayout({
			BufferLayoutElement("i_Position", ShaderDataType::Float2),
		});

		m_FullscreenQuad = VertexArray::Create();
		m_FullscreenQuad->SetIndexBuffer(IndexBuffer::Create(6, (const void*)indices));
		m_FullscreenQuad->AddVertexBuffer(vertexBuffer);
		m_FullscreenQuad->Unbind();
	}

	void SceneViewportWindow::OnRenderViewport()
	{
		if (m_RenderData.IsEditorCamera)
		{
			m_RenderData.Camera.ViewMatrix = m_Camera.GetViewMatrix();
			m_RenderData.Camera.ProjectionMatrix = m_Camera.GetProjectionMatrix();
			m_RenderData.Camera.CalculateViewProjection();
		}

		PrepareViewport();

		if (m_RenderData.Viewport.Size != glm::ivec2(0))
		{
			m_ScreenBuffer->Bind();
			OnClear();

			Scene::GetActive()->OnBeforeRender(m_RenderData);
			Scene::GetActive()->OnRender(m_RenderData);

			m_ScreenBuffer->Unbind();

			m_FrameBuffer->Blit(m_ScreenBuffer, 0, 0);

			Entity selectedEntity = EditorLayer::GetInstance().GetSelectedEntity();
			if (selectedEntity != Entity())
			{
				m_FrameBuffer->Bind();
				m_ScreenBuffer->BindAttachmentTexture(1);

				ImVec4 primaryColor = ImGuiTheme::Primary;
				glm::vec4 selectionColor = glm::vec4(primaryColor.x, primaryColor.y, primaryColor.z, 1.0f);

				m_SelectionOutlineShader->Bind();
				m_SelectionOutlineShader->SetInt("u_SelectedId", (int32_t)selectedEntity.GetIndex());
				m_SelectionOutlineShader->SetInt("u_IdsTexture", 0);
				m_SelectionOutlineShader->SetFloat4("u_OutlineColor", selectionColor);
				m_SelectionOutlineShader->SetFloat2("u_OutlineThickness", glm::vec2(4.0f) / (glm::vec2)m_RenderData.Viewport.Size / 2.0f);

				RenderCommand::DrawIndexed(m_FullscreenQuad);
				m_FrameBuffer->Unbind();
			}
		}
	}

	void SceneViewportWindow::OnViewportChanged()
	{
		m_ScreenBuffer->Resize(m_RenderData.Viewport.Size.x, m_RenderData.Viewport.Size.y);
		m_Camera.OnViewportChanged(m_RenderData.Viewport);
	}

	void SceneViewportWindow::OnRenderImGui()
	{
		BeginImGui();

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
					}
				}

				ImGui::EndDragDropTarget();
			}
		}

		ImGuiIO& io = ImGui::GetIO();
		Entity selectedEntity = EditorLayer::GetInstance().GetSelectedEntity();
		if (world.IsEntityAlive(selectedEntity) && EditorLayer::GetInstance().GetGuizmoMode() != GuizmoMode::None)
		{
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();

			ImVec2 windowPosition = ImGui::GetWindowPos();
			ImGuizmo::SetRect(windowPosition.x + m_ViewportOffset.x, 
				windowPosition.y + m_ViewportOffset.y, 
				(float)m_RenderData.Viewport.Size.x, 
				(float) m_RenderData.Viewport.Size.y);

			std::optional<TransformComponent*> transform = world.TryGetEntityComponent<TransformComponent>(selectedEntity);
			if (transform.has_value())
			{
				glm::mat4 transformationMatrix = transform.value()->GetTransformationMatrix();

				// TODO: move snap values to editor settings
				float snapValue = 0.5f;

				ImGuizmo::OPERATION operation = (ImGuizmo::OPERATION)-1;
				switch (EditorLayer::GetInstance().GetGuizmoMode())
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
			if (io.MouseClicked[ImGuiMouseButton_Left] && m_FrameBuffer != nullptr && m_IsHovered && m_RelativeMousePosition.x >= 0 && m_RelativeMousePosition.y >= 0)
				EditorLayer::GetInstance().SetSelectedEntity(GetEntityUnderCursor());
		}

		EndImGui();
	}

	void SceneViewportWindow::CreateFrameBuffer()
	{
		FrameBufferSpecifications specifications(m_RenderData.Viewport.Size.x, m_RenderData.Viewport.Size.y, {
			{ FrameBufferTextureFormat::RGB8, TextureWrap::Clamp, TextureFiltering::Closest },
		});

		FrameBufferSpecifications screenBufferSpecs(m_RenderData.Viewport.Size.x, m_RenderData.Viewport.Size.y, {
			{ FrameBufferTextureFormat::RGB8, TextureWrap::Clamp, TextureFiltering::Closest },
			{ FrameBufferTextureFormat::RedInteger, TextureWrap::Clamp, TextureFiltering::Closest },
			{ FrameBufferTextureFormat::Depth, TextureWrap::Clamp, TextureFiltering::Closest },
		});

		m_FrameBuffer = FrameBuffer::Create(specifications);
		m_ScreenBuffer = FrameBuffer::Create(screenBufferSpecs);
	}

	void SceneViewportWindow::OnClear()
	{
		RenderCommand::Clear();
		m_ScreenBuffer->ClearAttachment(1, INT32_MAX);
	}

	Entity SceneViewportWindow::GetEntityUnderCursor() const
	{
		m_ScreenBuffer->Bind();

		int32_t entityIndex;
		m_ScreenBuffer->ReadPixel(1, m_RelativeMousePosition.x, m_RelativeMousePosition.y, &entityIndex);

		std::optional<Entity> entity = Scene::GetActive()->GetECSWorld().GetRegistry().FindEntityByIndex(entityIndex);

		m_ScreenBuffer->Unbind();

		return entity.value_or(Entity());
	}
}
