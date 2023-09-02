#include "PropertiesWindow.h"

#include "Flare/Scene/Scene.h"
#include "Flare/Scripting/ScriptingEngine.h"

#include "FlareEditor/UI/EditorGUI.h"
#include "FlareEditor/EditorLayer.h"

#include <imgui.h>

namespace Flare
{
	void PropertiesWindow::OnImGuiRender()
	{
		ImGui::Begin("Properties");

		if (Scene::GetActive() == nullptr)
		{
			ImGui::End();
			return;
		}

		World& world = Scene::GetActive()->GetECSWorld();
		Entity selectedEntity = EditorLayer::GetInstance().GetSelectedEntity();

		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();

		if (world.IsEntityAlive(selectedEntity))
		{
			ImGui::BeginDisabled();
			ImGui::Text("Index %d Generation %d", selectedEntity.GetIndex(), selectedEntity.GetGeneration());
			ImGui::EndDisabled();
			
			RenderAddComponentMenu(selectedEntity);

			if (ImGui::BeginChild("Components"))
			{
				std::optional<ComponentId> removedComponent;
				for (ComponentId component : world.GetEntityComponents(selectedEntity))
				{
					const ComponentInfo& componentInfo = world.GetRegistry().GetComponentInfo(component);

					ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_FramePadding;
					if (ImGui::TreeNodeEx((void*)std::hash<ComponentId>()(component), flags, "%s", componentInfo.Name.c_str()))
					{
						if (component == COMPONENT_ID(CameraComponent))
							RenderCameraComponent(world.GetEntityComponent<CameraComponent>(selectedEntity));
						else if (component == COMPONENT_ID(TransformComponent))
							RenderTransformComponent(world.GetEntityComponent<TransformComponent>(selectedEntity));
						else if (component == COMPONENT_ID(SpriteComponent))
							RenderSpriteComponent(world.GetEntityComponent<SpriteComponent>(selectedEntity));
						else
						{
							std::optional<void*> componentData = world.GetRegistry().GetEntityComponent(selectedEntity, component);
							if (componentData.has_value() && componentInfo.Initializer)
								EditorGUI::TypeEditor(componentInfo.Initializer->Type, (uint8_t*) componentData.value());
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
					world.GetRegistry().RemoveEntityComponent(selectedEntity, removedComponent.value());

				ImGui::EndChild();
			}
		}

		ImGui::End();
	}

	void PropertiesWindow::RenderAddComponentMenu(Entity entity)
	{
		if (ImGui::BeginMenu("Add component"))
		{
			World& world = Scene::GetActive()->GetECSWorld();
			const std::vector<ComponentInfo>& components = world.GetRegistry().GetRegisteredComponents();

			for (const auto& info : components)
			{
				if (ImGui::MenuItem(info.Name.c_str()))
					world.GetRegistry().AddEntityComponent(entity, info.Id, nullptr);
			}

			ImGui::End();
		}
	}

	void PropertiesWindow::RenderCameraComponent(CameraComponent& cameraComponent)
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

	void PropertiesWindow::RenderTransformComponent(TransformComponent& transform)
	{
		if (EditorGUI::BeginPropertyGrid())
		{
			EditorGUI::Vector3PropertyField("Position", transform.Position);
			EditorGUI::Vector3PropertyField("Rotation", transform.Rotation);
			EditorGUI::Vector3PropertyField("Scale", transform.Scale);

			EditorGUI::EndPropertyGrid();
		}
	}

	void PropertiesWindow::RenderSpriteComponent(SpriteComponent& sprite)
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
}