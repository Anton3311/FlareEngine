#include "EditorLayer.h"

#include "Flare.h"
#include "Flare/Core/Application.h"

#include "Flare/Renderer2D/Renderer2D.h"

#include "FlareECS/Registry.h"
#include "FlareECS/Query/EntityView.h"
#include "FlareECS/Query/EntityViewIterator.h"
#include "FlareECS/Query/EntityRegistryIterator.h"
#include "FlareECS/Query/EntityArchetypesView.h"
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

	struct TagComponent
	{
		static ComponentId Id;

		const char* Name;
	};

	size_t TestComponent::Id = 0;
	size_t TransformComponent::Id = 0;
	size_t TagComponent::Id = 0;

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



		{
			TestComponent::Id = m_World.GetRegistry().RegisterComponent(typeid(TestComponent).name(), sizeof(TestComponent));
			TransformComponent::Id = m_World.GetRegistry().RegisterComponent(typeid(TransformComponent).name(), sizeof(TransformComponent));
			TagComponent::Id = m_World.GetRegistry().RegisterComponent(typeid(TagComponent).name(), sizeof(TagComponent));

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

		{
			m_TestEntity = m_World.GetRegistry().CreateEntity(ComponentSet(&TestComponent::Id, 1));
		}

		{
			m_Query = m_World.GetRegistry().CreateQuery(ComponentSet(&TransformComponent::Id, 1));
		}

		{
			m_World.RegisterSystem(m_World.GetRegistry().CreateQuery(ComponentSet(&TransformComponent::Id, 1)), [](EntityView view) -> void
			{
				ComponentView<TransformComponent> transforms = view.View<TransformComponent>();
				for (EntityViewElement entity : view)
				{
					TransformComponent& transform = transforms[entity];
					Renderer2D::DrawQuad(transform.Position, glm::vec2(0.4f), glm::vec4(1.0f));
				}
			});
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

		m_World.OnUpdate();

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

			if (ImGui::Button("Create Entity"))
			{
				m_TestEntity = m_World.GetRegistry().CreateEntity(ComponentSet(&TestComponent::Id, 1));
			}

			if (ImGui::CollapsingHeader("Registry"))
			{
				for (Entity entity : m_World.GetRegistry())
				{
					ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding;
					bool opened = ImGui::TreeNodeEx((void*)std::hash<Entity>()(entity), flags, "Entity");

					if (opened)
					{
						std::optional<ComponentId> removedComponent = {};
						for (ComponentId component : m_World.GetRegistry().GetEntityComponents(entity))
						{
							if (!removedComponent.has_value() && ImGui::BeginPopupContextWindow())
							{
								if (ImGui::MenuItem("Remove component"))
								{
									removedComponent = component;
								}

								ImGui::EndMenu();
							}

							ImGui::Separator();
							ImGui::Text("%s", m_World.GetRegistry().GetComponentInfo(component).Name.c_str());
							DrawComponent(entity, component);
						}

						if (removedComponent.has_value())
							m_World.GetRegistry().RemoveEntityComponent(entity, removedComponent.value());

						ImGui::TreePop();
					}
				}
			}

			if (ImGui::CollapsingHeader("Components"))
			{
				const auto& components = m_World.GetRegistry().GetRegisteredComponents();
				for (const auto& component : components)
				{
					ImGui::Separator();
					ImGui::Text("Id: %d", component.Id);
					ImGui::Text("Name: %s", component.Name.c_str());
					ImGui::Text("Size: %d", component.Size);
				}
			}

			if (ImGui::Button("Add transform component"))
			{
				TransformComponent transform = TransformComponent{ glm::vec3(2.0f, 0.0f, 0.0f) };
				m_World.GetRegistry().AddEntityComponent(m_TestEntity, TransformComponent::Id, &transform);
			}

			if (ImGui::Button("Add tag component"))
			{
				TagComponent tag = TagComponent{ "Tag" };
				m_World.GetRegistry().AddEntityComponent(m_TestEntity, TagComponent::Id, &tag);
			}

			ImGui::End();

			{
				ImGui::Begin("ECS Query");

				EntityArchetypesView view = m_World.GetRegistry().ExecuteQuery(m_Query);
				for (EntityView entityView : view)
				{
					ComponentView<TransformComponent> transforms = entityView.View<TransformComponent>();

					for (EntityViewElement entity : entityView)
					{
						ImGui::PushID(entity.GetEntityData());
						ImGui::Separator();
						ImGui::DragFloat3("Position", glm::value_ptr(transforms[entity].Position));
						ImGui::PopID();
					}
				}

				ImGui::End();
			}
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

	void EditorLayer::DrawComponent(Entity entity, ComponentId componentId)
	{
		if (componentId == TestComponent::Id)
		{
			TestComponent& testComponent = *(TestComponent*)m_World.GetRegistry().GetEntityComponent(entity, componentId).value();
			ImGui::DragFloat("Float A", &testComponent.FloatA);
			ImGui::DragFloat4("Vec", glm::value_ptr(testComponent.Vec));
		}
		else if (componentId == TransformComponent::Id)
		{
			TransformComponent& transform = *(TransformComponent*)m_World.GetRegistry().GetEntityComponent(entity, componentId).value();
			ImGui::DragFloat3("Position", glm::value_ptr(transform.Position));
		}
		else if (componentId == TagComponent::Id)
		{
			TagComponent& tag = *(TagComponent*)m_World.GetRegistry().GetEntityComponent(entity, componentId).value();
			ImGui::Text("%s", tag.Name);
		}
	}
}