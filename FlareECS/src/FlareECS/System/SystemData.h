#pragma once

#include "FlareCore/Collections/Span.h"

#include <string>
#include <optional>

namespace Flare
{
	class EntitiesCommandBuffer;

	using SystemId = uint32_t;
	using SystemGroupId = uint32_t;

	constexpr SystemId INVALID_SYSTEM_ID = std::numeric_limits<SystemId>::max();
	constexpr SystemId INVALID_SYSTEM_GROUP_ID = std::numeric_limits<SystemId>::max();

	struct SystemExecutionContext
	{
		EntitiesCommandBuffer* Commands = nullptr;
	};

	class System;
	struct FLAREECS_API SystemData
	{
	public:
		System* SystemInstance = nullptr;

		bool Enabled = true;

		SystemId Id = INVALID_SYSTEM_ID;
		uint32_t IndexInGroup = UINT32_MAX;
		SystemGroupId GroupId = INVALID_SYSTEM_GROUP_ID;

		void AddDependecy(SystemId system);
		void AddDependentSystem(SystemId system);

		inline Span<const SystemId> GetDependecies() const { return Span(m_Dependecies.data(), m_DependecyCount); }
		inline Span<const SystemId> GetDependentSystems() const { return Span(m_Dependecies.data(), m_Dependecies.size()).Slice(m_DependecyCount); }
	private:
		std::vector<SystemId> m_Dependecies;
		size_t m_DependecyCount = 0;
	};
}