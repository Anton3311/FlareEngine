#include "Components.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/Entity/ComponentInitializer.h"

#include <unordered_set>

namespace Flare
{
	Components::~Components()
	{
		Clear();
	}

	void Components::RegisterComponents()
	{
		FLARE_PROFILE_FUNCTION();
		for (ComponentInitializer* initializer : ComponentInitializer::GetInitializers())
		{
			if (IsComponentIdValid(initializer->m_Id))
				continue;

			Entity entityId = m_IdGenerator.CreateId();

			uint32_t registryIndex = (uint32_t)m_RegisteredComponents.size();
			ComponentInfo& info = m_RegisteredComponents.emplace_back();
			info.Id = ComponentId(entityId.GetIndex(), entityId.GetGeneration());
			info.RegistryIndex = registryIndex;
			info.Name = initializer->Type.TypeName;
			info.Size = initializer->Type.Size;
			info.Initializer = initializer;

			m_ComponentNameToIndex.emplace(info.Name, registryIndex);
			m_ComponentIdToIndex.emplace(info.Id, registryIndex);

			initializer->m_Id = info.Id;
			initializer->m_CorrespondingRegistry = this;
		}
	}

	void Components::ReregisterComponents()
	{
		FLARE_PROFILE_FUNCTION();
		std::vector<ComponentInfo> newComponents;
		std::unordered_set<uint32_t> reregisteredComponents;
		std::unordered_map<std::string, uint32_t> newNameToIndex;
		std::unordered_map<ComponentId, uint32_t> newIdToIndex;

		for (ComponentInitializer* initializer : ComponentInitializer::GetInitializers())
		{
			auto it = m_ComponentNameToIndex.find(std::string(initializer->Type.TypeName));
			bool found = it != m_ComponentNameToIndex.end();

			ComponentId id;
			ComponentInfo* info = nullptr;
			uint32_t registryIndex = UINT32_MAX;
			if (found)
			{
				id = m_RegisteredComponents[it->second].Id;
				reregisteredComponents.insert(it->second);

				registryIndex = (uint32_t)newComponents.size();
				info = &newComponents.emplace_back();
			}
			else
			{
				Entity entityId = m_IdGenerator.CreateId();
				id = ComponentId(entityId.GetIndex(), entityId.GetGeneration());
				registryIndex = (uint32_t)newComponents.size();
				info = &newComponents.emplace_back();
			}

			if (info)
			{
				info->Id = id;
				info->RegistryIndex = registryIndex;
				info->Name = initializer->Type.TypeName;
				info->Size = initializer->Type.Size;
				info->Initializer = initializer;

				newNameToIndex[info->Name] = registryIndex;
				newIdToIndex[info->Id] = registryIndex;

				initializer->m_Id = info->Id;
				initializer->m_CorrespondingRegistry = this;
			}
		}

		// Reuse previous component ids
		for (size_t i = 0; i < m_RegisteredComponents.size(); i++)
		{
			auto it = reregisteredComponents.find((uint32_t)i);
			if (it == reregisteredComponents.end())
			{
				ComponentId id = m_RegisteredComponents[i].Id;
				m_IdGenerator.AddDeletedId(Entity(id.GetIndex(), id.GetGeneration()));
			}
		}

		m_RegisteredComponents = std::move(newComponents);
		m_ComponentNameToIndex = std::move(newNameToIndex);
		m_ComponentIdToIndex = std::move(newIdToIndex);
	}

	void Components::OnComponentUnregister(ComponentInitializer& component)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(IsComponentIdValid(component.GetId()));
		FLARE_CORE_ASSERT(component.m_CorrespondingRegistry == this);

		// Notify about the remove first, in case the handler might still need
		// to query some information about this component
		{
			FLARE_PROFILE_SCOPE("NotifyUpdateHandlers");
			for (ComponentsRegistryUpdateHandler* handler : m_UpdateHandlers)
			{
				handler->OnComponentUnregister(component.GetId());
			}
		}

		m_ComponentNameToIndex.erase(std::string(component.Type.TypeName));
		m_ComponentIdToIndex.erase(component.GetId());

		uint32_t removedComponentRegistryIndex = m_ComponentIdToIndex[component.GetId()];
		if ((size_t)removedComponentRegistryIndex != m_RegisteredComponents.size() - 1)
		{
			// Move the last component in place of the removed one

			const ComponentInfo& lastComponentInfo = m_RegisteredComponents.back();
			uint32_t& oldMappingIndex = m_ComponentIdToIndex[lastComponentInfo.Id];

			m_RegisteredComponents[removedComponentRegistryIndex] = std::move(m_RegisteredComponents.back());
			m_RegisteredComponents[removedComponentRegistryIndex].RegistryIndex = removedComponentRegistryIndex;

			oldMappingIndex = removedComponentRegistryIndex;
		}

		m_RegisteredComponents.pop_back();

		InvalidateComponentInitializer(component);
	}

	void Components::Clear()
	{
		FLARE_PROFILE_FUNCTION();
		m_ComponentNameToIndex.clear();
		m_ComponentIdToIndex.clear();

		for (ComponentInfo& component : m_RegisteredComponents)
		{
			InvalidateComponentInitializer(*component.Initializer);
			m_IdGenerator.AddDeletedId(Entity(component.Id.GetIndex(), component.Id.GetGeneration()));
		}

		m_RegisteredComponents.clear();
	}

	std::optional<ComponentId> Components::FindComponent(std::string_view name) const
	{
		auto it = m_ComponentNameToIndex.find(std::string(name));
		if (it == m_ComponentNameToIndex.end())
			return {};
		FLARE_CORE_ASSERT(it->second < m_RegisteredComponents.size());
		return m_RegisteredComponents[it->second].Id;
	}

	bool Components::IsComponentIdValid(ComponentId id) const
	{
		auto it = m_ComponentIdToIndex.find(id);
		if (it == m_ComponentIdToIndex.end())
			return false;
		return it->second < m_RegisteredComponents.size();
	}

	void Components::AddUpdateHandler(ComponentsRegistryUpdateHandler& handler)
	{
		m_UpdateHandlers.push_back(&handler);
	}

	void Components::RemoveUpdateHandler(ComponentsRegistryUpdateHandler& handler)
	{
		FLARE_PROFILE_FUNCTION();
		auto it = std::find(m_UpdateHandlers.begin(), m_UpdateHandlers.end(), &handler);

		if (it != m_UpdateHandlers.end())
			m_UpdateHandlers.erase(it);
	}

	void Components::InvalidateComponentInitializer(ComponentInitializer& component) const
	{
		component.m_Id = ComponentId();
		component.m_CorrespondingRegistry = nullptr;
	}
}
