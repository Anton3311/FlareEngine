#include "PCH.h"

#include "ViewportWindow.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Core/Application.h"

#include "Flare/DebugRenderer/DebugRenderer.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/Passes/BlitPass.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererComponents.h"

#include "Flare/Renderer2D/Renderer2D.h"

#include "Flare/Scene/Scene.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"

#include "FlarePlatform/Event.h"

namespace Flare
{
	ViewportWindow::ViewportWindow(const Scope<SceneRenderer>& sceneRenderer, std::string_view name)
		: m_Name(name),
		m_SceneRenderer(sceneRenderer),
		m_IsFocused(false),
		m_IsVisible(true),
		m_PreviousFocusState(false),
		m_IsHovered(false),
		ShowWindow(true),
		m_RelativeMousePosition(glm::ivec2(0)),
		m_ViewportOffset(glm::uvec2(0))
	{
		m_ViewportEntity = Renderer::CreateViewport();
	}

	ViewportWindow::~ViewportWindow()
	{
		Renderer::DeleteViewport(m_ViewportEntity);
	}

	void ViewportWindow::OnRenderViewport(const World& renderWorld)
	{
		FLARE_PROFILE_FUNCTION();

		if (!ShowWindow || !m_IsVisible)
			return;

		Ref<Scene> scene = GetScene();
		if (scene == nullptr)
			return;

		m_SceneRenderer->RenderViewport(m_ViewportEntity, nullptr, FLARE_BIND_EVENT_CALLBACK(BuildRenderGraph));
	}

	void ViewportWindow::OnAddRenderPasses(RenderGraph& renderGraph)
	{
	}

	void ViewportWindow::SetMaximized(bool maximized)
	{
		m_Maximized = maximized;
	}

	Ref<Scene> ViewportWindow::GetScene() const
	{
		return m_Scene == nullptr ? Scene::GetActive() : m_Scene;
	}

	void ViewportWindow::BeginImGui()
	{
		FLARE_PROFILE_FUNCTION();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

		if (m_Maximized)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		m_IsVisible = ImGui::Begin(m_Name.c_str(), &ShowWindow, windowFlags);

		// HACK: If window's title bar isn't hovered, disable window moving by dragging anywhere inside the window.
		//       When the window wasn't docket it used to interfere with the camera controller and gizmos,
		//       as moving a camera or using a guizmo was also moving a window.
		if (!ImGui::IsWindowDocked())
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			ImRect titleBarRect = window->TitleBarRect();
			bool titleBarIsHovered = ImGui::IsMouseHoveringRect(titleBarRect.Min, titleBarRect.Max, false);

			if (!titleBarIsHovered)
			{
				window->Flags |= ImGuiWindowFlags_NoMove;
			}
		}

		if (m_WindowFocusRequested)
		{
			ImGui::FocusWindow(ImGui::GetCurrentWindow());
			m_WindowFocusRequested = false;
		}

		m_PreviousFocusState = m_IsFocused;

		m_IsHovered = ImGui::IsWindowHovered();
		m_IsFocused = ImGui::IsWindowFocused();

		if (!m_IsVisible)
			return;

		ImGuiIO& io = ImGui::GetIO();

		Ref<Window> window = Application::GetInstance().GetWindow();

		ImVec2 cursorPosition = ImGui::GetCursorPos();
		ImVec2 windowPosition = ImGui::GetWindowPos();
		ImVec2 windowSize = ImGui::GetContentRegionAvail();
		glm::ivec2 newViewportSize = glm::u32vec2((int32_t)windowSize.x, (int32_t)windowSize.y);
		m_ViewportOffset = glm::ivec2(cursorPosition.x, cursorPosition.y);

		m_RelativeMousePosition = glm::ivec2(
			(int32_t)(io.MousePos.x - windowPosition.x),
			(int32_t)(io.MousePos.y - windowPosition.y)) - m_ViewportOffset;

		m_RelativeMousePosition.y = newViewportSize.y - m_RelativeMousePosition.y;

		bool changed = false;

		World& renderWorld = Renderer::GetRenderWorld();
		Viewport& viewport = renderWorld.GetEntityComponent<Viewport>(m_ViewportEntity);

		glm::ivec2 viewportSize = (glm::ivec2)viewport.Size;
		glm::ivec2 viewportPosition = (glm::ivec2)viewport.Position;

		if (viewportSize == glm::ivec2(0))
		{
			viewportSize = newViewportSize;
			changed = true;
		}
		else if (newViewportSize != viewportSize)
		{
			viewportSize = newViewportSize;
			changed = true;
		}

		glm::ivec2 position = glm::ivec2(windowPosition.x, windowPosition.y) + m_ViewportOffset - (glm::ivec2)window->GetProperties().Position;
		if (position != viewportPosition)
		{
			viewportPosition = position;
			changed = true;
		}

		changed |= viewportSize != viewportSize;

		viewport.Position = viewportPosition;
		viewport.Size = viewportSize;

		if (changed)
			OnViewportChanged();
	}

	void ViewportWindow::RenderViewportBuffer(const Ref<Texture>& texture)
	{
		FLARE_PROFILE_FUNCTION();
		ImVec2 windowSize = ImGui::GetContentRegionAvail();

		const World& renderWorld = Renderer::GetRenderWorld();
		const Viewport& viewport = renderWorld.GetEntityComponent<const Viewport>(m_ViewportEntity);

		ImVec2 imageSize = ImVec2((float)viewport.Size.x, (float)viewport.Size.y);
		ImGui::Image(ImGuiLayer::GetId(texture), windowSize, ImVec2(0, 1), ImVec2(1, 0));
	}

	void ViewportWindow::EndImGui()
	{
		FLARE_PROFILE_FUNCTION();
		ImGui::End();
		ImGui::PopStyleVar(2); // Pop window padding & border size
	}

	void ViewportWindow::OnViewportChanged()
	{
	}

	void ViewportWindow::BuildRenderGraph(RenderGraph& renderGraph)
	{
		FLARE_PROFILE_FUNCTION();

		OnAddRenderPasses(renderGraph);
	}

	void ViewportWindow::OnAttach()
	{
		FLARE_PROFILE_FUNCTION();
	}

	void ViewportWindow::OnRenderImGui()
	{
		FLARE_PROFILE_FUNCTION();
		if (!ShowWindow)
			return;

		BeginImGui();

		const World& renderWorld = Renderer::GetRenderWorld();
		const RenderGraph& renderGraph = *renderWorld.GetEntityComponent<const ViewportRenderGraph>(m_ViewportEntity).Graph;
		RenderGraphTextureId colorTexture = renderWorld.GetEntityComponent<const ViewportColorOutput>(m_ViewportEntity).Id;

		if (renderGraph.GetResourceManager().IsTextureIdValid(colorTexture))
		{
			RenderViewportBuffer(renderGraph.GetTexture(colorTexture));
		}

		EndImGui();
	}
}
