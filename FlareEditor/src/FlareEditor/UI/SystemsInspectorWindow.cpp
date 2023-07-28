#include "SystemsInspectorWindow.h"

#include "Flare/Scene/Scene.h"

#include "FlareEditor/EditorContext.h"

#include <imgui.h>

namespace Flare
{
	bool SystemsInspectorWindow::s_Opened = false;

	void SystemsInspectorWindow::OnImGuiRender()
	{
		if (s_Opened && ImGui::Begin("Systems Inspector", &s_Opened))
		{
			Ref<Scene> scene = EditorContext::GetActiveScene();
			const World& world = scene->GetECSWorld();
			const SystemsManager& systems = world.GetSystemsManager();

			for (const SystemGroup& group : systems.GetGroups())
			{
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth;
				bool opened = ImGui::TreeNodeEx((void*)group.Name.c_str(), flags, "Group '%s'", group.Name.c_str());

				if (opened)
				{
					for (uint32_t systemIndex : group.SystemIndices)
					{
						const SystemData& systemData = systems.GetSystems()[systemIndex];

						ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Bullet;
						bool opened = ImGui::TreeNodeEx((void*)systemData.Name.c_str(), flags, "System '%s'", group.Name.c_str());
						if (opened)
							ImGui::TreePop();
					}

					ImGui::TreePop();
				}
			}

			ImGui::End();
		}
	}

	void SystemsInspectorWindow::Show()
	{
		s_Opened = true;
	}
}
