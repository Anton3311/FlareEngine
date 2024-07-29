#pragma once

#include "FlareCore/Core.h"

#include "FlareECS/System/SystemData.h"
#include "FlareECS/System/SystemInitializer.h"

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace Flare
{
	struct SystemRecord
	{
		SystemId Id = INVALID_SYSTEM_ID;
		const SystemInitializer* Descriptor = nullptr;
	};

	class FLAREECS_API SystemsRegistry
	{
	public:
		SystemsRegistry();

		void RegisterSystems();
		void UnregisterSystems();

		void Clear();

		inline void ReregisterSystems()
		{
			UnregisterSystems();
			RegisterSystems();
		}

		const SystemRecord& GetRecord(SystemId id) const;

		inline bool IsSystemIdValid(SystemId id) const { return (size_t)id < m_SystemRecords.size(); }
	private:
		std::vector<SystemRecord> m_SystemRecords;
	};
}
