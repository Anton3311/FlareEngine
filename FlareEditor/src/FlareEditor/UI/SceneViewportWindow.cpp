#include "SceneViewportWindow.h"

#include "Flare/Renderer/RenderCommand.h"

#include "FlareEditor/EditorContext.h"
#include "FlareEditor/AssetManager/EditorAssetManager.h"
#include "FlareEditor/EditorLayer.h"

#include <imgui.h>

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

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_PAYLOAD_NAME))
			{
				World& world = EditorContext::GetActiveScene()->GetECSWorld();

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

		EndImGui();
	}

	void SceneViewportWindow::CreateFrameBuffer()
	{
		FrameBufferSpecifications specifications(m_RenderData.ViewportSize.x, m_RenderData.ViewportSize.y, {
			{ FrameBufferTextureFormat::RGB8, TextureWrap::Clamp, TextureFiltering::NoFiltering },
			{ FrameBufferTextureFormat::RedInteger, TextureWrap::Clamp, TextureFiltering::NoFiltering },
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
