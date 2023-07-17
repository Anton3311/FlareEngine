#pragma once

#include "FlareScriptingCore/Defines.h"

#include <string>
#include <vector>
#include <typeinfo>
#include <iostream>

namespace Flare
{
	class ScriptingType
	{
	public:
		ScriptingType(const std::string& name, size_t size)
			: Name(name), Size(size)
		{
			GetRegisteredTypes().push_back(*this);
		}

		const std::string Name;
		const size_t Size;
	public:
		static std::vector<ScriptingType>& GetRegisteredTypes();
	};

	FLARE_API const std::vector<ScriptingType>& GetRegisteredScriptingTypes();
}

#define FLARE_DEFINE_SCRIPTING_TYPE(type) public: \
	static Flare::ScriptingType FlareScriptingType;
#define FLARE_IMPL_SCRIPTING_TYPE(type) \
	Flare::ScriptingType type::FlareScriptingType = Flare::ScriptingType(typeid(type).name(), sizeof(type));
