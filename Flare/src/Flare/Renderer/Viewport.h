#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/FrameBuffer.h"
#include "Flare/Renderer/RenderData.h"

#include "Flare/Renderer/RenderGraph/RenderGraph.h"

#include <vector>
#include <glm/glm.hpp>

namespace Flare
{
	class Texture;
	class FLARE_API Viewport
	{
	public:
		inline glm::ivec2 GetPosition() const { return m_Position; }
		inline glm::ivec2 GetSize() const { return m_Size; }

		inline float GetAspectRatio() const { return (float)m_Size.x / (float)m_Size.y; }

		void Resize(glm::ivec2 position, glm::ivec2 size);
	public:
		inline bool IsPostProcessingEnabled() const { return m_PostProcessingEnabled; }
		void SetPostProcessingEnabled(bool enabled);

		inline bool IsShadowMappingEnabled() const { return m_ShadowMappingEnabled; }
		void SetShadowMappingEnabled(bool enabled);

		inline bool IsDebugRenderingEnabled() const { return m_DebugRenderingEnabled; }
		void SetDebugRenderingEnabled(bool enabled);
	public:
		RenderData FrameData;
		Ref<FrameBuffer> RenderTarget = nullptr;

		RenderGraph Graph;

		Ref<Texture> ColorTexture = nullptr;
		Ref<Texture> NormalsTexture = nullptr;
		Ref<Texture> DepthTexture = nullptr;
	private:
		bool m_PostProcessingEnabled = true;
		bool m_ShadowMappingEnabled = true;
		bool m_DebugRenderingEnabled = false;
	private:
		glm::ivec2 m_Position = glm::ivec2(0, 0);
		glm::ivec2 m_Size = glm::ivec2(0, 0);
	};
}