#pragma once

#include "FlareECS/Entity/Component.h"
#include "FlareECS/Entity/EntityIndex.h"

#include <vector>
#include <optional>
#include <unordered_map>

namespace Flare
{
	struct FLAREECS_API Components
	{
		Components() = default;
		Components(const Components&) = delete;

		Components(Components&& other) noexcept
			: ComponentNameToIndex(std::move(other.ComponentNameToIndex)),
			ComponentIdToIndex(std::move(other.ComponentIdToIndex)),
			RegisteredComponents(std::move(other.RegisteredComponents)) {}

		Components& operator=(const Components&) = delete;
		Components& operator=(Components&& other) noexcept
		{
			ComponentNameToIndex = std::move(other.ComponentNameToIndex);
			ComponentIdToIndex = std::move(other.ComponentIdToIndex);
			RegisteredComponents = std::move(other.RegisteredComponents);

			return *this;
		}

		void RegisterComponents();
		void ReregisterComponents();

		inline void Clear()
		{
			ComponentNameToIndex.clear();
			ComponentIdToIndex.clear();

			for (const ComponentInfo& component : RegisteredComponents)
				m_IdGenerator.AddDeletedId(Entity(component.Id.GetIndex(), component.Id.GetGeneration()));

			RegisteredComponents.clear();
		}

		std::optional<ComponentId> FindComponnet(std::string_view name) const;
		bool IsComponentIdValid(ComponentId id) const;

		inline const ComponentInfo& GetComponentInfo(ComponentId id) const
		{
			FLARE_CORE_ASSERT(IsComponentIdValid(id));
			return RegisteredComponents[ComponentIdToIndex.at(id)];
		}

		inline const std::vector<ComponentInfo>& GetRegisteredComponents() const
		{
			return RegisteredComponents;
		}
	private:
		std::unordered_map<std::string, uint32_t> ComponentNameToIndex;
		std::unordered_map<ComponentId, uint32_t> ComponentIdToIndex;
		std::vector<ComponentInfo> RegisteredComponents;

		EntityIndex m_IdGenerator;
	};
}