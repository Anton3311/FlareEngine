#include "ECSInspector.h"

#include "FlareEditor/EditorSelection.h"
#include "FlareEditor/EditorLayer.h"

#include "FlareEditor/UI/EditorGUI.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"

#include <imgui.h>

namespace Flare
{
	ECSInspector s_Instance;

	void ECSInspector::OnImGuiRender()
	{
		if (m_Shown && ImGui::Begin("ECS Inspector", &m_Shown))
		{
			if (ImGui::BeginTabBar("ECS Inspector Tabs"))
			{
				World& world = World::GetCurrent();
				if (ImGui::BeginTabItem("Entity Info"))
				{
					const EditorSelection& selection = EditorLayer::GetInstance().Selection;
					bool hasValidSelection = false;

					Entity selected;

					if (selection.GetType() == EditorSelectionType::Entity)
					{
						hasValidSelection = true;

						selected = selection.GetEntity();

						if (!world.IsEntityAlive(selected))
							hasValidSelection = false;
					}

					if (hasValidSelection)
					{
						ImGui::SeparatorText("Entity");
						RenderEntityInfo(selected);

						ArchetypeId archetype = world.Entities.GetEntityArchetype(selected);

						ImGui::SeparatorText("Archetype");
						RenderArchetypeInfo(archetype);
					}
					else
					{
						const char* errorText = "No selected entity";

						ImVec2 size = ImGui::CalcTextSize(errorText);
						ImVec2 windowSize = ImGui::GetContentRegionAvail();

						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + windowSize.x / 2.0f - size.x / 2.0f);
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().FramePadding.y);

						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.23f, 0.11f, 1.0f));
						ImGui::Text(errorText);
						ImGui::PopStyleColor();
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Components"))
				{
					if (ImGui::BeginChild("Components List"))
					{
						const auto& components = world.Components.GetRegisteredComponents();
						for (const ComponentInfo& component : components)
						{
							ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth;
							bool opened = ImGui::TreeNodeEx((void*)component.Name.c_str(), flags, component.Name.c_str());

							if (opened)
							{
								if (EditorGUI::BeginPropertyGrid())
								{
									ImGui::BeginDisabled(true);

									uint32_t index = component.Id.GetIndex();
									uint32_t generation = component.Id.GetGeneration();

									EditorGUI::UIntPropertyField("Index", index);
									EditorGUI::UIntPropertyField("Generation", generation);

									uint32_t size = (uint32_t)component.Size;
									EditorGUI::UIntPropertyField("Size", size);

									ImGui::EndDisabled();

									EditorGUI::EndPropertyGrid();
								}
								ImGui::TreePop();
							}
						}

						ImGui::EndChild();
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Archetypes"))
				{
					if (ImGui::BeginChild("Archetypes List"))
					{
						for (const auto& archetype : world.GetArchetypes().Records)
						{
							ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth;
							bool opened = ImGui::TreeNodeEx((void*)&archetype, flags, "Archetype %d", archetype.Id);

							if (opened)
							{
								RenderArchetypeInfo(archetype.Id);
								ImGui::TreePop();
							}
						}

						ImGui::EndChild();
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Systems"))
				{
					const SystemsManager& systems = world.GetSystemsManager();

					if (ImGui::BeginChild("Systems List"))
					{
						for (const SystemGroup& group : systems.GetGroups())
						{
							ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth;
							bool opened = ImGui::TreeNodeEx((void*)group.Name.c_str(), flags, "Group '%s'", group.Name.c_str());

							if (opened)
							{
								for (uint32_t systemIndex : group.SystemIndices)
									RenderSystem(systemIndex);

								ImGui::TreePop();
							}
						}

						ImGui::EndChild();
					}

					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
			ImGui::End();
		}
	}

	void ECSInspector::RenderEntityInfo(Entity entity)
	{
		if (EditorGUI::BeginPropertyGrid())
		{
			uint32_t index = entity.GetIndex();
			uint32_t generation = entity.GetGeneration();

			ImGui::BeginDisabled(true);
			EditorGUI::UIntPropertyField("Index", index);
			EditorGUI::UIntPropertyField("Generation", generation);
			ImGui::EndDisabled();

			EditorGUI::EndPropertyGrid();
		}
	}

	void ECSInspector::RenderArchetypeInfo(ArchetypeId archetype)
	{
		World& world = World::GetCurrent();
		if (EditorGUI::BeginPropertyGrid())
		{
			const ArchetypeRecord& record = world.GetArchetypes()[archetype];
			const EntityStorage& storage = world.Entities.GetEntityStorage(archetype);

			ImGui::BeginDisabled(true);
			uint32_t entitiesCount = (uint32_t)storage.GetEntitiesCount();
			EditorGUI::UIntPropertyField("Entities count", entitiesCount);
			uint32_t chunksCount = (uint32_t)storage.GetChunksCount();
			EditorGUI::UIntPropertyField("Chunks count", chunksCount);
			uint32_t entitiesPerChunk = (uint32_t)storage.GetEntitiesPerChunkCount();
			EditorGUI::UIntPropertyField("Entities per chunk", entitiesPerChunk);
			uint32_t entitySize = (uint32_t)storage.GetEntitySize();
			EditorGUI::UIntPropertyField("Entity Size", entitySize);
			ImGui::EndDisabled();

			EditorGUI::EndPropertyGrid();
		}
	}

	void ECSInspector::RenderSystem(uint32_t systemIndex)
	{
		World& world = World::GetCurrent();
		const SystemsManager& systems = world.GetSystemsManager();
		const SystemData& systemData = systems.GetSystems()[systemIndex];

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Leaf;
		bool opened = ImGui::TreeNodeEx((void*)systemData.Name.c_str(), flags, "System '%s'", systemData.Name.c_str());
		if (opened)
		{
			ImGui::TreePop();
		}
	}

	void ECSInspector::Show()
	{
		s_Instance.m_Shown = true;
	}

	ECSInspector& ECSInspector::GetInstance()
	{
		return s_Instance;
	}
}
