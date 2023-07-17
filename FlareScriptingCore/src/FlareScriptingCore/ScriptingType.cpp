#include "ScriptingType.h"

namespace Flare
{
	std::vector<ScriptingType>& ScriptingType::GetRegisteredTypes()
	{
		static std::vector<ScriptingType> s_RegisteredTypes;
		return s_RegisteredTypes;
	}

	FLARE_API const std::vector<ScriptingType>& GetRegisteredScriptingTypes()
	{
		return ScriptingType::GetRegisteredTypes();
	}
}