#include "PropertiesWindow.h"

#include "FlareEditor/EditorContext.h"
#include "FlareEditor/UI/EditorGUI.h"

#include <imgui.h>

namespace Flare
{
	void PropertiesWindow::OnImGuiRender()
	{
		World& world = EditorContext::Instance.ActiveScene->GetECSWorld();
		Entity selectedEntity = EditorContext::Instance.SelectedEntity;

		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();

		ImGui::Begin("Properties");

		if (world.IsEntityAlive(selectedEntity))
		{
			ImGui::BeginDisabled();
			ImGui::Text("Index %d Generation %d", selectedEntity.GetIndex(), selectedEntity.GetGeneration());
			ImGui::EndDisabled();

			for (ComponentId component : world.GetEntityComponents(selectedEntity))
			{
				const ComponentInfo& componentInfo = world.GetRegistry().GetComponentInfo(component);

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_FramePadding;
				if (ImGui::TreeNodeEx((void*)component, flags, "%s", componentInfo.Name.c_str()))
				{
					if (EditorGUI::BeginPropertyGrid())
					{
						if (component == CameraComponent::Id)
							RenderCameraComponent(world.GetEntityComponent<CameraComponent>(selectedEntity));
						else if (component == TransformComponent::Id)
							RenderTransformComponent(world.GetEntityComponent<TransformComponent>(selectedEntity));
						else if (component == SpriteComponent::Id)
							RenderSpriteComponent(world.GetEntityComponent<SpriteComponent>(selectedEntity));

						EditorGUI::EndPropertyGrid();
					}

					ImGui::TreePop();
				}
			}
		}

		ImGui::End();
	}

	void PropertiesWindow::RenderCameraComponent(CameraComponent& cameraComponent)
	{
		EditorGUI::FloatPropertyField("Size", cameraComponent.Size);
		EditorGUI::FloatPropertyField("Near", cameraComponent.Near);
		EditorGUI::FloatPropertyField("Far", cameraComponent.Far);
	}

	void PropertiesWindow::RenderTransformComponent(TransformComponent& transform)
	{
		EditorGUI::Vector3PropertyField("Position", transform.Position);
		EditorGUI::Vector3PropertyField("Rotation", transform.Rotation);
		EditorGUI::Vector3PropertyField("Scale", transform.Scale);
	}

	void PropertiesWindow::RenderSpriteComponent(SpriteComponent& sprite)
	{
		EditorGUI::ColorPropertyField("Color", sprite.Color);
	}
}