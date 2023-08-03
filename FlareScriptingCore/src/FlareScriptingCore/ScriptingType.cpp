#include "ScriptingType.h"

namespace Flare::Scripting
{
	std::vector<ScriptingType*>& ScriptingType::GetRegisteredTypes()
	{
		static std::vector<ScriptingType*> s_RegisteredTypes;
		return s_RegisteredTypes;
	}
}