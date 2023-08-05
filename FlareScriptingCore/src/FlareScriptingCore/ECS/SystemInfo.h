#pragma once

#include "FlareECS/System.h"

#include "FlareScriptingCore/ScriptingType.h"
#include "FlareScriptingCore/ECS/SystemConfiguration.h"
#include "FlareScriptingCore/ECS/EntityView.h"

#include <string>
#include <string_view>
#include <typeinfo>

namespace Flare::Scripting
{
	struct SystemInfo
	{
		SystemInfo(const std::string_view& name)
			: Name(name), Id(UINT32_MAX)
		{
			GetRegisteredSystems().push_back(this);
		}

		SystemId Id;
		const std::string_view Name;
	public:
		static std::vector<SystemInfo*>& GetRegisteredSystems();
	};

	class SystemBase
	{
	public:
		virtual void Configure(SystemConfiguration& config) = 0;
		virtual void OnUpdate() = 0;
	};

#ifndef FLARE_SCRIPTING_CORE_NO_MACROS
	#define FLARE_SYSTEM(systemName) \
			FLARE_DEFINE_SCRIPTING_TYPE(systemName) \
			static Flare::SystemInfo System;

	#define FLARE_SYSTEM_IMPL(systemName) \
		FLARE_IMPL_SCRIPTING_TYPE(systemName, systemName::ConfigureSerialization) \
		Flare::SystemInfo systemName::System = Flare::SystemInfo(typeid(systemName).name());
#endif
}