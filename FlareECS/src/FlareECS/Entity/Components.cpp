#include "Components.h"

#include "FlareECS/Entity/ComponentInitializer.h"

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
