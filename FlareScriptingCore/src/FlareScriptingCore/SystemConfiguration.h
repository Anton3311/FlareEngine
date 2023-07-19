#pragma once

#include "FlareScriptingCore/Bindings/ECS/Component.h"

#include <vector>

namespace Flare::Internal
{
	class SystemQuery
	{
	public:
		SystemQuery(std::vector<ComponentId>* buffer)
			: m_Buffer(buffer) {}

		SystemQuery(const SystemQuery&) = delete;
		SystemQuery& operator=(const SystemQuery&) = delete;
	public:
		template<typename ComponentT>
		void Add()
		{
			m_Buffer->push_back(ComponentT::Info.Id);
		}
	private:
		std::vector<ComponentId>* m_Buffer;
	};

	class SystemConfiguration
	{
	public:
		SystemConfiguration(std::vector<ComponentId>* queryOutputBuffer)
			: Query(queryOutputBuffer) {}

		SystemConfiguration(SystemConfiguration&) = delete;
		SystemConfiguration& operator=(const SystemConfiguration&) = delete;
	public:
		SystemQuery Query;
	};
}