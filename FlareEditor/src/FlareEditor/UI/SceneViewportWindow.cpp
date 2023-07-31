#include "SceneViewportWindow.h"

#include "Flare/Renderer/RenderCommand.h"
#include "Flare/Scene/Components.h"
#include "Flare/Math/Math.h"

#include "Flare/Input/InputManager.h"

#include "FlareEditor/EditorContext.h"
#include "FlareEditor/AssetManager/EditorAssetManager.h"
#include "FlareEditor/EditorLayer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>

namespace Flare
{
	void SceneViewportWindow::OnRenderViewport()
	{
		if (m_RenderData.IsEditorCamera)
		{
			m_RenderData.Camera.ViewMatrix = m_Camera.GetViewMatrix();
			m_RenderData.Camera.ProjectionMatrix = m_Camera.GetProjectionMatrix();
			m_RenderData.Camera.CalculateViewProjection();
		}

		ViewportWindow::OnRenderViewport();
		
	}

	void SceneViewportWindow::OnViewportResize()
	{
		m_Camera.RecalculateProjection(m_RenderData.ViewportSize);
	}

	void SceneViewportWindow::OnRenderImGui()
	{
		BeginImGui();

		World& world = EditorContext::GetActiveScene()->GetECSWorld();
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
						EditorContext::OpenScene(handle);
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
		if (io.MouseClicked[ImGuiMouseButton_Left] && m_FrameBuffer != nullptr && m_IsHovered && m_RelativeMousePosition.x >= 0 && m_RelativeMousePosition.y >= 0)
			EditorContext::Instance.SelectedEntity = GetEntityUnderCursor();

		if (world.IsEntityAlive(EditorContext::Instance.SelectedEntity) && EditorContext::Instance.Gizmo != GizmoMode::None)
		{
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();

			ImVec2 windowPosition = ImGui::GetWindowPos();
			ImGuizmo::SetRect(windowPosition.x + m_ViewportOffset.x, 
				windowPosition.y + m_ViewportOffset.y, 
				(float)m_RenderData.ViewportSize.x, 
				(float) m_RenderData.ViewportSize.y);

			std::optional<TransformComponent*> transform = world.TryGetEntityComponent<TransformComponent>(EditorContext::Instance.SelectedEntity);
			if (transform.has_value())
			{
				glm::mat4 transformationMatrix = transform.value()->GetTransformationMatrix();

				// TODO: move snap values to editor settings
				float snapValue = 0.5f;

				ImGuizmo::OPERATION operation = (ImGuizmo::OPERATION)-1;
				switch (EditorContext::Instance.Gizmo)
				{
				case GizmoMode::Translate:
					operation = ImGuizmo::TRANSLATE;
					break;
				case GizmoMode::Rotate:
					snapValue = 5.0f;
					operation = ImGuizmo::ROTATE;
					break;
				case GizmoMode::Scale:
					operation = ImGuizmo::SCALE;
					break;
				default:
					FLARE_CORE_ASSERT("Unhandled Gizmo type");
				}

				ImGuizmo::MODE mode = ImGuizmo::WORLD;

				bool snappingEnabled = InputManager::IsKeyPressed(KeyCode::LeftControl) || InputManager::IsKeyPressed(KeyCode::RightControl);

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

		EndImGui();
	}

	void SceneViewportWindow::CreateFrameBuffer()
	{
		FrameBufferSpecifications specifications(m_RenderData.ViewportSize.x, m_RenderData.ViewportSize.y, {
			{ FrameBufferTextureFormat::RGB8, TextureWrap::Clamp, TextureFiltering::Closest },
			{ FrameBufferTextureFormat::RedInteger, TextureWrap::Clamp, TextureFiltering::Closest },
		});

		m_FrameBuffer = FrameBuffer::Create(specifications);
	}

	void SceneViewportWindow::OnClear()
	{
		RenderCommand::Clear();
		m_FrameBuffer->ClearAttachment(1, INT32_MAX);
	}

	Entity SceneViewportWindow::GetEntityUnderCursor() const
	{
		m_FrameBuffer->Bind();

		int32_t entityIndex;
		m_FrameBuffer->ReadPixel(1, m_RelativeMousePosition.x, m_RelativeMousePosition.y, &entityIndex);

		std::optional<Entity> entity = EditorContext::GetActiveScene()->GetECSWorld().GetRegistry().FindEntityByIndex(entityIndex);

		m_FrameBuffer->Unbind();

		return entity.value_or(Entity());
	}
}
