#pragma once

#include "FlareECS/Query/Query.h"
#include "FlareECS/System.h"

#include <functional>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Flare
{
	class SystemExecutionContext
	{
	public:
		SystemExecutionContext(const Query& query)
			: m_Query(query) {}
	public:
		constexpr Query GetQuery() const { return m_Query; }
	private:
		Query m_Query;
	};

	using SystemEventFunction = std::function<void(SystemExecutionContext& context)>;

	struct SystemGroup
	{
		SystemGroupId Id;
		std::string Name;
		std::vector<uint32_t> SystemIndices;
	};

	struct SystemData
	{
		SystemData(const Query& query)
			: ExecutionContext(query) {}

		std::string Name;

		SystemExecutionContext ExecutionContext;

		SystemEventFunction OnBeforeUpdate;
		SystemEventFunction OnAfterUpdate;
		SystemEventFunction OnUpdate;

		uint32_t Index;
		uint32_t IndexInGroup;
	};

	class SystemsManager
	{
	public:
		SystemGroupId CreateGroup(std::string_view name);
		std::optional<SystemGroupId> FindGroup(std::string_view name) const;

		void RegisterSystem(std::string_view name, 
			SystemGroupId group,
			const Query& query,
			const SystemEventFunction& onBeforeUpdate = nullptr,
			const SystemEventFunction& onUpdate = nullptr,
			const SystemEventFunction& onAfterUpdate = nullptr);

		void ExecuteGroup(SystemGroupId id);
		inline bool IsGroupIdValid(SystemGroupId id)
		{
			return id < (SystemGroupId)m_Groups.size();
		}
		
		inline const std::vector<SystemGroup>& GetGroups() const { return m_Groups; }
		inline const std::vector<SystemData>& GetSystems() const { return m_Systems; }
	private:
		std::vector<SystemData> m_Systems;
		std::unordered_map<std::string, SystemGroupId> m_GroupNameToId;
		std::vector<SystemGroup> m_Groups;
	};
}