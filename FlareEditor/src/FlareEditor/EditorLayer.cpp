#include "EditorLayer.h"

#include "Flare.h"
#include "Flare/Core/Application.h"

#include "Flare/Renderer2D/Renderer2D.h"

#include "FlareECS/Query/EntityView.h"
#include "FlareECS/Query/EntityViewIterator.h"
#include "FlareECS/Query/ComponentView.h"

#include <imgui.h>

namespace Flare
{
	struct TestComponent
	{
		static ComponentId Id;

		float FloatA;
		glm::vec4 Vec;
	};

	struct TransformComponent
	{
		static ComponentId Id;

		glm::vec3 Position;
	};

	size_t TestComponent::Id = 0;
	size_t TransformComponent::Id = 0;

	EditorLayer::EditorLayer()
		: Layer("EditorLayer")
	{
	}

	void EditorLayer::OnAttach()
	{
		m_QuadShader = Shader::Create("QuadShader.glsl");

		Ref<Window> window = Application::GetInstance().GetWindow();
		uint32_t width = window->GetProperties().Width;
		uint32_t height = window->GetProperties().Height;

		FrameBufferSpecifications specifications(width, height, {
			{FrameBufferTextureFormat::RGB8, TextureWrap::Clamp, TextureFiltering::NoFiltering }
		});

		m_FrameBuffer = FrameBuffer::Create(specifications);

		RenderCommand::SetClearColor(0.04f, 0.07f, 0.1f, 1.0f);

		CalculateProjection(m_CameraSize);



		TestComponent::Id = m_World.GetRegistry().RegisterComponent(typeid(TestComponent).name(), sizeof(TestComponent));
		TransformComponent::Id = m_World.GetRegistry().RegisterComponent(typeid(TransformComponent).name(), sizeof(TransformComponent));

		ComponentId components[] = { TestComponent::Id, TransformComponent::Id };
		Entity ents[2];
		ents[0] = m_World.GetRegistry().CreateEntity(ComponentSet(components, 2));
		ents[1] = m_World.GetRegistry().CreateEntity(ComponentSet(components, 2));

		for (uint32_t i = 0; i < 2; i++)
		{
			std::optional<void*> result = m_World.GetRegistry().GetEntityComponent(ents[i], TestComponent::Id);

			TestComponent& component = *(TestComponent*)result.value();
			component.FloatA = 10.0f + (float) i;
			component.Vec = glm::vec4(1.0f, 0.4f, 0.1f, 0.9f);

			std::optional<void*> transformResult = m_World.GetRegistry().GetEntityComponent(ents[i], TransformComponent::Id);
			TransformComponent& transform = *(TransformComponent*)transformResult.value();
			transform.Position = glm::vec3(0.0f, 1.0f, i);
		}
	}

	void EditorLayer::OnUpdate(float deltaTime)
	{
		m_PreviousFrameTime = deltaTime;

		const FrameBufferSpecifications& specs = m_FrameBuffer->GetSpecifications();
		if (m_ViewportSize != glm::i32vec2(0.0f) && (specs.Width != (uint32_t)m_ViewportSize.x || specs.Height != (uint32_t)m_ViewportSize.y))
		{
			RenderCommand::SetViewport(0, 0, (uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_FrameBuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			CalculateProjection(m_CameraSize);
		}

		m_FrameBuffer->Bind();
		RenderCommand::Clear();

		Renderer2D::ResetStats();
		Renderer2D::Begin(m_QuadShader, m_ProjectionMatrix);

		glm::vec4 cornerColors[4] =
		{
			glm::vec4(0.94f, 0.15f, 0.09f, 1.0f),
			glm::vec4(0.13f, 0.12f, 0.98f, 1.0f),
		};

		for (int32_t y = 0; y < m_Height; y++)
		{
			for (int32_t x = 0; x < m_Width; x++)
			{
				glm::vec4 color0 = glm::lerp(cornerColors[0], cornerColors[1], (float)x / (float)m_Width);
				glm::vec4 color1 = glm::lerp(cornerColors[0], cornerColors[1], (float)y / (float)m_Height);

				Renderer2D::DrawQuad(glm::vec3(x - m_Width / 2, y - m_Height / 2, 0), glm::vec2(0.8f), (color0 + color1) / 2.0f);
			}
		}

		Renderer2D::End();
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
		ImGui::Begin("DockSpaceTest", &open, windowFlags);

		ImGuiID dockspaceId = ImGui::GetID("DockSpace");
		ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);

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
			{
				window->SetVSync(vsync);
			}

			ImGui::DragInt("Width", &m_Width, 1, 100);
			ImGui::DragInt("Height", &m_Height);

			if (ImGui::SliderFloat("Camera Size", &m_CameraSize, 1.0f, 100.0f))
				CalculateProjection(m_CameraSize);

			ImGui::End();
		}

		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin("Viewport");

			ImVec2 windowSize = ImGui::GetContentRegionAvail();
			m_ViewportSize = glm::i32vec2((uint32_t)windowSize.x, (uint32_t)windowSize.y);

			const FrameBufferSpecifications frameBufferSpecs = m_FrameBuffer->GetSpecifications();
			ImVec2 imageSize = ImVec2(frameBufferSpecs.Width, frameBufferSpecs.Height);
			ImGui::Image((ImTextureID)m_FrameBuffer->GetColorAttachmentRendererId(0), windowSize);

			ImGui::End();
			ImGui::PopStyleVar();
		}

		{
			ImGui::Begin("ECS");

			ComponentId components[] = { TestComponent::Id, TransformComponent::Id };

			EntityView view = m_World.GetRegistry().View(ComponentSet(components, 2));
			ComponentView<TransformComponent> transforms = view.View<TransformComponent>();

			for (EntityViewElement ent : view)
			{
				TransformComponent& t = transforms[ent];

				if (ImGui::CollapsingHeader("Entity"))
				{
					ImGui::DragFloat3("Position", glm::value_ptr(t.Position));
				}
			}
			
			ImGui::End();
		}

		ImGui::End();
	}

	void EditorLayer::CalculateProjection(float size)
	{
		float width = m_ViewportSize.x;
		float height = m_ViewportSize.y;

		float halfSize = size / 2;
		float aspectRation = width / height;

		m_ProjectionMatrix = glm::ortho(-halfSize * aspectRation, halfSize * aspectRation, -halfSize, halfSize, -0.1f, 10.0f);
		m_InverseProjection = glm::inverse(m_ProjectionMatrix);
	}
}