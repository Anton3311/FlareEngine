#pragma once

#include "FlareScriptingCore/ScriptingType.h"
#include "FlareScriptingCore/Bindings/ECS/SystemConfiguration.h"
#include "FlareScriptingCore/Bindings/ECS/EntityView.h"

#include <string>
#include <string_view>
#include <typeinfo>

namespace Flare::Internal
{
	struct SystemInfo
	{
		SystemInfo(const std::string_view& name)
			: Name(name)
		{
			GetRegisteredSystems().push_back(this);
		}

		const std::string_view Name;
	public:
		static std::vector<const SystemInfo*>& GetRegisteredSystems();
	};

	class SystemBase
	{
	public:
		virtual void Configure(SystemConfiguration& config) = 0;
		virtual void Execute(Internal::EntityView& chunk) = 0;
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