#include "PCH.h"

#include "ECSInspector.h"

#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/World.h"
#include "FlareECS/EntityStorage/EntityStorage.h"
#include "FlareECS/Query/QueryData.h"

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
		FLARE_PROFILE_FUNCTION();
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
									uint32_t alignment = (uint32_t)component.Initializer->Type.Alignment;
									EditorGUI::UIntPropertyField("Alignment", alignment);

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

				if (ImGui::BeginTabItem("Queries"))
				{
					if (ImGui::BeginChild("Queries List"))
					{
						const QueryCache& queries = world.GetQueries();
						for (const QueryData& query : queries.GetQueries())
							RenderQuery(query);

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
		FLARE_PROFILE_FUNCTION();
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
		FLARE_PROFILE_FUNCTION();
		World& world = World::GetCurrent();
		const ArchetypeRecord& record = world.GetArchetypes()[archetype];

		if (EditorGUI::BeginPropertyGrid())
		{
			const EntityStorage& storage = world.Entities.GetEntityStorage(archetype);

			ImGui::BeginDisabled(true);
			uint32_t entitiesCount = (uint32_t)storage.GetEntityCount();
			EditorGUI::UIntPropertyField("Entities count", entitiesCount);
			uint32_t chunksCount = (uint32_t)storage.GetChunkCount();
			EditorGUI::UIntPropertyField("Chunks count", chunksCount);
			uint32_t entitiesPerChunk = (uint32_t)storage.GetEntitiesPerChunk();
			EditorGUI::UIntPropertyField("Entities per chunk", entitiesPerChunk);
			uint32_t entitySize = (uint32_t)storage.GetEntitySize();
			EditorGUI::UIntPropertyField("Entity Size", entitySize);

			{
				int32_t references = record.DeletionQueryReferences;
				EditorGUI::IntPropertyField("References in deletion queries", references);
			}

			{
				int32_t references = record.CreatedEntitiesQueryReferences;
				EditorGUI::IntPropertyField("References in created entities queries", references);
			}

			uint32_t queuedForDeletion = (uint32_t)world.Entities.GetDeletedEntityStorage(archetype).GetEntityCount();
			EditorGUI::UIntPropertyField("Queued for deletion", queuedForDeletion);
			ImGui::EndDisabled();

			EditorGUI::EndPropertyGrid();
		}

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding;
		if (ImGui::TreeNodeEx("Components", flags))
		{
			const ImGuiStyle& style = ImGui::GetStyle();
			for (ComponentId id : record.Components)
			{
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.FramePadding.y);
				ImGui::TextUnformatted(world.Entities.GetComponents().GetComponentInfo(id).Name.c_str());
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.FramePadding.y);
			}
			ImGui::TreePop();
		}
	}

	void ECSInspector::RenderSystem(uint32_t systemIndex)
	{
		FLARE_PROFILE_FUNCTION();
		World& world = World::GetCurrent();
		const SystemsRegistry& systemsRegistry = world.GetSystemsManager().GetSystemsRegistry();

		const SystemRecord& systemRecord = systemsRegistry.GetRecord(systemIndex);

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Leaf;
		bool opened = ImGui::TreeNodeEx((void*)systemRecord.Descriptor->TypeName, flags, "System '%s'", systemRecord.Descriptor->TypeName);
		if (opened)
		{
			ImGui::TreePop();
		}
	}

	void ECSInspector::RenderQuery(const QueryData& query)
	{
		FLARE_PROFILE_FUNCTION();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_OpenOnArrow;
		bool opened = ImGui::TreeNodeEx((void*)query.Id, flags, "Query %d", (int32_t)query.Id);
		if (opened)
		{
			const ImGuiStyle& style = ImGui::GetStyle();
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.FramePadding.y);

			switch (query.Target)
			{
			case QueryTarget::AllEntities:
				ImGui::TextUnformatted("AllEntities");
				break;
			case QueryTarget::DeletedEntities:
				ImGui::TextUnformatted("DeletedEntities");
				break;
			case QueryTarget::CreatedEntities:
				ImGui::TextUnformatted("CreatedEntities");
				break;
			}

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.FramePadding.y);

			if (ImGui::TreeNodeEx("Components", flags))
			{
				for (ComponentId id : query.Components)
				{
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.FramePadding.y);

					ImGui::Text("%s %s",
						HAS_BIT(id.GetIndex(), (uint32_t)QueryFilterType::Without)
							? "Without"
							: "With",
						World::GetCurrent().Components.GetComponentInfo(id.Masked()).Initializer->Type.TypeName.data());

					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.FramePadding.y);
				}

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Matching Archetypes", flags))
			{
				for (ArchetypeId archetype : query.MatchedArchetypes)
				{
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.FramePadding.y);
					ImGui::Text("%llu", (uint64_t)archetype);
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.FramePadding.y);
				}

				ImGui::TreePop();
			}

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
