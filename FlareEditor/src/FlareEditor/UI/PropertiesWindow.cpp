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
					if (component == CameraComponent::Id)
					{
						RenderCameraComponent(world.GetEntityComponent<CameraComponent>(selectedEntity));
					}

					ImGui::TreePop();
				}
			}
		}

		ImGui::End();
	}

	void PropertiesWindow::RenderCameraComponent(CameraComponent& cameraComponent)
	{
		if (EditorGUI::BeginPropertyGrid())
		{
			EditorGUI::FloatPropertyField("Size", cameraComponent.Size);
			EditorGUI::FloatPropertyField("Near", cameraComponent.Near);
			EditorGUI::FloatPropertyField("Far", cameraComponent.Far);
			EditorGUI::EndPropertyGrid();
		}
	}
}