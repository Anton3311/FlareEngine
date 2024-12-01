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
		FLARE_NONCOPYABLE(Components);
		FLARE_NONMOVABLE(Components);

		Components() = default;
		~Components();

		void RegisterComponents();
		void ReregisterComponents();

		void OnComponentUnregister(ComponentInitializer& component);
		void Clear();

		std::optional<ComponentId> FindComponent(std::string_view name) const;
		bool IsComponentIdValid(ComponentId id) const;

		inline const ComponentInfo& GetComponentInfo(ComponentId id) const
		{
			FLARE_CORE_ASSERT(IsComponentIdValid(id));
			return m_RegisteredComponents[m_ComponentIdToIndex.at(id)];
		}

		inline const std::vector<ComponentInfo>& GetRegisteredComponents() const
		{
			return m_RegisteredComponents;
		}
	private:
		void InvalidateComponentInitializer(ComponentInitializer& component) const;
	private:
		std::unordered_map<std::string, uint32_t> m_ComponentNameToIndex;
		std::unordered_map<ComponentId, uint32_t> m_ComponentIdToIndex;
		std::vector<ComponentInfo> m_RegisteredComponents;

		EntityIndex m_IdGenerator;
	};
}