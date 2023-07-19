#pragma once

#include "FlareScriptingCore/Bindings/ECS/Component.h"

#include <string_view>
#include <optional>
#include <vector>

namespace Flare::Internal
{
	struct ComponentInfo
	{
	public:
		ComponentInfo(std::string_view name)
			: Id(INVALID_COMPONENT_ID), Name(name), AliasedName({})
		{
			GetRegisteredComponents().push_back(this);
		}

		ComponentInfo(std::string_view name, std::string_view aliasedName)
			: Id(INVALID_COMPONENT_ID), Name(name), AliasedName(aliasedName)
		{
			GetRegisteredComponents().push_back(this);
		}
	public:
		static std::vector<ComponentInfo*>& GetRegisteredComponents();
	public:
		ComponentId Id;
		const std::string_view Name;
		const std::optional<std::string_view> AliasedName;
	};
}

#ifndef FLARE_SCRIPTING_CORE_NO_MACROS
	#define FLARE_COMPONENT(component) \
		FLARE_DEFINE_SCRIPTING_TYPE(component) \
		static Flare::Internal::ComponentInfo Info;
	#define FLARE_COMPONENT_IMPL(component) \
		FLARE_IMPL_SCRIPTING_TYPE(component) \
		Flare::Internal::ComponentInfo component::Info = Flare::Internal::ComponentInfo(typeid(component).name());

	#define FLARE_COMPONENT_ALIAS_IMPL(component, aliasedComponent) \
		FLARE_IMPL_SCRIPTING_TYPE(component) \
		Flare::Internal::ComponentInfo component::Info = Flare::Internal::ComponentInfo(typeid(component).name(), aliasedComponent);
#endif
