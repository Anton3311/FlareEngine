#include "SceneWindow.h"

#include "FlareECS/World.h"
#include "FlareECS/Query/EntityRegistryIterator.h"
#include "FlareECS/Registry.h"

#include "FlareEditor/EditorContext.h"

#include <imgui.h>

namespace Flare
{
	void SceneWindow::OnImGuiRender()
	{
		ImGui::Begin("Scene");

		World& world = EditorContext::Instance.ActiveScene->GetECSWorld();
		for (Entity entity : world.GetRegistry())
		{
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding;

			bool opened = ImGui::TreeNodeEx((void*)std::hash<Entity>()(entity), flags, "Entity %d", entity.GetIndex());

			if (ImGui::IsItemClicked())
				EditorContext::Instance.SelectedEntity = entity;

			if (opened)
			{
				ImGui::TreePop();
			}
		}

		ImGui::End();
	}
}