#include "SystemsInspectorWindow.h"

#include "Flare/Scene/Scene.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"
#include "FlareEditor/UI/EditorGUI.h"

#include <glm/glm.hpp>

namespace Flare
{
	bool SystemsInspectorWindow::s_Opened = false;
	uint32_t SystemsInspectorWindow::s_CurrentSystem = UINT32_MAX;

	void SystemsInspectorWindow::OnImGuiRender()
	{
		if (s_Opened && ImGui::Begin("Systems Inspector", &s_Opened))
		{
			ImVec2 size = ImGui::GetContentRegionAvail();

			Ref<Scene> scene = Scene::GetActive();
			const World& world = scene->GetECSWorld();
			const SystemsManager& systems = world.GetSystemsManager();

			{
				ImGui::BeginChild("Systems");
				for (const SystemGroup& group : systems.GetGroups())
				{
					ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth;
					bool opened = ImGui::TreeNodeEx((void*)group.Name.c_str(), flags, "Group '%s'", group.Name.c_str());

					if (opened)
					{
						for (uint32_t systemIndex : group.SystemIndices)
							RenderSystemItem(systemIndex);

						ImGui::TreePop();
					}
				}

				ImGui::EndChild();
			}

			ImGui::End();
		}
	}

	void SystemsInspectorWindow::Show()
	{
		s_Opened = true;
	}

	void SystemsInspectorWindow::RenderSystemItem(uint32_t systemIndex)
	{
		Ref<Scene> scene = Scene::GetActive();
		const World& world = scene->GetECSWorld();
		const SystemsManager& systems = world.GetSystemsManager();

		const SystemData& systemData = systems.GetSystems()[systemIndex];
		auto type = ScriptingEngine::FindSystemType(systemIndex);
		auto instance = ScriptingEngine::FindSystemInstance(systemIndex);

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Leaf;
		bool opened = ImGui::TreeNodeEx((void*)systemData.Name.c_str(), flags, "System '%s'", systemData.Name.c_str());
		if (opened)
		{
			ImGui::TreePop();
		}

		if (ImGui::IsItemClicked())
		{
			s_CurrentSystem = systemIndex;
		}
	}
}
