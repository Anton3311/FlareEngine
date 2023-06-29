#pragma once

#include "FlareECS/Registry.h"
#include "FlareECS/Component.h"

#include <vector>
#include <string_view>

namespace Flare
{
	class World
	{
	public:
		inline Registry& GetRegistry() { return m_Registry; }
	private:
		Registry m_Registry;
	};
}