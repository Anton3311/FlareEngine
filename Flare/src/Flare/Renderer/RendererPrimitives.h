#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/Mesh.h"

namespace Flare
{
	class FLARE_API RendererPrimitives
	{
	public:
		static Ref<const Mesh> GetCube();
		static Ref<const VertexArray> GetFullscreenQuad();
		static Ref<const Mesh> GetFullscreenQuadMesh();

		static void Clear();
	};
}
