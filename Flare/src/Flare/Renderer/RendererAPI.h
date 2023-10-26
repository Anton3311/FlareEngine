#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/VertexArray.h"

#include <stdint.h>

namespace Flare
{
	enum class CullingMode : uint8_t
	{
		None,
		Back,
		Front,
	};

	inline const char* CullingModeToString(CullingMode mode)
	{
		switch (mode)
		{
		case CullingMode::Front:
			return "Front";
		case CullingMode::Back:
			return "Back";
		case CullingMode::None:
			return "None";
		}

		FLARE_CORE_ASSERT(false, "Unhandled culling mode");
		return nullptr;
	}

	inline CullingMode CullinModeFromString(std::string_view mode)
	{
		if (mode == "None")
			return CullingMode::None;
		if (mode == "Front")
			return CullingMode::Front;
		if (mode == "Back")
			return CullingMode::Back;
		
		FLARE_CORE_ASSERT(false, "Invalid culling mode");
		return CullingMode::None;
	}

	class FLARE_API RendererAPI
	{
	public:
		enum class API
		{
			None,
			OpenGL,
		};
	public:
		virtual void Initialize() = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

		virtual void SetClearColor(float r, float g, float b, float a) = 0;
		virtual void Clear() = 0;

		virtual void SetDepthTestEnabled(bool enabled) = 0;
		virtual void SetCullingMode(CullingMode mode) = 0;

		virtual void SetLineWidth(float width) = 0;

		virtual void DrawIndexed(const Ref<const VertexArray>& vertexArray) = 0;
		virtual void DrawIndexed(const Ref<const VertexArray>& vertexArray, size_t indicesCount) = 0;
		virtual void DrawLines(const Ref<const VertexArray>& vertexArray, size_t cverticesCountount) = 0;
	public:
		static Scope<RendererAPI> Create();
		static API GetAPI();
	};
}