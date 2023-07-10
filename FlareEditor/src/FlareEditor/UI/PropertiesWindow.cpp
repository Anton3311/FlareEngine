#include "PropertiesWindow.h"

#include "FlareEditor/EditorContext.h"
#include "FlareEditor/UI/EditorGUI.h"

#include <imgui.h>

namespace Flare
{
	void PropertiesWindow::OnImGuiRender()
	{
		World& world = EditorContext::GetActiveScene()->GetECSWorld();
		Entity selectedEntity = EditorContext::Instance.SelectedEntity;

		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();

		ImGui::Begin("Properties");

		if (world.IsEntityAlive(selectedEntity))
		{
			ImGui::BeginDisabled();
			ImGui::Text("Index %d Generation %d", selectedEntity.GetIndex(), selectedEntity.GetGeneration());
			ImGui::EndDisabled();
			
			RenderAddComponentMenu(selectedEntity);

			std::optional<ComponentId> removedComponent;
			for (ComponentId component : world.GetEntityComponents(selectedEntity))
			{
				const ComponentInfo& componentInfo = world.GetRegistry().GetComponentInfo(component);

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_FramePadding;
				if (ImGui::TreeNodeEx((void*)component, flags, "%s", componentInfo.Name.c_str()))
				{
					if (component == CameraComponent::Id)
						RenderCameraComponent(world.GetEntityComponent<CameraComponent>(selectedEntity));
					else if (component == TransformComponent::Id)
						RenderTransformComponent(world.GetEntityComponent<TransformComponent>(selectedEntity));
					else if (component == SpriteComponent::Id)
						RenderSpriteComponent(world.GetEntityComponent<SpriteComponent>(selectedEntity));

					ImGui::TreePop();
				}

				// TODO: fix
				/*if (ImGui::BeginPopupContextItem())
				{
					if (ImGui::MenuItem("Remove"))
						removedComponent = component;

					ImGui::End();
				}*/
			}

			if (removedComponent.has_value())
				world.GetRegistry().RemoveEntityComponent(selectedEntity, removedComponent.value());
		}

		ImGui::End();
	}

	void PropertiesWindow::RenderAddComponentMenu(Entity entity)
	{
		if (ImGui::BeginMenu("Add component"))
		{
			World& world = EditorContext::GetActiveScene()->GetECSWorld();
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
			EditorGUI::BeginToggleGroup("Projection");
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

			EditorGUI::EndPropertyGrid();
		}
	}
}