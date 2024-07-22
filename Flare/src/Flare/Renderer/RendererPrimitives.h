#pragma once

#include "FlareCore/Core.h"

namespace Flare
{
	class Mesh;
	class FLARE_API RendererPrimitives
	{
	public:
		static Ref<const Mesh> GetCube();
		static Ref<const Mesh> GetFullscreenQuadMesh();

		static void Clear();
	};
}
