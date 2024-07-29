#include "SystemsRegistry.h"

#include "FlareCore/Core.h"
#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	SystemsRegistry::SystemsRegistry()
	{
		UnregisterSystems();
	}

	void SystemsRegistry::RegisterSystems()
	{
		FLARE_PROFILE_FUNCTION();

		auto& initializers = SystemInitializer::GetInitializers();
		for (SystemInitializer* initializer : initializers)
		{
			SystemId id = (SystemId)m_SystemRecords.size();
			initializer->m_Id = id;

			SystemRecord& record = m_SystemRecords.emplace_back();
			record.Id = id;
			record.Descriptor = initializer;
		}
	}

	void SystemsRegistry::UnregisterSystems()
	{
		FLARE_PROFILE_FUNCTION();

		auto& initializers = SystemInitializer::GetInitializers();
		for (SystemInitializer* initializer : initializers)
		{
			initializer->m_Id = INVALID_SYSTEM_ID;
		}

		m_SystemRecords.clear();
	}

	void SystemsRegistry::Clear()
	{
		FLARE_PROFILE_FUNCTION();
		UnregisterSystems();
		m_SystemRecords.clear();
	}

	const SystemRecord& SystemsRegistry::GetRecord(SystemId id) const
	{
		FLARE_CORE_ASSERT(IsSystemIdValid(id));
		return m_SystemRecords[id];
	}
}
