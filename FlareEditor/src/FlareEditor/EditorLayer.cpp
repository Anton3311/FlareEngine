#include "EditorLayer.h"

#include "Flare.h"
#include "Flare/Core/Application.h"
#include "Flare/Renderer2D/Renderer2D.h"
#include "Flare/Scene/SceneSerializer.h"

#include "Flare/AssetManager/AssetManager.h"

#include "FlareEditor/EditorContext.h"
#include "FlareEditor/AssetManager/EditorAssetManager.h"

#include <imgui.h>

namespace Flare
{
	EditorLayer::EditorLayer()
		: Layer("EditorLayer")
	{
	}

	void EditorLayer::OnAttach()
	{
		AssetManager::Intialize(CreateRef<EditorAssetManager>(std::filesystem::current_path()));

		Ref<Window> window = Application::GetInstance().GetWindow();
		uint32_t width = window->GetProperties().Width;
		uint32_t height = window->GetProperties().Height;

		FrameBufferSpecifications specifications(width, height, {
			{ FrameBufferTextureFormat::RGB8, TextureWrap::Clamp, TextureFiltering::NoFiltering }
		});

		m_FrameBuffer = FrameBuffer::Create(specifications);

		RenderCommand::SetClearColor(0.04f, 0.07f, 0.1f, 1.0f);

		EditorContext::Instance.ActiveScene = CreateRef<Scene>();
	}

	void EditorLayer::OnUpdate(float deltaTime)
	{
		m_PreviousFrameTime = deltaTime;

		const FrameBufferSpecifications& specs = m_FrameBuffer->GetSpecifications();
		if (m_ViewportSize != glm::i32vec2(0.0f) && (specs.Width != (uint32_t)m_ViewportSize.x || specs.Height != (uint32_t)m_ViewportSize.y))
		{
			RenderCommand::SetViewport(0, 0, (uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			EditorContext::Instance.ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

			m_FrameBuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		}

		m_FrameBuffer->Bind();
		RenderCommand::Clear();

		Renderer2D::ResetStats();

		EditorContext::Instance.ActiveScene->OnUpdateRuntime();

		m_FrameBuffer->Unbind();
	}

	void EditorLayer::OnEvent(Event& event)
	{
	}

	void EditorLayer::OnImGUIRender()
	{
		static bool fullscreen = true;
		static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (fullscreen)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}
		else
		{
			dockspaceFlags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}

		if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
			windowFlags |= ImGuiWindowFlags_NoBackground;

		static bool open = true;
		ImGui::Begin("DockSpace", &open, windowFlags);

		ImGuiID dockspaceId = ImGui::GetID("DockSpace");
		ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Scene"))
			{
				if (ImGui::MenuItem("Open Scene"))
					SceneSerializer::Deserialize(EditorContext::Instance.ActiveScene, "Scene.flare");
				if (ImGui::MenuItem("Save Scene"))
					SceneSerializer::Serialize(EditorContext::Instance.ActiveScene, "Scene.flare");
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		{
			ImGui::Begin("Renderer 2D");

			const auto& stats = Renderer2D::GetStats();
			ImGui::Text("Quads %d", stats.QuadsCount);
			ImGui::Text("Draw Calls %d", stats.DrawCalls);
			ImGui::Text("Vertices %d", stats.GetTotalVertexCount());

			ImGui::Text("Frame time %f", m_PreviousFrameTime);
			ImGui::Text("FPS %f", 1.0f / m_PreviousFrameTime);

			ImGui::End();
		}

		{
			ImGui::Begin("Settings");

			Ref<Window> window = Application::GetInstance().GetWindow();

			bool vsync = window->GetProperties().VSyncEnabled;
			if (ImGui::Checkbox("VSync", &vsync))
				window->SetVSync(vsync);

			ImGui::End();
		}

		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin("Viewport");

			ImVec2 windowSize = ImGui::GetContentRegionAvail();
			glm::i32vec2 newViewportSize = glm::i32vec2((uint32_t)windowSize.x, (uint32_t)windowSize.y);

			if (newViewportSize != m_ViewportSize)
			{
				EditorContext::Instance.ActiveScene->OnViewportResize(newViewportSize.x, newViewportSize.y);
				m_ViewportSize = newViewportSize;
			}

			const FrameBufferSpecifications frameBufferSpecs = m_FrameBuffer->GetSpecifications();
			ImVec2 imageSize = ImVec2(frameBufferSpecs.Width, frameBufferSpecs.Height);
			ImGui::Image((ImTextureID)m_FrameBuffer->GetColorAttachmentRendererId(0), windowSize);

			ImGui::End();
			ImGui::PopStyleVar();
		}

		m_SceneWindow.OnImGuiRender();
		m_PropertiesWindow.OnImGuiRender();
		m_AssetManagerWindow.OnImGuiRender();

		ImGui::End();
	}
}