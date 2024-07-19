#pragma once

#include "FlareCore/Core.h"

#include <stdint.h>

namespace Flare
{
	class FLARE_API RendererAPI
	{
	public:
		enum class API
		{
			None,
			Vulkan,
		};
	public:
		static void Create(API api);

		static API GetAPI();
	};
}