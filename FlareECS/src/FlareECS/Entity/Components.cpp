#include "Components.h"

#include "FlareECS/Entity/ComponentInitializer.h"

#include <unordered_set>

namespace Flare
{
	void Components::RegisterComponents()
	{
		for (ComponentInitializer* initializer : ComponentInitializer::GetInitializers())
		{
			Entity entityId = m_IdGenerator.CreateId();

			uint32_t registryIndex = (uint32_t)RegisteredComponents.size();
			ComponentInfo& info = RegisteredComponents.emplace_back();

			info.Id = ComponentId(entityId.GetIndex(), entityId.GetGeneration());
			info.RegistryIndex = registryIndex;
			info.Name = initializer->Type.TypeName;
			info.Size = initializer->Type.Size;
			info.Deleter = initializer->Type.Destructor;
			info.Initializer = initializer;

			ComponentNameToIndex.emplace(info.Name, registryIndex);
			ComponentIdToIndex.emplace(info.Id, registryIndex);

			initializer->m_Id = info.Id;
		}
	}

	void Components::ReregisterComponents()
	{
		std::vector<ComponentInfo> newComponents;
		std::unordered_set<uint32_t> reregistedComponents;
		std::unordered_map<std::string, uint32_t> newNameToIndex;
		std::unordered_map<ComponentId, uint32_t> newIdToIndex;

		for (ComponentInitializer* initializer : ComponentInitializer::GetInitializers())
		{
			auto it = ComponentNameToIndex.find(std::string(initializer->Type.TypeName));
			bool found = it != ComponentNameToIndex.end();

			ComponentId id;
			ComponentInfo* info = nullptr;
			uint32_t registryIndex = UINT32_MAX;
			if (found)
			{
				id = RegisteredComponents[it->second].Id;
				reregistedComponents.insert(it->second);

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
				info->Deleter = initializer->Type.Destructor;
				info->Initializer = initializer;

				newNameToIndex[info->Name] = registryIndex;
				newIdToIndex[info->Id] = registryIndex;

				initializer->m_Id = info->Id;
			}
		}

		// Reuse previous component ids
		for (size_t i = 0; i < RegisteredComponents.size(); i++)
		{
			auto it = reregistedComponents.find((uint32_t)i);
			if (it == reregistedComponents.end())
			{
				ComponentId id = RegisteredComponents[i].Id;
				m_IdGenerator.AddDeletedId(Entity(id.GetIndex(), id.GetGeneration()));
			}
		}

		RegisteredComponents = std::move(newComponents);
		ComponentNameToIndex = std::move(newNameToIndex);
		ComponentIdToIndex = std::move(newIdToIndex);
	}

	std::optional<ComponentId> Components::FindComponnet(std::string_view name) const
	{
		auto it = ComponentNameToIndex.find(std::string(name));
		if (it == ComponentNameToIndex.end())
			return {};
		FLARE_CORE_ASSERT(it->second < RegisteredComponents.size());
		return RegisteredComponents[it->second].Id;
	}

	bool Components::IsComponentIdValid(ComponentId id) const
	{
		auto it = ComponentIdToIndex.find(id);
		if (it == ComponentIdToIndex.end())
			return false;
		return it->second < RegisteredComponents.size();
	}
}
