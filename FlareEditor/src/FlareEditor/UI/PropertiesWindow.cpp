#include "PropertiesWindow.h"

#include "FlareEditor/EditorContext.h"

#include <imgui.h>

namespace Flare
{
	void PropertiesWindow::OnImGuiRender()
	{
		World& world = EditorContext::Instance.ActiveScene->GetECSWorld();
		Entity selectedEntity = EditorContext::Instance.SelectedEntity;

		ImGui::Begin("Properties");

		if (world.IsEntityAlive(selectedEntity))
		{
			ImGui::BeginDisabled();
			ImGui::Text("Index %d Generation %d", selectedEntity.GetIndex(), selectedEntity.GetGeneration());
			ImGui::EndDisabled();

			for (ComponentId component : world.GetEntityComponents(selectedEntity))
			{
				if (ImGui::CollapsingHeader(world.GetRegistry().GetComponentInfo(component).Name.c_str()))
				{
					if (component == CameraComponent::Id)
						RenderCameraComponent(world.GetEntityComponent<CameraComponent>(selectedEntity));
				}
			}
		}

		ImGui::End();
	}

	void PropertiesWindow::RenderCameraComponent(CameraComponent& cameraComponent)
	{
		ImGui::DragFloat("Size", &cameraComponent.Size);
		ImGui::DragFloat("Near", &cameraComponent.Near);
		ImGui::DragFloat("Far", &cameraComponent.Far);
	}
}