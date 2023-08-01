#include "SystemsInspectorWindow.h"

#include "Flare/Scene/Scene.h"
#include "Flare/ImGui/ImGUILayer.h"

#include "FlareEditor/EditorContext.h"
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
			ImVec2 systemsSectionSize = ImVec2(glm::max(300.0f, size.x * 0.3f), size.y);

			Ref<Scene> scene = Scene::GetActive();
			const World& world = scene->GetECSWorld();
			const SystemsManager& systems = world.GetSystemsManager();

			{
				ImGui::BeginChild("Systems", systemsSectionSize);
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

			ImGui::SameLine();
			ImGui::GetCurrentWindow()->DC.LayoutType = ImGuiLayoutType_Horizontal;
			ImGui::Separator();
			ImGui::GetCurrentWindow()->DC.LayoutType = ImGuiLayoutType_Vertical;
			ImGui::SameLine();

			{
				ImGui::BeginChild("System Editor", ImGui::GetContentRegionAvail());
				const auto& allSystems = systems.GetSystems();
				if (s_CurrentSystem < allSystems.size())
				{
					auto type = ScriptingEngine::FindSystemType(s_CurrentSystem);
					auto inst = ScriptingEngine::FindSystemInstance(s_CurrentSystem);

					if (type.has_value() && inst.has_value())
						EditorGUI::TypeEditor(*(type.value()), (uint8_t*)inst.value());
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
