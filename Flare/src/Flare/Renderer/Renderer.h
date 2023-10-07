#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/Viewport.h"

#include "Flare/Renderer/VertexArray.h"
#include "Flare/Renderer/Material.h"

#include <glm/glm.hpp>

namespace Flare
{
	class FLARE_API Renderer
	{
	public:
		static void Initialize();
		static void Shutdown();

		static void SetMainViewport(Viewport& viewport);

		static void BeginScene(Viewport& viewport);
		static void EndScene();

		static void DrawMesh(const Ref<VertexArray>& mesh, const Ref<Material>& material, size_t indicesCount = SIZE_MAX);

		static Viewport& GetMainViewport();
		static Viewport& GetCurrentViewport();
	};
}