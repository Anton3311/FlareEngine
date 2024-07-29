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

	class SystemsRegisteringHandler
	{
	public:
		virtual void OnUnregisterSystems() = 0;
		virtual void OnRegisterSystems() = 0;
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

		void AddResigteringHandler(SystemsRegisteringHandler* handler);
		void RemoveRegisteringHandler(SystemsRegisteringHandler* handler);

		inline bool IsSystemIdValid(SystemId id) const { return (size_t)id < m_SystemRecords.size(); }
	private:
		std::vector<SystemRecord> m_SystemRecords;
		std::vector<SystemsRegisteringHandler*> m_RegisteringHandlers;
	};
}
