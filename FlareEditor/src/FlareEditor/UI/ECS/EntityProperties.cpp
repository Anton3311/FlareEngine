#include "EntityProperties.h"

#include "FlareEditor/UI/EditorGUI.h"

#include <imgui.h>

namespace Flare
{
	EntityProperties::EntityProperties(World& world)
		: m_World(world) {}

	void EntityProperties::OnRenderImGui(Entity entity)
	{
		if (!m_World.IsEntityAlive(entity))
			return;

		ImGui::BeginDisabled();
		ImGui::Text("Index %d Generation %d", entity.GetIndex(), entity.GetGeneration());
		ImGui::EndDisabled();

		RenderAddComponentMenu(entity);

		if (ImGui::BeginChild("Components"))
		{
			std::optional<ComponentId> removedComponent;
			for (ComponentId component : m_World.GetEntityComponents(entity))
			{
				const ComponentInfo& componentInfo = m_World.Components.GetComponentInfo(component);

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_FramePadding;
				if (ImGui::TreeNodeEx((void*)std::hash<ComponentId>()(component), flags, "%s", componentInfo.Name.c_str()))
				{
					if (component == COMPONENT_ID(CameraComponent))
						RenderCameraComponent(m_World.GetEntityComponent<CameraComponent>(entity));
					else if (component == COMPONENT_ID(TransformComponent))
						RenderTransformComponent(m_World.GetEntityComponent<TransformComponent>(entity));
					else if (component == COMPONENT_ID(SpriteComponent))
						RenderSpriteComponent(m_World.GetEntityComponent<SpriteComponent>(entity));
					else
					{
						std::optional<void*> componentData = m_World.Entities.GetEntityComponent(entity, component);
						if (componentData.has_value() && componentInfo.Initializer)
							EditorGUI::TypeEditor(componentInfo.Initializer->Type, (uint8_t*)componentData.value());
					}

					ImGui::TreePop();
				}

				if (ImGui::BeginPopupContextItem(componentInfo.Name.c_str()))
				{
					if (ImGui::MenuItem("Remove"))
						removedComponent = component;

					ImGui::End();
				}
			}

			if (removedComponent.has_value())
				m_World.Entities.RemoveEntityComponent(entity, removedComponent.value());

			ImGui::EndChild();
		}
	}


	void EntityProperties::RenderCameraComponent(CameraComponent& cameraComponent)
	{
		if (EditorGUI::BeginPropertyGrid())
		{
			EditorGUI::BeginToggleGroupProperty("Projection", 2);
			if (EditorGUI::ToggleGroupItem("Orthographic", cameraComponent.Projection == CameraComponent::ProjectionType::Orthographic))
				cameraComponent.Projection = CameraComponent::ProjectionType::Orthographic;
			if (EditorGUI::ToggleGroupItem("Perspective", cameraComponent.Projection == CameraComponent::ProjectionType::Perspective))
				cameraComponent.Projection = CameraComponent::ProjectionType::Perspective;

			EditorGUI::EndToggleGroup();

			if (cameraComponent.Projection == CameraComponent::ProjectionType::Orthographic)
				EditorGUI::FloatPropertyField("Size", cameraComponent.Size);
			else
				EditorGUI::FloatPropertyField("FOV", cameraComponent.FOV);

			EditorGUI::FloatPropertyField("Near", cameraComponent.Near);
			EditorGUI::FloatPropertyField("Far", cameraComponent.Far);

			EditorGUI::EndPropertyGrid();
		}
	}

	void EntityProperties::RenderTransformComponent(TransformComponent& transform)
	{
		if (EditorGUI::BeginPropertyGrid())
		{
			EditorGUI::Vector3PropertyField("Position", transform.Position);
			EditorGUI::Vector3PropertyField("Rotation", transform.Rotation);
			EditorGUI::Vector3PropertyField("Scale", transform.Scale);

			EditorGUI::EndPropertyGrid();
		}
	}

	void EntityProperties::RenderSpriteComponent(SpriteComponent& sprite)
	{
		if (EditorGUI::BeginPropertyGrid())
		{
			EditorGUI::ColorPropertyField("Color", sprite.Color);
			EditorGUI::AssetField("Texture", sprite.Texture);
			EditorGUI::Vector2PropertyField("Tiling", sprite.TextureTiling);

			EditorGUI::PropertyName("Flip");
			{
				bool flipX = HAS_BIT(sprite.Flags, SpriteRenderFlags::FlipX);
				bool flipY = HAS_BIT(sprite.Flags, SpriteRenderFlags::FlipY);

				ImGui::Checkbox("X", &flipX);
				ImGui::SameLine();
				ImGui::Checkbox("Y", &flipY);

				sprite.Flags = sprite.Flags & ~(SpriteRenderFlags::FlipX | SpriteRenderFlags::FlipY);

				if (flipX)
					sprite.Flags |= SpriteRenderFlags::FlipX;
				if (flipY)
					sprite.Flags |= SpriteRenderFlags::FlipY;
			}

			EditorGUI::EndPropertyGrid();
		}
	}

	void EntityProperties::RenderAddComponentMenu(Entity entity)
	{
		if (ImGui::BeginMenu("Add component"))
		{
			const std::vector<ComponentInfo>& components = m_World.Components.GetRegisteredComponents();

			for (const auto& info : components)
			{
				if (ImGui::MenuItem(info.Name.c_str()))
					m_World.Entities.AddEntityComponent(entity, info.Id, nullptr);
			}

			ImGui::End();
		}
	}
}
